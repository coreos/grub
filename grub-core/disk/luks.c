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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/err.h>
#include <grub/disk.h>
#include <grub/crypto.h>
#include <grub/normal.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define MAX_PASSPHRASE 256

#define LUKS_KEY_ENABLED  0x00AC71F3
#define LUKS_STRIPES      4000

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
  grub_uint8_t uuid[40];
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

typedef enum
{
  GRUB_LUKS_MODE_ECB,
  GRUB_LUKS_MODE_CBC_PLAIN,
  GRUB_LUKS_MODE_CBC_ESSIV
}
luks_mode_t;

struct grub_luks
{
  char *devname, *source;
  grub_uint32_t offset;
  grub_disk_t source_disk;
  int ref;
  grub_crypto_cipher_handle_t cipher;
  grub_crypto_cipher_handle_t essiv_cipher;
  luks_mode_t mode;
  int id;
  struct grub_luks *next;
};
typedef struct grub_luks *grub_luks_t;

static grub_luks_t luks_list = NULL;
static grub_uint8_t n = 0;

gcry_err_code_t AF_merge (const gcry_md_spec_t * hash, grub_uint8_t * src,
			  grub_uint8_t * dst, grub_size_t blocksize,
			  grub_size_t blocknumbers);

static const struct grub_arg_option options[] =
  {
    {"uuid", 'u', 0, N_("Mount by UUID."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static gcry_err_code_t
luks_decrypt (grub_crypto_cipher_handle_t cipher, luks_mode_t mode,
	      grub_uint8_t * data, grub_size_t len,
	      grub_size_t sector, grub_crypto_cipher_handle_t essiv_cipher)
{
  grub_size_t i;
  gcry_err_code_t err;

  switch (mode)
    {
    case GRUB_LUKS_MODE_ECB:
      return grub_crypto_ecb_decrypt (cipher, data, data, len);

    case GRUB_LUKS_MODE_CBC_PLAIN:
      for (i = 0; i < len; i += GRUB_DISK_SECTOR_SIZE)
	{
	  grub_uint32_t iv[(cipher->cipher->blocksize
			    + sizeof (grub_uint32_t) - 1)
			   / sizeof (grub_uint32_t)];
	  grub_memset (iv, 0, cipher->cipher->blocksize);
	  iv[0] = grub_cpu_to_le32 (sector & 0xFFFFFFFF);
	  err = grub_crypto_cbc_decrypt (cipher, data + i, data + i,
					 GRUB_DISK_SECTOR_SIZE, iv);
	  if (err)
	    return err;
	  sector++;
	}
      return GPG_ERR_NO_ERROR;

    case GRUB_LUKS_MODE_CBC_ESSIV:
      for (i = 0; i < len; i += GRUB_DISK_SECTOR_SIZE)
	{
	  grub_uint32_t iv[(cipher->cipher->blocksize
			    + sizeof (grub_uint32_t) - 1)
			   / sizeof (grub_uint32_t)];
	  grub_memset (iv, 0, cipher->cipher->blocksize);
	  iv[0] = grub_cpu_to_le32 (sector & 0xFFFFFFFF);
	  err =
	    grub_crypto_ecb_encrypt (essiv_cipher, iv, iv,
				     cipher->cipher->blocksize);
	  if (err)
	    return err;
	  err = grub_crypto_cbc_decrypt (cipher, data + i, data + i,
					 GRUB_DISK_SECTOR_SIZE, iv);
	  if (err)
	    return err;
	  sector++;
	}
      return GPG_ERR_NO_ERROR;

    default:
      return GPG_ERR_NOT_IMPLEMENTED;
    }
}

static int check_uuid, have_it;
static char *uuid;

static grub_err_t
grub_luks_scan_device_real (const char *name)
{
  grub_disk_t source;
  grub_err_t err;
  grub_luks_t newdev;
  struct grub_luks_phdr header;
  grub_crypto_cipher_handle_t cipher = NULL, essiv_cipher = NULL;
  const gcry_md_spec_t *hash = NULL, *essiv_hash = NULL;
  grub_size_t keysize;
  /* GCC thinks we may use this variable uninitialised. Silence the warning.  */
  grub_size_t essiv_keysize = 0;
  grub_uint8_t *hashed_key = NULL;
  luks_mode_t mode;
  grub_uint8_t *split_key = NULL;
  unsigned i;
  grub_size_t length;
  const struct gcry_cipher_spec *ciph;
  char passphrase[MAX_PASSPHRASE] = "";
  grub_uint8_t candidate_digest[sizeof (header.mkDigest)];

  /* Try to open disk.  */
  source = grub_disk_open (name);
  if (!source)
    return grub_errno;

  /* Read the LUKS header.  */
  err = grub_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    {
      grub_disk_close (source);
      return err;
    }

  /* Look for LUKS magic sequence.  */
  if (grub_memcmp (header.magic, LUKS_MAGIC, sizeof (header.magic))
      || grub_be_to_cpu16 (header.version) != 1)
    {
      grub_disk_close (source);
      return GRUB_ERR_NONE;
    }

  if (check_uuid && grub_memcmp (header.uuid, uuid,
				 sizeof (header.uuid)) != 0)
    return 0;

  newdev = grub_zalloc (sizeof (struct grub_luks));
  if (!newdev)
    {
      grub_disk_close (source);
      return grub_errno;
    }

  newdev->id = n;
  newdev->devname = grub_xasprintf ("luks%d", n);
  newdev->source = grub_strdup (name);
  n++;

  /* Make sure that strings are null terminated.  */
  header.cipherName[sizeof (header.cipherName) - 1] = 0;
  header.cipherMode[sizeof (header.cipherMode) - 1] = 0;
  header.hashSpec[sizeof (header.hashSpec) - 1] = 0;

  ciph = grub_crypto_lookup_cipher_by_name (header.cipherName);
  if (!ciph)
    {
      grub_disk_close (source);
      return grub_error (GRUB_ERR_FILE_NOT_FOUND, "Cipher %s isn't available",
			 header.cipherName);
    }

  /* Configure the cipher used for the bulk data.  */
  cipher = grub_crypto_cipher_open (ciph);
  if (!cipher)
    {
      grub_disk_close (source);
      grub_free (newdev);
      return grub_errno;
    }

  keysize = grub_be_to_cpu32 (header.keyBytes);
  if (keysize > 1024)
    {
      grub_disk_close (source);
      grub_free (newdev);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid keysize %d",
			 keysize);
    }

  /* Configure the cipher mode.  */
  if (grub_strncmp (header.cipherMode, "ecb", 3) == 0)
    mode = GRUB_LUKS_MODE_ECB;
  else if (grub_strncmp (header.cipherMode, "cbc-plain", 9) == 0
	   || grub_strncmp (header.cipherMode, "plain", 5) == 0)
    mode = GRUB_LUKS_MODE_CBC_PLAIN;
  else if (grub_strncmp (header.cipherMode, "cbc-essiv", 9) == 0)
    {
      mode = GRUB_LUKS_MODE_CBC_ESSIV;
      char *hash_str = header.cipherMode + 10;

      /* Configure the hash and cipher used for ESSIV.  */
      essiv_hash = grub_crypto_lookup_md_by_name (hash_str);
      if (!essiv_hash)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_error (GRUB_ERR_FILE_NOT_FOUND,
			     "Couldn't load %s hash", hash_str);
	}
      essiv_cipher = grub_crypto_cipher_open (ciph);
      if (!cipher)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_errno;
	}

      essiv_keysize = essiv_hash->mdlen;
      hashed_key = grub_malloc (essiv_hash->mdlen);
      if (!hashed_key)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_errno;
	}
    }
  else
    {
      grub_crypto_cipher_close (cipher);
      grub_disk_close (source);
      grub_free (newdev);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "Unknown cipher mode: %s",
			 header.cipherMode);
    }

  /* Configure the hash used for the AF splitter and HMAC.  */
  hash = grub_crypto_lookup_md_by_name (header.hashSpec);
  if (!hash)
    {
      grub_crypto_cipher_close (cipher);
      grub_crypto_cipher_close (essiv_cipher);
      grub_free (hashed_key);
      grub_disk_close (source);
      grub_free (newdev);
      return grub_error (GRUB_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
			 header.hashSpec);
    }

  grub_printf ("Attempting to decrypt master key...\n");

  grub_uint8_t candidate_key[keysize];
  grub_uint8_t digest[keysize];

  split_key = grub_malloc (keysize * LUKS_STRIPES);
  if (!split_key)
    {
      grub_crypto_cipher_close (cipher);
      grub_crypto_cipher_close (essiv_cipher);
      grub_free (hashed_key);
      grub_disk_close (source);
      grub_free (newdev);
      return grub_errno;
    }

  /* Get the passphrase from the user.  */
  grub_printf ("Enter passphrase: ");
  if (!grub_password_get (passphrase, MAX_PASSPHRASE))
    {
      grub_crypto_cipher_close (cipher);
      grub_crypto_cipher_close (essiv_cipher);
      grub_free (hashed_key);
      grub_free (split_key);
      grub_disk_close (source);
      grub_free (newdev);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "Passphrase not supplied");
    }

  /* Try to recover master key from each active keyslot.  */
  for (i = 0; i < ARRAY_SIZE (header.keyblock); i++)
    {
      gcry_err_code_t gcry_err;

      /* Check if keyslot is enabled.  */
      if (grub_be_to_cpu32 (header.keyblock[i].active) != LUKS_KEY_ENABLED)
	continue;

      grub_dprintf ("luks", "Trying keyslot %d\n", i);

      /* Calculate the PBKDF2 of the user supplied passphrase.  */
      gcry_err = grub_crypto_pbkdf2 (hash, (grub_uint8_t *) passphrase,
				     grub_strlen (passphrase),
				     header.keyblock[i].passwordSalt,
				     sizeof (header.
					     keyblock[i].passwordSalt),
				     grub_be_to_cpu32 (header.keyblock[i].
						       passwordIterations),
				     digest, keysize);

      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_dprintf ("luks", "PBKDF2 done\n");

      /* Set the PBKDF2 output as the cipher key.  */
      gcry_err = grub_crypto_cipher_set_key (cipher, digest, keysize);
      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Configure ESSIV if necessary.  */
      if (mode == GRUB_LUKS_MODE_CBC_ESSIV)
	{
	  grub_crypto_hash (essiv_hash, hashed_key, digest, keysize);
	  grub_crypto_cipher_set_key (essiv_cipher, hashed_key,
				      essiv_keysize);
	}

      length =
	grub_be_to_cpu32 (header.keyBytes) *
	grub_be_to_cpu32 (header.keyblock[i].stripes);

      /* Read and decrypt the key material from the disk.  */
      err = grub_disk_read (source,
			    grub_be_to_cpu32 (header.keyblock
					      [i].keyMaterialOffset), 0,
			    length, split_key);
      if (err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return err;
	}

      gcry_err = luks_decrypt (cipher, mode, split_key,
			       length, 0, essiv_cipher);
      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Merge the decrypted key material to get the candidate master key.  */
      gcry_err = AF_merge (hash, split_key, candidate_key, keysize,
			   grub_be_to_cpu32 (header.keyblock[i].stripes));
      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_dprintf ("luks", "candidate key recovered\n");

      /* Calculate the PBKDF2 of the candidate master key.  */
      gcry_err = grub_crypto_pbkdf2 (hash, candidate_key,
				     grub_be_to_cpu32 (header.keyBytes),
				     header.mkDigestSalt,
				     sizeof (header.mkDigestSalt),
				     grub_be_to_cpu32
				     (header.mkDigestIterations),
				     candidate_digest,
				     sizeof (candidate_digest));
      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_disk_close (source);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Compare the calculated PBKDF2 to the digest stored
         in the header to see if it's correct.  */
      if (grub_memcmp (candidate_digest, header.mkDigest,
		       sizeof (header.mkDigest)) != 0)
	continue;

      grub_disk_close (source);

      grub_printf ("Slot %d opened\n", i);

      /* Set the master key.  */
      gcry_err = grub_crypto_cipher_set_key (cipher, candidate_key, keysize);
      if (gcry_err)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_crypto_cipher_close (essiv_cipher);
	  grub_free (hashed_key);
	  grub_free (split_key);
	  grub_free (newdev);
	  return grub_crypto_gcry_error (gcry_err);
	}

      newdev->cipher = cipher;

      /* Configure ESSIV if necessary.  */
      if (mode == GRUB_LUKS_MODE_CBC_ESSIV)
	{
	  grub_crypto_hash (essiv_hash, hashed_key, candidate_key, keysize);
	  gcry_err =
	    grub_crypto_cipher_set_key (essiv_cipher, hashed_key,
					essiv_keysize);
	  if (gcry_err)
	    {
	      grub_crypto_cipher_close (cipher);
	      grub_crypto_cipher_close (essiv_cipher);
	      grub_free (hashed_key);
	      grub_free (split_key);
	      grub_free (newdev);
	      return grub_crypto_gcry_error (gcry_err);
	    }
	  newdev->essiv_cipher = essiv_cipher;
	}
      else
	{
	  newdev->essiv_cipher = NULL;
	}

      newdev->offset = grub_be_to_cpu32 (header.payloadOffset);
      newdev->source_disk = NULL;
      newdev->mode = mode;

      grub_free (split_key);
      grub_free (hashed_key);
      newdev->next = luks_list;
      luks_list = newdev;

      have_it = 1;

      return GRUB_ERR_NONE;
    }

  return GRUB_ACCESS_DENIED;
}

