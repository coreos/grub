/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2010,2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/cryptodisk.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/err.h>
#include <grub/disk.h>
#include <grub/crypto.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define MAX_PASSPHRASE 256

#define LUKS_KEY_ENABLED  0x00AC71F3

/* On disk LUKS header */
struct grub_luks_phdr
{
  grub_uint8_t magic[6];
#define LUKS_MAGIC        "LUKS\xBA\xBE"
  grub_uint16_t version;
  char cipherName[32];
  char cipherMode[32];
  char hashSpec[32];
  grub_uint32_t payloadOffset;
  grub_uint32_t keyBytes;
  grub_uint8_t mkDigest[20];
  grub_uint8_t mkDigestSalt[32];
  grub_uint32_t mkDigestIterations;
  char uuid[40];
  struct
  {
    grub_uint32_t active;
    grub_uint32_t passwordIterations;
    grub_uint8_t passwordSalt[32];
    grub_uint32_t keyMaterialOffset;
    grub_uint32_t stripes;
  } keyblock[8];
} __attribute__ ((packed));

typedef struct grub_luks_phdr *grub_luks_phdr_t;

gcry_err_code_t AF_merge (const gcry_md_spec_t * hash, grub_uint8_t * src,
			  grub_uint8_t * dst, grub_size_t blocksize,
			  grub_size_t blocknumbers);

