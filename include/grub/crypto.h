/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006
 *                2007, 2008, 2009  Free Software Foundation, Inc.
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

/* Contains elements based on gcrypt-module.h and gcrypt.h.in.
   If it's changed please update this file.  */

#ifndef GRUB_CIPHER_HEADER
#define GRUB_CIPHER_HEADER 1

#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>

typedef enum 
  {
    GPG_ERR_NO_ERROR,
    GPG_ERR_BAD_MPI,
    GPG_ERR_BAD_SECKEY,
    GPG_ERR_BAD_SIGNATURE,
    GPG_ERR_CIPHER_ALGO,
    GPG_ERR_CONFLICT,
    GPG_ERR_DECRYPT_FAILED,
    GPG_ERR_DIGEST_ALGO,
    GPG_ERR_GENERAL,
    GPG_ERR_INTERNAL,
    GPG_ERR_INV_ARG,
    GPG_ERR_INV_CIPHER_MODE,
    GPG_ERR_INV_FLAG,
    GPG_ERR_INV_KEYLEN,
    GPG_ERR_INV_OBJ,
    GPG_ERR_INV_OP,
    GPG_ERR_INV_SEXP,
    GPG_ERR_INV_VALUE,
    GPG_ERR_MISSING_VALUE,
    GPG_ERR_NO_ENCRYPTION_SCHEME,
    GPG_ERR_NO_OBJ,
    GPG_ERR_NO_PRIME,
    GPG_ERR_NO_SIGNATURE_SCHEME,
    GPG_ERR_NOT_FOUND,
    GPG_ERR_NOT_IMPLEMENTED,
    GPG_ERR_NOT_SUPPORTED,
    GPG_ERROR_CFLAGS,
    GPG_ERR_PUBKEY_ALGO,
    GPG_ERR_SELFTEST_FAILED,
    GPG_ERR_TOO_SHORT,
    GPG_ERR_UNSUPPORTED,
    GPG_ERR_WEAK_KEY,
    GPG_ERR_WRONG_KEY_USAGE,
    GPG_ERR_WRONG_PUBKEY_ALGO,
  } gcry_err_code_t;
#define gpg_err_code_t gcry_err_code_t
#define gpg_error_t gcry_err_code_t

enum gcry_cipher_modes 
  {
    GCRY_CIPHER_MODE_NONE   = 0,  /* Not yet specified. */
    GCRY_CIPHER_MODE_ECB    = 1,  /* Electronic codebook. */
    GCRY_CIPHER_MODE_CFB    = 2,  /* Cipher feedback. */
    GCRY_CIPHER_MODE_CBC    = 3,  /* Cipher block chaining. */
    GCRY_CIPHER_MODE_STREAM = 4,  /* Used with stream ciphers. */
    GCRY_CIPHER_MODE_OFB    = 5,  /* Outer feedback. */
    GCRY_CIPHER_MODE_CTR    = 6   /* Counter. */
  };

/* Type for the cipher_setkey function.  */
typedef gcry_err_code_t (*gcry_cipher_setkey_t) (void *c,
						 const unsigned char *key,
						 unsigned keylen);

/* Type for the cipher_encrypt function.  */
typedef void (*gcry_cipher_encrypt_t) (void *c,
				       unsigned char *outbuf,
				       const unsigned char *inbuf);

/* Type for the cipher_decrypt function.  */
typedef void (*gcry_cipher_decrypt_t) (void *c,
				       unsigned char *outbuf,
				       const unsigned char *inbuf);

/* Type for the cipher_stencrypt function.  */
typedef void (*gcry_cipher_stencrypt_t) (void *c,
					 unsigned char *outbuf,
					 const unsigned char *inbuf,
					 unsigned int n);

/* Type for the cipher_stdecrypt function.  */
typedef void (*gcry_cipher_stdecrypt_t) (void *c,
					 unsigned char *outbuf,
					 const unsigned char *inbuf,
					 unsigned int n);

typedef struct gcry_cipher_oid_spec
{
  const char *oid;
  int mode;
} gcry_cipher_oid_spec_t;

/* Module specification structure for ciphers.  */
typedef struct gcry_cipher_spec
{
  const char *name;
  const char **aliases;
  gcry_cipher_oid_spec_t *oids;
  grub_size_t blocksize;
  grub_size_t keylen;
  grub_size_t contextsize;
  gcry_cipher_setkey_t setkey;
  gcry_cipher_encrypt_t encrypt;
  gcry_cipher_decrypt_t decrypt;
  gcry_cipher_stencrypt_t stencrypt;
  gcry_cipher_stdecrypt_t stdecrypt;
  struct gcry_cipher_spec *next;
} gcry_cipher_spec_t;

/* Type for the md_init function.  */
typedef void (*gcry_md_init_t) (void *c);

/* Type for the md_write function.  */
typedef void (*gcry_md_write_t) (void *c, const void *buf, grub_size_t nbytes);

/* Type for the md_final function.  */
typedef void (*gcry_md_final_t) (void *c);

/* Type for the md_read function.  */
typedef unsigned char *(*gcry_md_read_t) (void *c);

typedef struct gcry_md_oid_spec
{
  const char *oidstring;
} gcry_md_oid_spec_t;

/* Module specification structure for message digests.  */
typedef struct gcry_md_spec
{
  const char *name;
  unsigned char *asnoid;
  int asnlen;
  gcry_md_oid_spec_t *oids;
  grub_size_t mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  grub_size_t contextsize; /* allocate this amount of context */
  /* Block size, needed for HMAC.  */
  grub_size_t blocksize;
  struct gcry_md_spec *next;
} gcry_md_spec_t;

