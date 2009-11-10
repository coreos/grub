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

/* Definition of a function used to report selftest failures. 
   DOMAIN is a string describing the function block:
          "cipher", "digest", "pubkey or "random",
   ALGO   is the algorithm under test,
   WHAT   is a string describing what has been tested,
   DESC   is a string describing the error. */
typedef void (*selftest_report_func_t)(const char *domain,
                                       int algo, 
                                       const char *what,
                                       const char *errdesc);

/* Definition of the selftest functions.  */
typedef gpg_err_code_t (*selftest_func_t)
     (int algo, int extended, selftest_report_func_t report);

/* The type used to convey additional information to a cipher.  */
typedef gpg_err_code_t (*cipher_set_extra_info_t)
     (void *c, int what, const void *buffer, grub_size_t buflen);


/* Extra module specification structures.  These are used for internal
   modules which provide more functions than available through the
   public algorithm register APIs.  */
typedef struct cipher_extra_spec
{
  selftest_func_t selftest;
  cipher_set_extra_info_t set_extra_info;
} cipher_extra_spec_t;

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
  int mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  grub_size_t contextsize; /* allocate this amount of context */
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


void EXPORT_FUNC(grub_burn_stack) (grub_size_t size);

#endif