static const struct grub_arg_option options[] =
  {
    {"uuid", 'u', 0, N_("Mount by UUID."), 0, 0},
    {"all", 'a', 0, N_("Mount all."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static int check_uuid, have_it;
static char *search_uuid;

static grub_cryptodisk_t
configure_ciphers (const struct grub_luks_phdr *header)
{
  grub_cryptodisk_t newdev;
  const char *iptr;
  char *optr;
  char uuid[sizeof (header->uuid) + 1];
  char ciphername[sizeof (header->cipherName) + 1];
  char ciphermode[sizeof (header->cipherMode) + 1];
  char *cipheriv = NULL;
  char hashspec[sizeof (header->hashSpec) + 1];
  grub_crypto_cipher_handle_t cipher = NULL, secondary_cipher = NULL;
  grub_crypto_cipher_handle_t essiv_cipher = NULL;
  const gcry_md_spec_t *hash = NULL, *essiv_hash = NULL;
  const struct gcry_cipher_spec *ciph;
  grub_cryptodisk_mode_t mode;
  grub_cryptodisk_mode_iv_t mode_iv;
  int benbi_log = 0;

  /* Look for LUKS magic sequence.  */
  if (grub_memcmp (header->magic, LUKS_MAGIC, sizeof (header->magic))
      || grub_be_to_cpu16 (header->version) != 1)
    return NULL;

  optr = uuid;
  for (iptr = header->uuid; iptr < &header->uuid[ARRAY_SIZE (header->uuid)];
       iptr++)
    {
      if (*iptr != '-')
	*optr++ = *iptr;
    }
  *optr = 0;

  if (check_uuid && grub_strcasecmp (search_uuid, uuid) != 0)
    {
      grub_dprintf ("luks", "%s != %s", uuid, search_uuid);
      return NULL;
    }

  /* Make sure that strings are null terminated.  */
  grub_memcpy (ciphername, header->cipherName, sizeof (header->cipherName));
  ciphername[sizeof (header->cipherName)] = 0;
  grub_memcpy (ciphermode, header->cipherMode, sizeof (header->cipherMode));
  ciphermode[sizeof (header->cipherMode)] = 0;
  grub_memcpy (hashspec, header->hashSpec, sizeof (header->hashSpec));
  hashspec[sizeof (header->hashSpec)] = 0;

  ciph = grub_crypto_lookup_cipher_by_name (ciphername);
  if (!ciph)
    {
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Cipher %s isn't available",
		  ciphername);
      return NULL;
    }

  /* Configure the cipher used for the bulk data.  */
  cipher = grub_crypto_cipher_open (ciph);
  if (!cipher)
    return NULL;

  if (grub_be_to_cpu32 (header->keyBytes) > 1024)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid keysize %d",
		  grub_be_to_cpu32 (header->keyBytes));
      return NULL;
    }

  /* Configure the cipher mode.  */
  if (grub_strcmp (ciphermode, "ecb") == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_ECB;
      mode_iv = GRUB_CRYPTODISK_MODE_IV_PLAIN;
      cipheriv = NULL;
    }
  else if (grub_strcmp (ciphermode, "plain") == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_CBC;
      mode_iv = GRUB_CRYPTODISK_MODE_IV_PLAIN;
      cipheriv = NULL;
    }
  else if (grub_memcmp (ciphermode, "cbc-", sizeof ("cbc-") - 1) == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_CBC;
      cipheriv = ciphermode + sizeof ("cbc-") - 1;
    }
  else if (grub_memcmp (ciphermode, "pcbc-", sizeof ("pcbc-") - 1) == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_PCBC;
      cipheriv = ciphermode + sizeof ("pcbc-") - 1;
    }
  else if (grub_memcmp (ciphermode, "xts-", sizeof ("xts-") - 1) == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_XTS;
      cipheriv = ciphermode + sizeof ("xts-") - 1;
      secondary_cipher = grub_crypto_cipher_open (ciph);
      if (!secondary_cipher)
	{
	  grub_crypto_cipher_close (cipher);
	  return NULL;
	}
      if (cipher->cipher->blocksize != GRUB_CRYPTODISK_GF_BYTES)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      cipher->cipher->blocksize);
	  return NULL;
	}
      if (secondary_cipher->cipher->blocksize != GRUB_CRYPTODISK_GF_BYTES)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      secondary_cipher->cipher->blocksize);
	  return NULL;
	}
    }
  else if (grub_memcmp (ciphermode, "lrw-", sizeof ("lrw-") - 1) == 0)
    {
      mode = GRUB_CRYPTODISK_MODE_LRW;
      cipheriv = ciphermode + sizeof ("lrw-") - 1;
      if (cipher->cipher->blocksize != GRUB_CRYPTODISK_GF_BYTES)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported LRW block size: %d",
		      cipher->cipher->blocksize);
	  return NULL;
	}
    }
  else
    {
      grub_crypto_cipher_close (cipher);
      grub_error (GRUB_ERR_BAD_ARGUMENT, "Unknown cipher mode: %s",
		  ciphermode);
      return NULL;
    }

  if (cipheriv == NULL);
  else if (grub_memcmp (cipheriv, "plain", sizeof ("plain") - 1) == 0)
      mode_iv = GRUB_CRYPTODISK_MODE_IV_PLAIN;
  else if (grub_memcmp (cipheriv, "plain64", sizeof ("plain64") - 1) == 0)
      mode_iv = GRUB_CRYPTODISK_MODE_IV_PLAIN64;
  else if (grub_memcmp (cipheriv, "benbi", sizeof ("benbi") - 1) == 0)
    {
      if (cipher->cipher->blocksize & (cipher->cipher->blocksize - 1)
	  || cipher->cipher->blocksize == 0)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported benbi blocksize: %d",
		    cipher->cipher->blocksize);
      for (benbi_log = 0; 
	   (cipher->cipher->blocksize << benbi_log) < GRUB_DISK_SECTOR_SIZE;
	   benbi_log++);
      mode_iv = GRUB_CRYPTODISK_MODE_IV_BENBI;
    }
  else if (grub_memcmp (cipheriv, "null", sizeof ("null") - 1) == 0)
      mode_iv = GRUB_CRYPTODISK_MODE_IV_NULL;
  else if (grub_memcmp (cipheriv, "essiv:", sizeof ("essiv:") - 1) == 0)
    {
      char *hash_str = cipheriv + 6;

      mode_iv = GRUB_CRYPTODISK_MODE_IV_ESSIV;

      /* Configure the hash and cipher used for ESSIV.  */
      essiv_hash = grub_crypto_lookup_md_by_name (hash_str);
      if (!essiv_hash)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_error (GRUB_ERR_FILE_NOT_FOUND,
		      "Couldn't load %s hash", hash_str);
	  return NULL;
	}
      essiv_cipher = grub_crypto_cipher_open (ciph);
      if (!essiv_cipher)
	{
	  grub_crypto_cipher_close (cipher);
	  return NULL;
	}
    }
  else
    {
      grub_crypto_cipher_close (cipher);
      grub_error (GRUB_ERR_BAD_ARGUMENT, "Unknown IV mode: %s",
		  cipheriv);
      return NULL;
    }

  /* Configure the hash used for the AF splitter and HMAC.  */
  hash = grub_crypto_lookup_md_by_name (hashspec);
  if (!hash)
    {
      grub_crypto_cipher_close (cipher);
      grub_crypto_cipher_close (essiv_cipher);
      grub_crypto_cipher_close (secondary_cipher);
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
		  hashspec);
      return NULL;
    }

  newdev = grub_zalloc (sizeof (struct grub_cryptodisk));
  if (!newdev)
    return NULL;
  newdev->cipher = cipher;
  newdev->offset = grub_be_to_cpu32 (header->payloadOffset);
  newdev->source_disk = NULL;
  newdev->benbi_log = benbi_log;
  newdev->mode = mode;
  newdev->mode_iv = mode_iv;
  newdev->secondary_cipher = secondary_cipher;
  newdev->essiv_cipher = essiv_cipher;
  newdev->essiv_hash = essiv_hash;
  newdev->hash = hash;
  newdev->log_sector_size = 9;
  grub_memcpy (newdev->uuid, uuid, sizeof (newdev->uuid));
  COMPILE_TIME_ASSERT (sizeof (newdev->uuid) >= sizeof (uuid));
  return newdev;
}

