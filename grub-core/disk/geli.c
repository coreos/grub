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

/* This file is loosely based on FreeBSD geli implementation
   (but no code was directly copied). FreeBSD geli is distributed under
   following terms:  */
/*-
 * Copyright (c) 2005-2006 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

struct grub_geli_key
{
  grub_uint8_t iv_key[64];
  grub_uint8_t cipher_key[64];
  grub_uint8_t hmac[64];
} __attribute__ ((packed));

struct grub_geli_phdr
{
  grub_uint8_t magic[16];
#define GELI_MAGIC "GEOM::ELI"
  grub_uint32_t version;
  grub_uint32_t unused1;
  grub_uint16_t alg;
  grub_uint16_t keylen;
  grub_uint16_t unused3[5];
  grub_uint32_t sector_size;
  grub_uint8_t keys_used;
  grub_uint32_t niter;
  grub_uint8_t salt[64];
  struct grub_geli_key keys[2];
} __attribute__ ((packed));

/* FIXME: support big-endian pre-version-4 volumes.  */
/* FIXME: support for keyfiles.  */
/* FIXME: support for HMAC.  */
/* FIXME: support for UUID.  */
/* FIXME: support for mounting all boot volumes.  */
const char *algorithms[] = {
  [0x01] = "des",
  [0x02] = "3des",
  [0x03] = "blowfish",
  [0x04] = "cast5",
  /* FIXME: 0x05 is skipjack, but we don't have it.  */
  [0x0b] = "aes",
  /* FIXME: 0x10 is null.  */
  [0x15] = "camellia128",
};

#define MAX_PASSPHRASE 256

static const struct grub_arg_option options[] =
  {
    {"uuid", 'u', 0, N_("Mount by UUID."), 0, 0},
    {"all", 'a', 0, N_("Mount all."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static int check_uuid, have_it;
static char *search_uuid;

static gcry_err_code_t
geli_rekey (struct grub_cryptodisk *dev, grub_uint64_t zoneno)
{
  gcry_err_code_t gcry_err;
  const struct {
    char magic[4];
    grub_uint64_t zone;
  } __attribute__ ((packed)) tohash
      = { {'e', 'k', 'e', 'y'}, grub_cpu_to_le64 (zoneno) };
  grub_uint64_t key[(dev->hash->mdlen + 7) / 8];

  grub_dprintf ("geli", "rekeying %" PRIuGRUB_UINT64_T " keysize=%d\n",
		zoneno, dev->rekey_derived_size);
  gcry_err = grub_crypto_hmac_buffer (dev->hash, dev->rekey_key, 64,
				      &tohash, sizeof (tohash), key);
  if (gcry_err)
    return grub_crypto_gcry_error (gcry_err);

  return grub_cryptodisk_setkey (dev, (grub_uint8_t *) key,
				 dev->rekey_derived_size); 
}

static grub_cryptodisk_t
configure_ciphers (const struct grub_geli_phdr *header)
{
  grub_cryptodisk_t newdev;
  grub_crypto_cipher_handle_t cipher = NULL;
  const struct gcry_cipher_spec *ciph;
  const char *ciphername = NULL;
  const gcry_md_spec_t *hash = NULL, *iv_hash = NULL;

  /* Look for GELI magic sequence.  */
  if (grub_memcmp (header->magic, GELI_MAGIC, sizeof (GELI_MAGIC))
      || grub_le_to_cpu32 (header->version) > 5
      || grub_le_to_cpu32 (header->version) < 1)
    {
      grub_dprintf ("geli", "wrong magic %02x\n", header->magic[0]);
      return NULL;
    }
  if ((grub_le_to_cpu32 (header->sector_size)
       & (grub_le_to_cpu32 (header->sector_size) - 1))
      || grub_le_to_cpu32 (header->sector_size) == 0)
    {
      grub_dprintf ("geli", "incorrect sector size %d\n",
		    grub_le_to_cpu32 (header->sector_size));
      return NULL;
    }     

#if 0
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
#endif

  if (grub_le_to_cpu16 (header->alg) >= ARRAY_SIZE (algorithms)
      || algorithms[grub_le_to_cpu16 (header->alg)] == NULL)
    {
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Cipher 0x%x unknown",
		  grub_le_to_cpu16 (header->alg));
      return NULL;
    }

  ciphername = algorithms[grub_le_to_cpu16 (header->alg)];
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

  if (grub_le_to_cpu16 (header->keylen) > 1024)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid keysize %d",
		  grub_le_to_cpu16 (header->keylen));
      return NULL;
    }

  hash = grub_crypto_lookup_md_by_name ("sha512");
  if (!hash)
    {
      grub_crypto_cipher_close (cipher);
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
		  "sha512");
      return NULL;
    }

  iv_hash = grub_crypto_lookup_md_by_name ("sha256");
  if (!hash)
    {
      grub_crypto_cipher_close (cipher);
      grub_error (GRUB_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
		  "sha512");
      return NULL;
    }

  newdev = grub_zalloc (sizeof (struct grub_cryptodisk));
  if (!newdev)
    return NULL;
  newdev->cipher = cipher;
  newdev->offset = 0;
  newdev->source_disk = NULL;
  newdev->benbi_log = 0;
  newdev->mode = GRUB_CRYPTODISK_MODE_CBC;
  newdev->mode_iv = GRUB_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH;
  newdev->secondary_cipher = NULL;
  newdev->essiv_cipher = NULL;
  newdev->essiv_hash = NULL;
  newdev->hash = hash;
  newdev->iv_hash = iv_hash;

  for (newdev->log_sector_size = 0;
       (1U << newdev->log_sector_size) < grub_le_to_cpu32 (header->sector_size);
       newdev->log_sector_size++);

  if (grub_le_to_cpu32 (header->version) >= 5)
    {
      newdev->rekey = geli_rekey;
      newdev->rekey_shift = 20;
    }
