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
#include <grub/err.h>
#include <grub/disk.h>
#include <grub/crypto.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#ifdef GRUB_UTIL
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grub/emu/hostdisk.h>
#include <unistd.h>
#include <string.h>
#endif

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

typedef enum
{
  GRUB_LUKS_MODE_ECB,
  GRUB_LUKS_MODE_CBC_PLAIN,
  GRUB_LUKS_MODE_CBC_ESSIV,
  GRUB_LUKS_MODE_XTS
} luks_mode_t;

struct grub_luks
{
  char *source;
  grub_uint32_t offset;
  grub_disk_t source_disk;
  int ref;
  grub_crypto_cipher_handle_t cipher;
  grub_crypto_cipher_handle_t secondary_cipher;
  const gcry_md_spec_t *essiv_hash, *hash;
  luks_mode_t mode;
  unsigned long id, source_id;
  enum grub_disk_dev_id source_dev_id;
  char uuid[sizeof (((struct grub_luks_phdr *) 0)->uuid) + 1];
#ifdef GRUB_UTIL
  char *cheat;
  int cheat_fd;
#endif
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
    {"all", 'a', 0, N_("Mount all."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static inline void
make_iv (grub_uint32_t *iv, grub_size_t sz, grub_disk_addr_t sector)
{
  grub_memset (iv, 0, sz * sizeof (iv[0]));
  iv[0] = grub_cpu_to_le32 (sector & 0xFFFFFFFF);
}

/* Our irreducible polynom is x^128+x^7+x^2+x+1. Lowest byte of it is:  */
#define POLYNOM 0x87

static void
gf_mul_x (grub_uint8_t *g)
{
  int over = 0, over2 = 0;
  int j;

  for (j = 0; j < 16; j++)
    {
      over2 = !!(g[j] & 0x80);
      g[j] <<= 1;
      g[j] |= over;
      over = over2;
    }
  if (over)
    g[0] ^= POLYNOM;
}

static gcry_err_code_t
luks_decrypt (const struct grub_luks *dev,
	      grub_uint8_t * data, grub_size_t len, grub_size_t sector)
{
  grub_size_t i;
  gcry_err_code_t err;

  switch (dev->mode)
    {
    case GRUB_LUKS_MODE_ECB:
      return grub_crypto_ecb_decrypt (dev->cipher, data, data, len);

    case GRUB_LUKS_MODE_CBC_PLAIN:
      for (i = 0; i < len; i += GRUB_DISK_SECTOR_SIZE)
	{
	  grub_size_t sz = ((dev->cipher->cipher->blocksize
			    + sizeof (grub_uint32_t) - 1)
			    / sizeof (grub_uint32_t));
	  grub_uint32_t iv[sz];
	  make_iv (iv, sz, sector);
	  err = grub_crypto_cbc_decrypt (dev->cipher, data + i, data + i,
					 GRUB_DISK_SECTOR_SIZE, iv);
	  if (err)
	    return err;
	  sector++;
	}
      return GPG_ERR_NO_ERROR;

    case GRUB_LUKS_MODE_CBC_ESSIV:
      for (i = 0; i < len; i += GRUB_DISK_SECTOR_SIZE)
	{
	  grub_size_t sz = ((dev->cipher->cipher->blocksize
			    + sizeof (grub_uint32_t) - 1)
			    / sizeof (grub_uint32_t));
	  grub_uint32_t iv[sz];
	  make_iv (iv, sz, sector);

	  err = grub_crypto_ecb_encrypt (dev->secondary_cipher, iv, iv,
					 dev->cipher->cipher->blocksize);
	  if (err)
	    return err;
	  err = grub_crypto_cbc_decrypt (dev->cipher, data + i, data + i,
					 GRUB_DISK_SECTOR_SIZE, iv);
	  if (err)
	    return err;
	  sector++;
	}
      return GPG_ERR_NO_ERROR;

    case GRUB_LUKS_MODE_XTS:
      for (i = 0; i < len; i += GRUB_DISK_SECTOR_SIZE)
	{
	  grub_size_t sz = ((dev->cipher->cipher->blocksize
			    + sizeof (grub_uint32_t) - 1)
			    / sizeof (grub_uint32_t));
	  grub_uint32_t iv[sz];
	  int j;
	  make_iv (iv, sz, sector);

	  err = grub_crypto_ecb_encrypt (dev->secondary_cipher, iv, iv,
					 dev->cipher->cipher->blocksize);
	  if (err)
	    return err;

	  for (j = 0; j < GRUB_DISK_SECTOR_SIZE;
	       j += dev->cipher->cipher->blocksize)
	    {
	      grub_crypto_xor (data + i + j, data + i + j, iv,
			       dev->cipher->cipher->blocksize);
	      err = grub_crypto_ecb_decrypt (dev->cipher, data + i + j, 
					     data + i + j,
					     dev->cipher->cipher->blocksize);
	      if (err)
		return err;
	      grub_crypto_xor (data + i + j, data + i + j, iv,
			       dev->cipher->cipher->blocksize);
	      gf_mul_x ((grub_uint8_t *) iv);
	    }
	  sector++;
	}
      return GPG_ERR_NO_ERROR;

    default:
      return GPG_ERR_NOT_IMPLEMENTED;
    }
}

static int check_uuid, have_it;
static char *search_uuid;

static grub_luks_t
configure_ciphers (const struct grub_luks_phdr *header)
{
  grub_luks_t newdev;
  const char *iptr;
  char *optr;
  char uuid[sizeof (header->uuid) + 1];
  char ciphername[sizeof (header->cipherName) + 1];
  char ciphermode[sizeof (header->cipherMode) + 1];
  char hashspec[sizeof (header->hashSpec) + 1];
  grub_crypto_cipher_handle_t cipher = NULL, secondary_cipher = NULL;
  const gcry_md_spec_t *hash = NULL, *essiv_hash = NULL;
  const struct gcry_cipher_spec *ciph;
  luks_mode_t mode;
  
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
    mode = GRUB_LUKS_MODE_ECB;
  else if (grub_strcmp (ciphermode, "cbc-plain") == 0
	   || grub_strcmp (ciphermode, "plain") == 0)
    mode = GRUB_LUKS_MODE_CBC_PLAIN;
  else if (grub_memcmp (ciphermode, "cbc-essiv:", sizeof ("cbc-essiv:") - 1) 
	   == 0)
    {
      mode = GRUB_LUKS_MODE_CBC_ESSIV;
      char *hash_str = ciphermode + 10;

      /* Configure the hash and cipher used for ESSIV.  */
      essiv_hash = grub_crypto_lookup_md_by_name (hash_str);
      if (!essiv_hash)
	{
	  grub_crypto_cipher_close (cipher);
	  grub_error (GRUB_ERR_FILE_NOT_FOUND,
		      "Couldn't load %s hash", hash_str);
	  return NULL;
	}
      secondary_cipher = grub_crypto_cipher_open (ciph);
      if (!secondary_cipher)
	{
	  grub_crypto_cipher_close (cipher);
	  return NULL;
	}
    }
  else if (grub_strcmp (ciphermode, "xts-plain") == 0)
    {
      mode = GRUB_LUKS_MODE_XTS;
      secondary_cipher = grub_crypto_cipher_open (ciph);
      if (!secondary_cipher)
	{
	  grub_crypto_cipher_close (cipher);
	  return NULL;
	}
      if (cipher->cipher->blocksize != 16)
	{
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      cipher->cipher->blocksize);
	  return NULL;
	}
      if (secondary_cipher->cipher->blocksize != 16)
	{
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      secondary_cipher->cipher->blocksize);
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

  /* Configure the hash used for the AF splitter and HMAC.  */
  hash = grub_crypto_lookup_md_by_name (hashspec);
  if (!hash)
    {
      grub_crypto_cipher_close (cipher);
      grub_crypto_cipher_close (secondary_cipher);
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
		  hashspec);
      return NULL;
    }

  newdev = grub_zalloc (sizeof (struct grub_luks));
  if (!newdev)
    return NULL;
  newdev->cipher = cipher;
  newdev->offset = grub_be_to_cpu32 (header->payloadOffset);
  newdev->source_disk = NULL;
  newdev->mode = mode;
  newdev->secondary_cipher = secondary_cipher;
  newdev->essiv_hash = essiv_hash;
  newdev->hash = hash;
  newdev->id = n++;
  grub_memcpy (newdev->uuid, uuid, sizeof (newdev->uuid));
  return newdev;
}

static grub_err_t
luks_recover_key (grub_luks_t dev, const struct grub_luks_phdr *header,
		  const char *name, grub_disk_t source)
{
  grub_size_t keysize = grub_be_to_cpu32 (header->keyBytes);
  grub_uint8_t candidate_key[keysize];
  grub_uint8_t digest[keysize];
  grub_uint8_t *hashed_key = NULL;
  grub_uint8_t *split_key = NULL;
  char passphrase[MAX_PASSPHRASE] = "";
  grub_uint8_t candidate_digest[sizeof (header->mkDigest)];
  unsigned i;
  grub_size_t essiv_keysize = 0;
  grub_size_t length;
  grub_err_t err;

  if (dev->mode == GRUB_LUKS_MODE_CBC_ESSIV)
    {
      essiv_keysize = dev->essiv_hash->mdlen;
      hashed_key = grub_malloc (dev->essiv_hash->mdlen);
      if (!hashed_key)
	return grub_errno;
    }

  grub_printf ("Attempting to decrypt master key...\n");

  split_key = grub_malloc (keysize * LUKS_STRIPES);
  if (!split_key)
    {
      grub_free (hashed_key);
      return grub_errno;
    }

  /* Get the passphrase from the user.  */
  grub_printf ("Enter passphrase for %s (%s): ", name, dev->uuid);
  if (!grub_password_get (passphrase, MAX_PASSPHRASE))
    {
      grub_free (hashed_key);
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
				     sizeof (header->
					     keyblock[i].passwordSalt),
				     grub_be_to_cpu32 (header->keyblock[i].
						       passwordIterations),
				     digest, keysize);

      if (gcry_err)
	{
	  grub_free (hashed_key);
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      grub_dprintf ("luks", "PBKDF2 done\n");

      /* Set the PBKDF2 output as the cipher key.  */
      gcry_err = grub_crypto_cipher_set_key (dev->cipher, digest,
					     (dev->mode == GRUB_LUKS_MODE_XTS)
					     ? (keysize / 2) : keysize);
      if (gcry_err)
	{
	  grub_free (hashed_key);
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Configure ESSIV if necessary.  */
      if (dev->mode == GRUB_LUKS_MODE_CBC_ESSIV)
	{
	  grub_crypto_hash (dev->essiv_hash, hashed_key, digest, keysize);
	  gcry_err = grub_crypto_cipher_set_key (dev->secondary_cipher,
						 hashed_key,
						 essiv_keysize);
	  if (gcry_err)
	    {
	      grub_free (hashed_key);
	      grub_free (split_key);
	      return grub_crypto_gcry_error (gcry_err);
	    }
	}

      if (dev->mode == GRUB_LUKS_MODE_XTS)
	{
	  gcry_err = grub_crypto_cipher_set_key (dev->secondary_cipher,
						 digest + (keysize / 2),
						 keysize / 2);
	  if (gcry_err)
	    {
	      grub_free (hashed_key);
	      grub_free (split_key);
	      return grub_crypto_gcry_error (gcry_err);
	    }
	}

      length =
	grub_be_to_cpu32 (header->keyBytes) *
	grub_be_to_cpu32 (header->keyblock[i].stripes);

      /* Read and decrypt the key material from the disk.  */
      err = grub_disk_read (source,
			    grub_be_to_cpu32 (header->keyblock
					      [i].keyMaterialOffset), 0,
			    length, split_key);
      if (err)
	{
	  grub_free (hashed_key);
	  grub_free (split_key);
	  return err;
	}

      gcry_err = luks_decrypt (dev, split_key, length, 0);
      if (gcry_err)
	{
	  grub_free (hashed_key);
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Merge the decrypted key material to get the candidate master key.  */
      gcry_err = AF_merge (dev->hash, split_key, candidate_key, keysize,
			   grub_be_to_cpu32 (header->keyblock[i].stripes));
      if (gcry_err)
	{
	  grub_free (hashed_key);
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
	  grub_free (hashed_key);
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
      gcry_err = grub_crypto_cipher_set_key (dev->cipher, candidate_key,
					     (dev->mode == GRUB_LUKS_MODE_XTS)
					     ? (keysize / 2) : keysize);
      if (gcry_err)
	{
	  grub_free (hashed_key);
	  grub_free (split_key);
	  return grub_crypto_gcry_error (gcry_err);
	}

      /* Configure ESSIV if necessary.  */
      if (dev->mode == GRUB_LUKS_MODE_CBC_ESSIV)
	{
	  grub_crypto_hash (dev->essiv_hash, hashed_key,
			    candidate_key, keysize);
	  gcry_err =
	    grub_crypto_cipher_set_key (dev->secondary_cipher, hashed_key,
					essiv_keysize);
	  if (gcry_err)
	    {
	      grub_free (hashed_key);
	      grub_free (split_key);
	      return grub_crypto_gcry_error (gcry_err);
	    }
	}

      if (dev->mode == GRUB_LUKS_MODE_XTS)
	{
	  gcry_err = grub_crypto_cipher_set_key (dev->secondary_cipher,
						 candidate_key + (keysize / 2),
						 keysize / 2);
	  if (gcry_err)
	    {
	      grub_free (hashed_key);
	      grub_free (split_key);
	      return grub_crypto_gcry_error (gcry_err);
	    }
	}

      grub_free (split_key);
      grub_free (hashed_key);

      return GRUB_ERR_NONE;
    }

  return GRUB_ACCESS_DENIED;
}

static void
luks_close (grub_luks_t luks)
{
  grub_crypto_cipher_close (luks->cipher);
  grub_crypto_cipher_close (luks->secondary_cipher);
  grub_free (luks);
}

static grub_err_t
grub_luks_scan_device_real (const char *name, grub_disk_t source)
{
  grub_err_t err;
  struct grub_luks_phdr header;
  grub_luks_t newdev, dev;

  for (dev = luks_list; dev != NULL; dev = dev->next)
    if (dev->source_id == source->id && dev->source_dev_id == source->dev->id)
      return GRUB_ERR_NONE;

  /* Read the LUKS header.  */
  err = grub_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  if (!newdev)
    return grub_errno;

  err = luks_recover_key (newdev, &header, name, source);
  if (err)
    {
      luks_close (newdev);
      return err;
    }

  newdev->source = grub_strdup (name);
  if (!newdev->source)
    {
      grub_free (newdev);
      return grub_errno;
    }

  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->next = luks_list;
  luks_list = newdev;

  have_it = 1;

  return GRUB_ERR_NONE;
}

#ifdef GRUB_UTIL
grub_err_t
grub_luks_cheat_mount (const char *sourcedev, const char *cheat)
{
  grub_err_t err;
  struct grub_luks_phdr header;
  grub_luks_t newdev, dev;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (sourcedev);
  if (!source)
    return grub_errno;

  for (dev = luks_list; dev != NULL; dev = dev->next)
    if (dev->source_id == source->id && dev->source_dev_id == source->dev->id)
      {
	grub_disk_close (source);	
	return GRUB_ERR_NONE;
      }

  /* Read the LUKS header.  */
  err = grub_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  grub_disk_close (source);
  if (!newdev)
    return grub_errno;

  newdev->cheat = grub_strdup (cheat);
  newdev->source = grub_strdup (sourcedev);
  if (!newdev->source || !newdev->cheat)
    {
      grub_free (newdev->source);
      grub_free (newdev->cheat);
      grub_free (newdev);
      return grub_errno;
    }
  newdev->cheat_fd = -1;
  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->next = luks_list;
  luks_list = newdev;
  return GRUB_ERR_NONE;
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

static int
grub_luks_iterate (int (*hook) (const char *name),
		   grub_disk_pull_t pull)
{
  grub_luks_t i;

  if (pull != GRUB_DISK_PULL_NONE)
    return 0;

  for (i = luks_list; i != NULL; i = i->next)
    {
      char buf[30];
      grub_snprintf (buf, sizeof (buf), "luks%lu", i->id);
      if (hook (buf))
	return 1;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_luks_open (const char *name, grub_disk_t disk,
		grub_disk_pull_t pull __attribute__ ((unused)))
{
  grub_luks_t dev;

  if (grub_memcmp (name, "luks", sizeof ("luks") - 1) != 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");

  if (grub_memcmp (name, "luksuuid/", sizeof ("luksuuid/") - 1) == 0)
    {
      for (dev = luks_list; dev != NULL; dev = dev->next)
	if (grub_strcasecmp (name + sizeof ("luksuuid/") - 1, dev->uuid) == 0)
	  break;
    }
  else
    {
      unsigned long id = grub_strtoul (name + sizeof ("luks") - 1, 0, 0);
      if (grub_errno)
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");
      /* Search for requested device in the list of LUKS devices.  */
      for (dev = luks_list; dev != NULL; dev = dev->next)
	if (dev->id == id)
	  break;
    }
  if (!dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");

#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      if (dev->cheat_fd == -1)
	dev->cheat_fd = open (dev->cheat, O_RDONLY);
      if (dev->cheat_fd == -1)
	return grub_error (GRUB_ERR_IO, "couldn't open %s: %s",
			   dev->cheat, strerror (errno));
    }
#endif

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

  if (dev->ref != 0)
    return;
#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      close (dev->cheat_fd);
      dev->cheat_fd = -1;
    }
#endif
  grub_disk_close (dev->source_disk);
  dev->source_disk = NULL;
}

static grub_err_t
grub_luks_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  grub_err_t err;

#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      err = grub_util_fd_sector_seek (dev->cheat_fd, dev->cheat, sector);
      if (err)
	return err;
      if (grub_util_fd_read (dev->cheat_fd, buf, size << GRUB_DISK_SECTOR_BITS)
	  != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
	return grub_error (GRUB_ERR_READ_ERROR, "cannot read from `%s'",
			   dev->cheat);
      return GRUB_ERR_NONE;
    }
#endif

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
  return grub_crypto_gcry_error (luks_decrypt (dev, (grub_uint8_t *) buf,
					       size << GRUB_DISK_SECTOR_BITS,
					       sector));
}

static grub_err_t
grub_luks_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_luks_memberlist (grub_disk_t disk)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  grub_disk_memberlist_t list = NULL;

  list = grub_malloc (sizeof (*list));
  if (list)
    {
      list->disk = dev->source_disk;
      list->next = NULL;
    }

  return list;
}

void
grub_util_luks_print_ciphers (grub_disk_t disk)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  if (dev->cipher)
    grub_printf ("%s ", dev->cipher->cipher->modname);
  if (dev->secondary_cipher)
    grub_printf ("%s ", dev->secondary_cipher->cipher->modname);
  if (dev->hash)
    grub_printf ("%s ", dev->hash->modname);
  if (dev->essiv_hash)
    grub_printf ("%s ", dev->essiv_hash->modname);
}

void
grub_util_luks_print_uuid (grub_disk_t disk)
{
  grub_luks_t dev = (grub_luks_t) disk->data;
  grub_printf ("%s ", dev->uuid);
}
#endif

static void
luks_cleanup (void)
{
  grub_luks_t dev = luks_list;
  grub_luks_t tmp;

  while (dev != NULL)
    {
      grub_free (dev->source);
      grub_free (dev->cipher);
      grub_free (dev->secondary_cipher);
      tmp = dev->next;
      grub_free (dev);
      dev = tmp;
    }
}

static grub_err_t
grub_cmd_luksmount (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;

  if (argc < 1 && !state[1].set)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");

  have_it = 0;
  if (state[0].set)
    {
      grub_luks_t dev;

      for (dev = luks_list; dev != NULL; dev = dev->next)
	if (grub_strcasecmp (dev->uuid, args[0]) == 0)
	  {
	    grub_dprintf ("luks", "already mounted as luks%lu\n", dev->id);
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
      grub_luks_t dev;

      check_uuid = 0;
      search_uuid = NULL;
      disk = grub_disk_open (args[0]);
      if (!disk)
	return grub_errno;

      for (dev = luks_list; dev != NULL; dev = dev->next)
	if (dev->source_id == disk->id && dev->source_dev_id == disk->dev->id)
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

static struct grub_disk_dev grub_luks_dev = {
  .name = "luks",
  .id = GRUB_DISK_DEVICE_LUKS_ID,
  .iterate = grub_luks_iterate,
  .open = grub_luks_open,
  .close = grub_luks_close,
  .read = grub_luks_read,
  .write = grub_luks_write,
#ifdef GRUB_UTIL
  .memberlist = grub_luks_memberlist,
#endif
  .next = 0
};

static grub_extcmd_t cmd;

GRUB_MOD_INIT (luks)
{
  cmd = grub_register_extcmd ("luksmount", grub_cmd_luksmount, 0,
			      N_("SOURCE|-u UUID|-a"),
			      N_("Mount a LUKS device."), options);
  grub_disk_dev_register (&grub_luks_dev);
}

GRUB_MOD_FINI (luks)
{
  grub_unregister_extcmd (cmd);
  grub_disk_dev_unregister (&grub_luks_dev);
  luks_cleanup ();
}
