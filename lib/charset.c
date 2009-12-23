/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */

#include <grub/charset.h>

grub_ssize_t
grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
		    const grub_uint8_t *src, grub_size_t srcsize,
		    const grub_uint8_t **srcend)
{
  grub_uint16_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
    {
      grub_uint32_t c = *src++;
      if (srcsize != (grub_size_t)-1)
	srcsize--;
      if (count)
	{
	  if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
	    {
	      /* invalid */
	      return -1;
	    }
	  else
	    {
	      code <<= 6;
	      code |= (c & GRUB_UINT8_6_TRAILINGBITS);
	      count--;
	    }
	}
      else
	{
	  if (c == 0)
	    break;

	  if ((c & GRUB_UINT8_1_LEADINGBIT) == 0)
	    code = c;
	  else if ((c & GRUB_UINT8_3_LEADINGBITS) == GRUB_UINT8_2_LEADINGBITS)
	    {
	      count = 1;
	      code = c & GRUB_UINT8_5_TRAILINGBITS;
	    }
	  else if ((c & GRUB_UINT8_4_LEADINGBITS) == GRUB_UINT8_3_LEADINGBITS)
	    {
	      count = 2;
	      code = c & GRUB_UINT8_4_TRAILINGBITS;
	    }
	  else if ((c & GRUB_UINT8_5_LEADINGBITS) == GRUB_UINT8_4_LEADINGBITS)
	    {
	      count = 3;
	      code = c & GRUB_UINT8_3_TRAILINGBITS;
	    }
	  else if ((c & GRUB_UINT8_6_LEADINGBITS) == GRUB_UINT8_5_LEADINGBITS)
	    {
	      count = 4;
	      code = c & GRUB_UINT8_2_TRAILINGBITS;
	    }
	  else if ((c & GRUB_UINT8_7_LEADINGBITS) == GRUB_UINT8_6_LEADINGBITS)
	    {
	      count = 5;
	      code = c & GRUB_UINT8_1_TRAILINGBIT;
	    }
	  else
	    return -1;
	}

      if (count == 0)
	{
	  if (destsize < 2 && code >= GRUB_UCS2_LIMIT)
	    break;
	  if (code >= GRUB_UCS2_LIMIT)
	    {
	      *p++ = GRUB_UTF16_UPPER_SURROGATE (code);
	      *p++ = GRUB_UTF16_LOWER_SURROGATE (code);
	      destsize -= 2;
	    }
	  else
	    {
	      *p++ = code;
	      destsize--;
	    }
	}
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}

/* Convert UCS-4 to UTF-8.  */
char *
grub_ucs4_to_utf8_alloc (grub_uint32_t *src, grub_size_t size)
{
  grub_size_t remaining;
  grub_uint32_t *ptr;
  grub_size_t cnt = 0;
  grub_uint8_t *ret, *dest;

  remaining = size;
  ptr = src;
  while (remaining--)
    {
      grub_uint32_t code = *ptr++;
      
      if (code <= 0x007F)
	cnt++;
      else if (code <= 0x07FF)
	cnt += 2;
      else if ((code >= 0xDC00 && code <= 0xDFFF)
	       || (code >= 0xD800 && code <= 0xDBFF))
	/* No surrogates in UCS-4... */
	cnt++;
      else
	cnt += 3;
    }
  cnt++;

  ret = grub_malloc (cnt);
  if (!ret)
    return 0;

  dest = ret;
  remaining = size;
  ptr = src;
  while (remaining--)
    {
      grub_uint32_t code = *ptr++;

      if (code <= 0x007F)
	*dest++ = code;
      else if (code <= 0x07FF)
	{
	  *dest++ = (code >> 6) | 0xC0;
	  *dest++ = (code & 0x3F) | 0x80;
	}
      else if ((code >= 0xDC00 && code <= 0xDFFF)
	       || (code >= 0xD800 && code <= 0xDBFF))
	{
	  /* No surrogates in UCS-4... */
	  *dest++ = '?';
	}
      else
	{
	  *dest++ = (code >> 12) | 0xE0;
	  *dest++ = ((code >> 6) & 0x3F) | 0x80;
	  *dest++ = (code & 0x3F) | 0x80;
	}
    }
  *dest = 0;

  return ret;
}