#if 0
  grub_memcpy (newdev->uuid, uuid, sizeof (newdev->uuid));
#endif
  return newdev;
}

static grub_err_t
recover_key (grub_cryptodisk_t dev, const struct grub_geli_phdr *header,
	     const char *name, grub_disk_t source __attribute__ ((unused)))
{
  grub_size_t keysize = grub_le_to_cpu16 (header->keylen) / 8;
  grub_uint8_t digest[dev->hash->mdlen];
  grub_uint8_t geomkey[dev->hash->mdlen];
  grub_uint8_t verify_key[dev->hash->mdlen];
  grub_uint8_t zero[dev->cipher->cipher->blocksize];
  char passphrase[MAX_PASSPHRASE] = "";
  unsigned i;
  gcry_err_code_t gcry_err;

  grub_memset (zero, 0, sizeof (zero));

  grub_printf ("Attempting to decrypt master key...\n");

  /* Get the passphrase from the user.  */
  grub_printf ("Enter passphrase for %s (%s): ", name, dev->uuid);
  if (!grub_password_get (passphrase, MAX_PASSPHRASE))
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Passphrase not supplied");

  /* Calculate the PBKDF2 of the user supplied passphrase.  */
  if (grub_le_to_cpu32 (header->niter) != 0)
    {
      grub_uint8_t pbkdf_key[64];
      gcry_err = grub_crypto_pbkdf2 (dev->hash, (grub_uint8_t *) passphrase,
				     grub_strlen (passphrase),
				     header->salt,
				     sizeof (header->salt),
				     grub_le_to_cpu32 (header->niter),
				     pbkdf_key, sizeof (pbkdf_key));

      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);

      gcry_err = grub_crypto_hmac_buffer (dev->hash, NULL, 0, pbkdf_key,
					  sizeof (pbkdf_key), geomkey);
      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);
    }
  else
    {
      struct grub_crypto_hmac_handle *hnd;

      hnd = grub_crypto_hmac_init (dev->hash, NULL, 0);
      if (!hnd)
	return grub_crypto_gcry_error (GPG_ERR_OUT_OF_MEMORY);

      grub_crypto_hmac_write (hnd, header->salt, sizeof (header->salt));
      grub_crypto_hmac_write (hnd, passphrase, grub_strlen (passphrase));

      gcry_err = grub_crypto_hmac_fini (hnd, geomkey);
      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);
    }

  gcry_err = grub_crypto_hmac_buffer (dev->hash, geomkey,
				      sizeof (geomkey), "\1", 1, digest);
  if (gcry_err)
    return grub_crypto_gcry_error (gcry_err);

  gcry_err = grub_crypto_hmac_buffer (dev->hash, geomkey,
				      sizeof (geomkey), "\0", 1, verify_key);
  if (gcry_err)
    return grub_crypto_gcry_error (gcry_err);

  grub_dprintf ("geli", "keylen = %" PRIuGRUB_SIZE "\n", keysize);

  /* Try to recover master key from each active keyslot.  */
  for (i = 0; i < ARRAY_SIZE (header->keys); i++)
    {
      struct grub_geli_key candidate_key;
      grub_uint8_t key_hmac[dev->hash->mdlen];

      /* Check if keyslot is enabled.  */
      if (! (header->keys_used & (1 << i)))
	  continue;

      grub_dprintf ("geli", "Trying keyslot %d\n", i);

      gcry_err = grub_crypto_cipher_set_key (dev->cipher,
					     digest, keysize);
      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);

      gcry_err = grub_crypto_cbc_decrypt (dev->cipher, &candidate_key,
					  &header->keys[i],
					  sizeof (candidate_key),
					  zero);
      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);

      gcry_err = grub_crypto_hmac_buffer (dev->hash, verify_key,
					  sizeof (verify_key), 
					  &candidate_key,
					  (sizeof (candidate_key)
					   - sizeof (candidate_key.hmac)),
					  key_hmac);
      if (gcry_err)
	return grub_crypto_gcry_error (gcry_err);

      if (grub_memcmp (candidate_key.hmac, key_hmac, dev->hash->mdlen) != 0)
	continue;
      grub_printf ("Slot %d opened\n", i);

      /* Set the master key.  */
      if (!dev->rekey)
	{
	  gcry_err = grub_cryptodisk_setkey (dev, candidate_key.cipher_key,
					     keysize); 
	  if (gcry_err)
	    return grub_crypto_gcry_error (gcry_err);
	}
      else
	{
	  /* For a reason I don't know, the IV key is used in rekeying.  */
	  grub_memcpy (dev->rekey_key, candidate_key.iv_key,
		       sizeof (candidate_key.iv_key));
	  dev->rekey_derived_size = keysize;
	  dev->last_rekey = -1;
	  COMPILE_TIME_ASSERT (sizeof (dev->rekey_key)
			       >= sizeof (candidate_key.iv_key));
	}

      dev->iv_prefix_len = sizeof (candidate_key.iv_key);
      grub_memcpy (dev->iv_prefix, candidate_key.iv_key,
		   sizeof (candidate_key.iv_key));

      COMPILE_TIME_ASSERT (sizeof (dev->iv_prefix) >= sizeof (candidate_key.iv_key));

      return GRUB_ERR_NONE;
    }

  return GRUB_ACCESS_DENIED;
}

