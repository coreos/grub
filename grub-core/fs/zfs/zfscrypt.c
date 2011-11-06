/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/zfs/zfs.h>
#include <grub/zfs/zio.h>
#include <grub/zfs/dnode.h>
#include <grub/zfs/uberblock_impl.h>
#include <grub/zfs/vdev_impl.h>
#include <grub/zfs/zio_checksum.h>
#include <grub/zfs/zap_impl.h>
#include <grub/zfs/zap_leaf.h>
#include <grub/zfs/zfs_znode.h>
#include <grub/zfs/dmu.h>
#include <grub/zfs/dmu_objset.h>
#include <grub/zfs/sa_impl.h>
#include <grub/zfs/dsl_dir.h>
#include <grub/zfs/dsl_dataset.h>
#include <grub/crypto.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

enum grub_zfs_algo
  {
    GRUB_ZFS_ALGO_CCM,
    GRUB_ZFS_ALGO_GCM,
  };

struct grub_zfs_key
{
  grub_uint64_t algo;
  grub_uint8_t enc_nonce[13];
  grub_uint8_t unused[3];
  grub_uint8_t enc_key[48];
  grub_uint8_t unknown_purpose_nonce[13];
  grub_uint8_t unused2[3];
  grub_uint8_t unknown_purpose_key[48];
};

struct grub_zfs_wrap_key
{
  struct grub_zfs_wrap_key *next;
  grub_uint64_t key[GRUB_ZFS_MAX_KEYLEN / 8];
};

static struct grub_zfs_wrap_key *zfs_wrap_keys;