extern gcry_cipher_spec_t *EXPORT_VAR (grub_ciphers);
extern gcry_md_spec_t *EXPORT_VAR (grub_digests);

static inline void 
grub_cipher_register (gcry_cipher_spec_t *cipher)
{
  cipher->next = grub_ciphers;
  grub_ciphers = cipher;
}

static inline void
grub_cipher_unregister (gcry_cipher_spec_t *cipher)
{
  gcry_cipher_spec_t **ciph;
  for (ciph = &grub_ciphers; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      *ciph = (*ciph)->next;
}

static inline void 
grub_md_register (gcry_md_spec_t *digest)
{
  digest->next = grub_digests;
  grub_digests = digest;
}

static inline void 
grub_md_unregister (gcry_md_spec_t *cipher)
{
  gcry_md_spec_t **ciph;
  for (ciph = &grub_digests; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      *ciph = (*ciph)->next;
}

static inline void
grub_crypto_hash (const gcry_md_spec_t *hash, void *out, void *in,
		  grub_size_t inlen)
{
  grub_uint8_t ctx[hash->contextsize];
  hash->init (&ctx);
  hash->write (&ctx, in, inlen);
  hash->final (&ctx);
  grub_memcpy (out, hash->read (&ctx), hash->mdlen);
}

static inline const gcry_md_spec_t *
grub_crypto_lookup_md_by_name (const char *name)
{
  const gcry_md_spec_t *md;
  for (md = grub_digests; md; md = md->next)
    if (grub_strcasecmp (name, md->name) == 0)
      return md;
  return NULL;
}

typedef struct grub_crypto_cipher_handle
{
  const struct gcry_cipher_spec *cipher;
  char ctx[0];
} *grub_crypto_cipher_handle_t;

static inline const gcry_cipher_spec_t *
grub_crypto_lookup_cipher_by_name (const char *name)
{
  const gcry_cipher_spec_t *ciph;
  for (ciph = grub_ciphers; ciph; ciph = ciph->next)
    {
      const char **alias;
      if (grub_strcasecmp (name, ciph->name) == 0)
	return ciph;
      if (!ciph->aliases)
	continue;
      for (alias = ciph->aliases; *alias; alias++)
	if (grub_strcasecmp (name, *alias) == 0)
	  return ciph;
    }
  return NULL;
}


static inline grub_crypto_cipher_handle_t
grub_crypto_cipher_open (const struct gcry_cipher_spec *cipher)
{
  grub_crypto_cipher_handle_t ret;
  ret = grub_malloc (sizeof (*ret) + cipher->contextsize);
  if (!ret)
    return NULL;
  ret->cipher = cipher;
  return ret;
}

static inline gcry_err_code_t
grub_crypto_cipher_set_key (grub_crypto_cipher_handle_t cipher,
			    const unsigned char *key,
			    unsigned keylen)
{
  return cipher->cipher->setkey (cipher->ctx, key, keylen);
}


static inline void
grub_crypto_cipher_close (grub_crypto_cipher_handle_t cipher)
{
  grub_free (cipher);
}


static inline void
grub_crypto_xor (void *out, const void *in1, const void *in2, grub_size_t size)
{
  const grub_uint8_t *in1ptr = in1, *in2ptr = in2;
  grub_uint8_t *outptr = out;
  while (size--)
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
    }
}

static inline grub_err_t
grub_crypto_ecb_decrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size)
{
  grub_uint8_t *inptr, *outptr, *end;
  if (size % cipher->cipher->blocksize != 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, 
		       "This encryption can't decrypt partial blocks");
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
  return GRUB_ERR_NONE;
}

static inline grub_err_t
grub_crypto_ecb_encrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size)
{
  grub_uint8_t *inptr, *outptr, *end;
  if (size % cipher->cipher->blocksize != 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, 
		       "This encryption can't decrypt partial blocks");
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    cipher->cipher->encrypt (cipher->ctx, outptr, inptr);
  return GRUB_ERR_NONE;
}

static inline grub_err_t
grub_crypto_cbc_encrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size,
			 void *iv_in)
{
  grub_uint8_t *inptr, *outptr, *end;
  void *iv;
  if (size % cipher->cipher->blocksize != 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, 
		       "This encryption can't decrypt partial blocks");
  end = (grub_uint8_t *) in + size;
  iv = iv_in;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    {
      grub_crypto_xor (outptr, inptr, iv, cipher->cipher->blocksize);
      cipher->cipher->encrypt (cipher->ctx, outptr, outptr);
      iv = outptr;
    }
  grub_memcpy (iv_in, iv, cipher->cipher->blocksize);
  return GRUB_ERR_NONE;
}

static inline grub_err_t
grub_crypto_cbc_decrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size,
			 void *iv)
{
  grub_uint8_t *inptr, *outptr, *end;
  grub_uint8_t ivt[cipher->cipher->blocksize];
  if (size % cipher->cipher->blocksize != 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, 
		       "This encryption can't decrypt partial blocks");
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    {
      grub_memcpy (ivt, inptr, cipher->cipher->blocksize);
      cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
      grub_crypto_xor (outptr, outptr, iv, cipher->cipher->blocksize);
      grub_memcpy (iv, ivt, cipher->cipher->blocksize);
    }
  return GRUB_ERR_NONE;
}


void EXPORT_FUNC(grub_burn_stack) (grub_size_t size);

extern gcry_md_spec_t _gcry_digest_spec_md5;
#define GRUB_MD_MD5 ((const gcry_md_spec_t *) &_gcry_digest_spec_md5)

#endif