static grub_err_t
luks_recover_key (grub_cryptodisk_t dev, const struct grub_luks_phdr *header,
		  const char *name, grub_disk_t source)
{
  grub_size_t keysize = grub_be_to_cpu32 (header->keyBytes);
  grub_uint8_t candidate_key[keysize];
  grub_uint8_t digest[keysize];
  grub_uint8_t *split_key = NULL;
  char passphrase[MAX_PASSPHRASE] = "";
  grub_uint8_t candidate_digest[sizeof (header->mkDigest)];
  unsigned i;
  grub_size_t length;
  grub_err_t err;
  grub_size_t max_stripes = 1;

  grub_printf ("Attempting to decrypt master key...\n");

  for (i = 0; i < ARRAY_SIZE (header->keyblock); i++)
    if (grub_be_to_cpu32 (header->keyblock[i].active) == LUKS_KEY_ENABLED
	&& grub_be_to_cpu32 (header->keyblock[i].stripes) > max_stripes)
      max_stripes = grub_be_to_cpu32 (header->keyblock[i].stripes);

  split_key = grub_malloc (keysize * max_stripes);
  if (!split_key)
    return grub_errno;

  /* Get the passphrase from the user.  */
  grub_printf ("Enter passphrase for %s (%s): ", name, dev->uuid);
  if (!grub_password_get (passphrase, MAX_PASSPHRASE))
    {
      grub_free (split_key);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "Passphrase not supplied");
    }

  /* Try to recover master key from each active keyslot.  */
  for (i = 0; i < ARRAY_SIZE (header->keyblock); i++)
    {
      gcry_err_code_t gcry_err;

      /* Check if keyslot is enabled.  */
      if (grub_be_to_cpu32 (header->keyblock[i].active) != LUKS_KEY_ENABLED)
	continue;

      grub_dprintf ("luks", "Trying keyslot %d\n", i);

      /* Calculate the PBKDF2 of the user supplied passphrase.  */
      gcry_err = grub_crypto_pbkdf2 (dev->hash, (grub_uint8_t *) passphrase,
				     grub_strlen (passphrase),
				     header->keyblock[i].passwordSalt,
				     sizeof (header->keyblock[i].passwordSalt),
				     grub_be_to_cpu32 (header->keyblock[i].
						       passwordIterations),
				     digest, keysize);

      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_dprintf ("luks", "PBKDF2 done\n");

      gcry_err = grub_cryptodisk_setkey (dev, digest, keysize); 
      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      length = (keysize * grub_be_to_cpu32 (header->keyblock[i].stripes));

      /* Read and decrypt the key material from the disk.  */
      err = grub_disk_read (source,
			    grub_be_to_cpu32 (header->keyblock
					      [i].keyMaterialOffset), 0,
			    length, split_key);
      if (err)
	{
	  grub_free (split_key);
	  return err;
	}

      gcry_err = grub_cryptodisk_decrypt (dev, split_key, length, 0);
      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Merge the decrypted key material to get the candidate master key.  */
      gcry_err = AF_merge (dev->hash, split_key, candidate_key, keysize,
			   grub_be_to_cpu32 (header->keyblock[i].stripes));
      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_dprintf ("luks", "candidate key recovered\n");

      /* Calculate the PBKDF2 of the candidate master key.  */
      gcry_err = grub_crypto_pbkdf2 (dev->hash, candidate_key,
				     grub_be_to_cpu32 (header->keyBytes),
				     header->mkDigestSalt,
				     sizeof (header->mkDigestSalt),
				     grub_be_to_cpu32
				     (header->mkDigestIterations),
				     candidate_digest,
				     sizeof (candidate_digest));
      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Compare the calculated PBKDF2 to the digest stored
         in the header to see if it's correct.  */
      if (grub_memcmp (candidate_digest, header->mkDigest,
		       sizeof (header->mkDigest)) != 0)
	{
	  grub_dprintf ("luks", "bad digest\n");
	  continue;
	}

      grub_printf ("Slot %d opened\n", i);

      /* Set the master key.  */
      gcry_err = grub_cryptodisk_setkey (dev, candidate_key, keysize); 
      if (gcry_err)
	{
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_free (split_key);

      return GRUB_ERR_NONE;
    }

  return GRUB_ACCESS_DENIED;
}