grub_err_t
grub_zfs_add_key (grub_uint8_t *key_in)
{
  struct grub_zfs_wrap_key *key;
  key = grub_malloc (sizeof (*key));
  if (!key)
    return grub_errno;
  grub_memcpy (key->key, key_in, GRUB_ZFS_MAX_KEYLEN);
  key->next = zfs_wrap_keys;
  zfs_wrap_keys = key;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ccm_decrypt (grub_crypto_cipher_handle_t cipher,
		  grub_uint8_t *out, const grub_uint8_t *in,
		  grub_size_t psize,
		  void *mac_out, const void *nonce,
		  unsigned l, unsigned m)
{
  grub_uint8_t iv[16];
  grub_uint8_t mul[16];
  grub_uint32_t mac[4];
  unsigned i, j;
  grub_err_t err;

  grub_memcpy (iv + 1, nonce, 15 - l);

  iv[0] = (l - 1) | (((m-2) / 2) << 3);
  for (j = 0; j < l; j++)
    iv[15 - j] = psize >> (8 * j);
  err = grub_crypto_ecb_encrypt (cipher, mac, iv, 16);
  if (err)
    return err;

  iv[0] = l - 1;

  for (i = 0; i < (psize + 15) / 16; i++)
    {
      grub_size_t csize;
      csize = 16;
      if (csize > psize - 16 * i)
	csize = psize - 16 * i;
      for (j = 0; j < l; j++)
	iv[15 - j] = (i + 1) >> (8 * j);
      err = grub_crypto_ecb_encrypt (cipher, mul, iv, 16);
      if (err)
	return err;
      grub_crypto_xor (out + 16 * i, in + 16 * i, mul, csize);
      grub_crypto_xor (mac, mac, out + 16 * i, csize);
      err = grub_crypto_ecb_encrypt (cipher, mac, mac, 16);
      if (err)
	return err;
    }
  for (j = 0; j < l; j++)
    iv[15 - j] = 0;
  err = grub_crypto_ecb_encrypt (cipher, mul, iv, 16);
  if (err)
    return err;
  if (mac_out)
    grub_crypto_xor (mac_out, mac, mul, m);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_zfs_decrypt_real (grub_crypto_cipher_handle_t cipher, void *nonce,
		       char *buf, grub_size_t size,
		       const grub_uint32_t *expected_mac,
		       grub_zfs_endian_t endian)
{
  grub_uint32_t mac[4];
  unsigned i;
  grub_uint32_t sw[4];
  grub_err_t err;
      
  grub_memcpy (sw, nonce, 16);
  for (i = 0; i < 4; i++)
    sw[i] = grub_cpu_to_be32 (grub_zfs_to_cpu32 (sw[i], endian));

  if (!cipher)
    return grub_error (GRUB_ERR_ACCESS_DENIED,
		       "no decryption key available");;
  err = grub_ccm_decrypt (cipher,
			  (grub_uint8_t *) buf,
			  (grub_uint8_t *) buf,
			  size, mac,
			  sw + 1, 3, 12);
  if (err)
    return err;
  
  for (i = 0; i < 3; i++)
    if (grub_zfs_to_cpu32 (expected_mac[i], endian)
	!= grub_be_to_cpu32 (mac[i]))
      return grub_error (GRUB_ERR_BAD_FS, "MAC verification failed");
  return GRUB_ERR_NONE;
}

static grub_crypto_cipher_handle_t
grub_zfs_load_key_real (const struct grub_zfs_key *key,
			grub_size_t keysize)
{
  unsigned keylen;
  struct grub_zfs_wrap_key *wrap_key;
  grub_crypto_cipher_handle_t ret = NULL;
  grub_err_t err;

  if (keysize != sizeof (*key))
    {
      grub_dprintf ("zfs", "Unexpected key size %" PRIuGRUB_SIZE "\n", keysize);
      return 0;
    }

  if (grub_memcmp (key->enc_key + 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16)
      == 0)
    keylen = 16;
  else if (grub_memcmp (key->enc_key + 40, "\0\0\0\0\0\0\0\0", 8) == 0)
    keylen = 24;
  else
    keylen = 32;

  for (wrap_key = zfs_wrap_keys; wrap_key; wrap_key = wrap_key->next)
    {
      grub_crypto_cipher_handle_t cipher;
      grub_uint8_t decrypted[32], mac[32];
      cipher = grub_crypto_cipher_open (GRUB_CIPHER_AES);
      if (!cipher)
	{
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}
      err = grub_crypto_cipher_set_key (cipher,
					(const grub_uint8_t *) wrap_key->key,
					keylen);
      if (err)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      
      err = grub_ccm_decrypt (cipher, decrypted, key->unknown_purpose_key, 32,
			      mac, key->unknown_purpose_nonce, 2, 16);
      if (err || (grub_crypto_memcmp (mac, key->unknown_purpose_key + 32, 16)
		  != 0))
	{
	  grub_dprintf ("zfs", "key loading failed\n");
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}

      err = grub_ccm_decrypt (cipher, decrypted, key->enc_key, keylen, mac,
			      key->enc_nonce, 2, 16);
      if (err || grub_crypto_memcmp (mac, key->enc_key + keylen, 16) != 0)
	{
	  grub_dprintf ("zfs", "key loading failed\n");
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      ret = grub_crypto_cipher_open (GRUB_CIPHER_AES);
      if (!ret)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      err = grub_crypto_cipher_set_key (ret, decrypted, keylen);
      if (err)
	{
	    grub_errno = GRUB_ERR_NONE;
	    grub_crypto_cipher_close (ret);
	    continue;
	  }
      return ret;
    }
  return NULL;
}

static const struct grub_arg_option options[] =
  {
    {"raw", 'r', 0, N_("Assume input is raw."), 0, 0},
    {"hex", 'h', 0, N_("Assume input is hex."), 0, 0},
    {"passphrase", 'p', 0, N_("Assume input is passphrase."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_zfs_key (grub_extcmd_context_t ctxt, int argc, char **args)
{
  grub_uint8_t buf[1024];
  grub_ssize_t real_size;

  if (argc > 0)
    {
      grub_file_t file;
      file = grub_file_open (args[0]);
      if (!file)
	return grub_errno;
      real_size = grub_file_read (file, buf, 1024);
      if (real_size < 0)
	return grub_errno;
    }
  if (ctxt->state[0].set
      || (argc > 0 && !ctxt->state[1].set && !ctxt->state[2].set))
    {
      grub_err_t err;
      if (real_size < GRUB_ZFS_MAX_KEYLEN)
	grub_memset (buf + real_size, 0, GRUB_ZFS_MAX_KEYLEN - real_size);
      err = grub_zfs_add_key (buf);
      if (err)
	return err;
      return GRUB_ERR_NONE;
    }

  if (ctxt->state[1].set)
    {
      int i;
      grub_err_t err;
      if (real_size < 2 * GRUB_ZFS_MAX_KEYLEN)
	grub_memset (buf + real_size, '0', 2 * GRUB_ZFS_MAX_KEYLEN - real_size);
      for (i = 0; i < GRUB_ZFS_MAX_KEYLEN; i++)
	{
	  char c1 = grub_tolower (buf[2 * i]) - '0';
	  char c2 = grub_tolower (buf[2 * i + 1]) - '0';
	  if (c1 > 9)
	    c1 += '0' - 'a' + 10;
	  if (c2 > 9)
	    c2 += '0' - 'a' + 10;
	  buf[i] = (c1 << 4) | c2;
	}
      err = grub_zfs_add_key (buf);
      if (err)
	return err;
      return GRUB_ERR_NONE;
    }
  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd_key;

GRUB_MOD_INIT(zfscrypto)
{
  grub_zfs_decrypt = grub_zfs_decrypt_real;
  grub_zfs_load_key = grub_zfs_load_key_real;
  cmd_key = grub_register_extcmd ("zfskey", grub_cmd_zfs_key, 0,
				  "zfskey [-h|-p|-r] [FILE]",
				  "Import ZFS wrapping key stored in FILE.",
				  options);
}

GRUB_MOD_FINI(zfscrypto)
{
  grub_zfs_decrypt = 0;
  grub_zfs_load_key = 0;
  grub_unregister_extcmd (cmd_key);
}