static int
grub_luks_scan_device (const char *name)
{
  grub_err_t err;
  err = grub_luks_scan_device_real (name);
  if (err)
    grub_print_error ();
  return have_it && check_uuid ? 0 : 1;
}

static int
grub_luks_iterate (int (*hook) (const char *name))
{
  grub_luks_t i;

  for (i = luks_list; i != NULL; i = i->next)
    if (hook (i->devname))
      return 1;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_luks_open (const char *name, grub_disk_t disk)
{
  grub_luks_t dev;

  /* Search for requested device in the list of LUKS devices.  */
  for (dev = luks_list; dev != NULL; dev = dev->next)
    if (grub_strcmp (dev->devname, name) == 0)
      break;

  if (!dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");

  if (!dev->source_disk)
    {
      grub_dprintf ("luks", "Opening device %s\n", name);
      /* Try to open the source disk and populate the requested disk.  */
      dev->source_disk = grub_disk_open (dev->source);
      if (!dev->source_disk)
	return grub_errno;
    }

  disk->data = dev;
  disk->total_sectors = grub_disk_get_size (dev->source_disk) - dev->offset;
  disk->id = dev->id;
  dev->ref++;
  return GRUB_ERR_NONE;
}

static void
grub_luks_close (grub_disk_t disk)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  grub_dprintf ("luks", "Closing disk\n");

  dev->ref--;

  if (dev->ref == 0)
    {
      grub_disk_close (dev->source_disk);
      dev->source_disk = NULL;
    }
}

static grub_err_t
grub_luks_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  grub_err_t err;
  grub_dprintf ("luks",
		"Reading %" PRIuGRUB_SIZE " sectors from sector 0x%"
		PRIxGRUB_UINT64_T " with offset of %" PRIuGRUB_UINT32_T "\n",
		size, sector, dev->offset);

  err = grub_disk_read (dev->source_disk, sector + dev->offset, 0,
			size << GRUB_DISK_SECTOR_BITS, buf);
  if (err)
    {
      grub_dprintf ("luks", "grub_disk_read failed with error %d\n", err);
      return err;
    }
  return grub_crypto_gcry_error (luks_decrypt (dev->cipher,
					       dev->mode,
					       (grub_uint8_t *) buf,
					       size << GRUB_DISK_SECTOR_BITS,
					       sector, dev->essiv_cipher));
}