static void
luks_close (grub_cryptodisk_t luks)
{
  grub_crypto_cipher_close (luks->cipher);
  grub_crypto_cipher_close (luks->secondary_cipher);
  grub_crypto_cipher_close (luks->essiv_cipher);
  grub_free (luks);
}

static grub_err_t
grub_luks_scan_device_real (const char *name, grub_disk_t source)
{
  grub_err_t err;
  struct grub_luks_phdr header;
  grub_cryptodisk_t newdev, dev;

  dev = grub_cryptodisk_get_by_source_disk (source);

  if (dev)
    return GRUB_ERR_NONE;

  /* Read the LUKS header.  */
  err = grub_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  if (!newdev)
    return grub_errno;

  newdev->total_length = grub_disk_get_size (source) - newdev->offset;

  err = luks_recover_key (newdev, &header, name, source);
  if (err)
    {
      luks_close (newdev);
      return err;
    }

  grub_cryptodisk_insert (newdev, name, source);

  have_it = 1;

  return GRUB_ERR_NONE;
}

#ifdef GRUB_UTIL
grub_err_t
grub_luks_cheat_mount (const char *sourcedev, const char *cheat)
{
  grub_err_t err;
  struct grub_luks_phdr header;
  grub_cryptodisk_t newdev, dev;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (sourcedev);
  if (!source)
    return grub_errno;

  dev = grub_cryptodisk_get_by_source_disk (source);

  if (dev)
    {
      grub_disk_close (source);	
      return GRUB_ERR_NONE;
    }

  /* Read the LUKS header.  */
  err = grub_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  if (!newdev)
    {
      grub_disk_close (source);
      return grub_errno;
    }

  newdev->total_length = grub_disk_get_size (source) - newdev->offset;

  err = grub_cryptodisk_cheat_insert (newdev, sourcedev, source, cheat);
  grub_disk_close (source);
  if (err)
    grub_free (newdev);

  return err;
}
#endif

static int
grub_luks_scan_device (const char *name)
{
  grub_err_t err;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (name);
  if (!source)
    return grub_errno;

  err = grub_luks_scan_device_real (name, source);

  grub_disk_close (source);
  
  if (err)
    grub_print_error ();
  return have_it && check_uuid ? 0 : 1;
}

#ifdef GRUB_UTIL

void
grub_util_luks_print_uuid (grub_disk_t disk)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  grub_printf ("%s ", dev->uuid);
}
#endif

static grub_err_t
grub_cmd_luksmount (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;

  if (argc < 1 && !state[1].set)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");

  have_it = 0;
  if (state[0].set)
    {
      grub_cryptodisk_t dev;

      dev = grub_cryptodisk_get_by_uuid (args[0]);
      if (dev)
	{
	  grub_dprintf ("luks", "already mounted as crypto%lu\n", dev->id);
	  return GRUB_ERR_NONE;
	}

      check_uuid = 1;
      search_uuid = args[0];
      grub_device_iterate (&grub_luks_scan_device);
      search_uuid = NULL;

      if (!have_it)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such luks found");
      return GRUB_ERR_NONE;
    }
  else if (state[1].set)
    {
      check_uuid = 0;
      search_uuid = NULL;
      grub_device_iterate (&grub_luks_scan_device);
      search_uuid = NULL;
      return GRUB_ERR_NONE;
    }
  else
    {
      grub_err_t err;
      grub_disk_t disk;
      grub_cryptodisk_t dev;

      check_uuid = 0;
      search_uuid = NULL;
      disk = grub_disk_open (args[0]);
      if (!disk)
	return grub_errno;

      dev = grub_cryptodisk_get_by_source_disk (disk);
      if (dev)
	{
	  grub_dprintf ("luks", "already mounted as luks%lu\n", dev->id);
	  grub_disk_close (disk);
	  return GRUB_ERR_NONE;
	}

      err = grub_luks_scan_device_real (args[0], disk);

      grub_disk_close (disk);

      return err;
    }
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (luks)
{
  COMPILE_TIME_ASSERT (sizeof (((struct grub_luks_phdr *) 0)->uuid)
		       < GRUB_CRYPTODISK_MAX_UUID_LENGTH);
  cmd = grub_register_extcmd ("luksmount", grub_cmd_luksmount, 0,
			      N_("SOURCE|-u UUID|-a"),
			      N_("Mount a LUKS device."), options);
}

GRUB_MOD_FINI (luks)
{
  grub_unregister_extcmd (cmd);
}
