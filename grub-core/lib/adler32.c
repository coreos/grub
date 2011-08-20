/* adler32.c - adler32 check.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011  Free Software Foundation, Inc.
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
#include <grub/dl.h>
#include <grub/crypto.h>

/* Based on adler32() from adler32.c of zlib-1.2.5 library. */

#define BASE 65521UL
#define NMAX 5552

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

static grub_uint32_t
update_adler32 (grub_uint32_t adler, const grub_uint8_t *buf, grub_size_t len)
{
  unsigned long sum2;
  unsigned int n;

  sum2 = (adler >> 16) & 0xffff;
  adler &= 0xffff;

  if (len == 1)
    {
      adler += buf[0];
      if (adler >= BASE)
	adler -= BASE;
      sum2 += adler;
      if (sum2 >= BASE)
	sum2 -= BASE;
      return adler | (sum2 << 16);
    }

  if (len < 16)
    {
      while (len--)
	{
	  adler += *buf++;
	  sum2 += adler;
	}
      if (adler >= BASE)
	adler -= BASE;
      sum2 %= BASE;
      return adler | (sum2 << 16);
    }

  while (len >= NMAX)
    {
      len -= NMAX;
      n = NMAX / 16;
      do
	{
	  DO16 (buf);
	  buf += 16;
	}
      while (--n);
      adler %= BASE;
      sum2 %= BASE;
    }

  if (len)
    {
      while (len >= 16)
	{
	  len -= 16;
	  DO16 (buf);
	  buf += 16;
	}
      while (len--)
	{
	  adler += *buf++;
	  sum2 += adler;
	}
      adler %= BASE;
      sum2 %= BASE;
    }

  return adler | (sum2 << 16);
}

typedef struct
{
  grub_uint32_t adler;
  grub_uint8_t buf[4];
}
adler32_context;

static void
adler32_init (void *context)
{
  adler32_context *ctx = (adler32_context *) context;
  ctx->adler = 1;
}

static void
adler32_write (void *context, const void *inbuf, grub_size_t inlen)
{
  adler32_context *ctx = (adler32_context *) context;
  if (!inbuf)
    return;
  ctx->adler = update_adler32 (ctx->adler, inbuf, inlen);
}

static grub_uint8_t *
adler32_read (void *context)
{
  adler32_context *ctx = (adler32_context *) context;
  return ctx->buf;
}

static void
adler32_final (void *context __attribute__ ((unused)))
{
}

gcry_md_spec_t _gcry_digest_spec_adler32 = {
  "ADLER32",0 , 0, 0 , 4,
  adler32_init, adler32_write, adler32_final, adler32_read,
  sizeof (adler32_context),
  .blocksize = 64
};

GRUB_MOD_INIT(adler32)
{
  grub_md_register (&_gcry_digest_spec_adler32);
}

GRUB_MOD_FINI(adler32)
{
  grub_md_unregister (&_gcry_digest_spec_adler32);
}