static grub_err_t
grub_luks_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static void
luks_cleanup (void)
{
  grub_luks_t dev = luks_list;
  grub_luks_t tmp;

  while (dev != NULL)
    {
      grub_free (dev->devname);
      grub_free (dev->source);
      grub_free (dev->cipher);
      grub_free (dev->essiv_cipher);
      tmp = dev->next;
      grub_free (dev);
      dev = tmp;
    }
}

static grub_err_t
grub_cmd_luksmount (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");

  have_it = 0;
  if (state[0].set)
    {
      check_uuid = 1;
      uuid = args[0];
      grub_device_iterate (&grub_luks_scan_device);
      uuid = NULL;
    }
  else
    {
      grub_err_t err;
      check_uuid = 0;
      uuid = NULL;
      err = grub_luks_scan_device_real (args[0]);
      return err;
    }
  if (!have_it)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such luks found");
  return 0;
}

static struct grub_disk_dev grub_luks_dev = {
  .name = "luks",
  .id = GRUB_DISK_DEVICE_LUKS_ID,
  .iterate = grub_luks_iterate,
  .open = grub_luks_open,
  .close = grub_luks_close,
  .read = grub_luks_read,
  .write = grub_luks_write,
  .next = 0
};

static grub_extcmd_t cmd;

GRUB_MOD_INIT (luks)
{
  cmd = grub_register_extcmd ("luksmount", grub_cmd_luksmount, 0,
			      N_("SOURCE|-u UUID"),
			      N_("Mount a LUKS device."), options);
  grub_disk_dev_register (&grub_luks_dev);
}

GRUB_MOD_FINI (luks)
{
  grub_unregister_extcmd (cmd);
  grub_disk_dev_unregister (&grub_luks_dev);
  luks_cleanup ();
}