static void
close (grub_cryptodisk_t luks)
{
  grub_crypto_cipher_close (luks->cipher);
  grub_crypto_cipher_close (luks->secondary_cipher);
  grub_crypto_cipher_close (luks->essiv_cipher);
  grub_free (luks);
}

static grub_err_t
grub_geli_scan_device_real (const char *name, grub_disk_t source)
{
  grub_err_t err;
  struct grub_geli_phdr header;
  grub_cryptodisk_t newdev, dev;
  grub_disk_addr_t sector;

  grub_dprintf ("geli", "scanning %s\n", source->name);
  dev = grub_cryptodisk_get_by_source_disk (source);

  if (dev)
    return GRUB_ERR_NONE;

  sector = grub_disk_get_size (source);
  if (sector == GRUB_DISK_SIZE_UNKNOWN)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "not a geli");

  /* Read the LUKS header.  */
  err = grub_disk_read (source, sector - 1, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  if (!newdev)
    return grub_errno;

  newdev->total_length = grub_disk_get_size (source) - 1;

  err = recover_key (newdev, &header, name, source);
  if (err)
    {
      close (newdev);
      return err;
    }

  grub_cryptodisk_insert (newdev, name, source);

  have_it = 1;

  return GRUB_ERR_NONE;
}

#ifdef GRUB_UTIL
grub_err_t
grub_geli_cheat_mount (const char *sourcedev, const char *cheat)
{
  grub_err_t err;
  struct grub_geli_phdr header;
  grub_cryptodisk_t newdev, dev;
  grub_disk_t source;
  grub_disk_addr_t sector;

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

  sector = grub_disk_get_size (source);
  if (sector == GRUB_DISK_SIZE_UNKNOWN)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "not a geli");

  /* Read the LUKS header.  */
  err = grub_disk_read (source, sector - 1, 0, sizeof (header), &header);
  if (err)
    return err;

  newdev = configure_ciphers (&header);
  if (!newdev)
    {
      grub_disk_close (source);
      return grub_errno;
    }

  newdev->total_length = grub_disk_get_size (source) - 1;

  err = grub_cryptodisk_cheat_insert (newdev, sourcedev, source, cheat);
  grub_disk_close (source);
  if (err)
    grub_free (newdev);

  return err;
}
#endif

static int
grub_geli_scan_device (const char *name)
{
  grub_err_t err;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (name);
  if (!source)
    return grub_errno;

  err = grub_geli_scan_device_real (name, source);

  grub_disk_close (source);
  
  if (err)
    grub_print_error ();
  return have_it && check_uuid ? 0 : 1;
}

#ifdef GRUB_UTIL

void
grub_util_geli_print_uuid (grub_disk_t disk)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  grub_printf ("%s ", dev->uuid);
}
#endif

static grub_err_t
grub_cmd_gelimount (grub_extcmd_context_t ctxt, int argc, char **args)
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
      grub_device_iterate (&grub_geli_scan_device);
      search_uuid = NULL;

      if (!have_it)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such luks found");
      return GRUB_ERR_NONE;
    }
  else if (state[1].set)
    {
      check_uuid = 0;
      search_uuid = NULL;
      grub_device_iterate (&grub_geli_scan_device);
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

      err = grub_geli_scan_device_real (args[0], disk);

      grub_disk_close (disk);

      return err;
    }
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (geli)
{
  cmd = grub_register_extcmd ("gelimount", grub_cmd_gelimount, 0,
			      N_("SOURCE|-u UUID|-a"),
			      N_("Mount a GELI device."), options);
}

GRUB_MOD_FINI (geli)
{
  grub_unregister_extcmd (cmd);
}
