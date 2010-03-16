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
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/unicode.h>
#include <grub/term.h>
#include <grub/normal.h>

#ifdef HAVE_UNIFONT_WIDTHSPEC
#include "widthspec.h"
#endif

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
void
grub_ucs4_to_utf8 (grub_uint32_t *src, grub_size_t size,
		   grub_uint8_t *dest, grub_size_t destsize)
{
  /* Keep last char for \0.  */
  grub_uint8_t *destend = dest + destsize - 1;

  while (size-- && dest < destend)
    {
      grub_uint32_t code = *src++;

      if (code <= 0x007F)
	*dest++ = code;
      else if (code <= 0x07FF)
	{
	  if (dest + 1 >= destend)
	    break;
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
	  if (dest + 2 >= destend)
	    break;
	  *dest++ = (code >> 12) | 0xE0;
	  *dest++ = ((code >> 6) & 0x3F) | 0x80;
	  *dest++ = (code & 0x3F) | 0x80;
	}
    }
  *dest = 0;
}

/* Convert UCS-4 to UTF-8.  */
char *
grub_ucs4_to_utf8_alloc (grub_uint32_t *src, grub_size_t size)
{
  grub_size_t remaining;
  grub_uint32_t *ptr;
  grub_size_t cnt = 0;
  grub_uint8_t *ret;

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

  grub_ucs4_to_utf8 (src, size, ret, cnt);

  return (char *) ret;
}

int
grub_is_valid_utf8 (const grub_uint8_t *src, grub_size_t srcsize)
{
  grub_uint32_t code = 0;
  int count = 0;

  while (srcsize)
    {
      grub_uint32_t c = *src++;
      if (srcsize != (grub_size_t)-1)
	srcsize--;
      if (count)
	{
	  if ((c & 0xc0) != 0x80)
	    {
	      /* invalid */
	      return 0;
	    }
	  else
	    {
	      code <<= 6;
	      code |= (c & 0x3f);
	      count--;
	    }
	}
      else
	{
	  if (c == 0)
	    break;

	  if ((c & 0x80) == 0x00)
	    code = c;
	  else if ((c & 0xe0) == 0xc0)
	    {
	      count = 1;
	      code = c & 0x1f;
	    }
	  else if ((c & 0xf0) == 0xe0)
	    {
	      count = 2;
	      code = c & 0x0f;
	    }
	  else if ((c & 0xf8) == 0xf0)
	    {
	      count = 3;
	      code = c & 0x07;
	    }
	  else if ((c & 0xfc) == 0xf8)
	    {
	      count = 4;
	      code = c & 0x03;
	    }
	  else if ((c & 0xfe) == 0xfc)
	    {
	      count = 5;
	      code = c & 0x01;
	    }
	  else
	    return 0;
	}
    }

  return 1;
}

int
grub_utf8_to_ucs4_alloc (const char *msg, grub_uint32_t **unicode_msg,
			grub_uint32_t **last_position)
{
  grub_size_t msg_len = grub_strlen (msg);

  *unicode_msg = grub_malloc (grub_strlen (msg) * sizeof (grub_uint32_t));
 
  if (!*unicode_msg)
    {
      grub_printf ("utf8_to_ucs4 ERROR1: %s", msg);
      return -1;
    }

  msg_len = grub_utf8_to_ucs4 (*unicode_msg, msg_len,
  			      (grub_uint8_t *) msg, -1, 0);

  *last_position = *unicode_msg + msg_len;

  return msg_len;
}

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UCS-4 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
grub_size_t
grub_utf8_to_ucs4 (grub_uint32_t *dest, grub_size_t destsize,
		   const grub_uint8_t *src, grub_size_t srcsize,
		   const grub_uint8_t **srcend)
{
  grub_uint32_t *p = dest;
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
	  if ((c & 0xc0) != 0x80)
	    {
	      /* invalid */
	      code = '?';
	      /* Character c may be valid, don't eat it.  */
	      src--;
	      if (srcsize != (grub_size_t)-1)
		srcsize++;
	      count = 0;
	    }
	  else
	    {
	      code <<= 6;
	      code |= (c & 0x3f);
	      count--;
	    }
	}
      else
	{
	  if (c == 0)
	    break;

	  if ((c & 0x80) == 0x00)
	    code = c;
	  else if ((c & 0xe0) == 0xc0)
	    {
	      count = 1;
	      code = c & 0x1f;
	    }
	  else if ((c & 0xf0) == 0xe0)
	    {
	      count = 2;
	      code = c & 0x0f;
	    }
	  else if ((c & 0xf8) == 0xf0)
	    {
	      count = 3;
	      code = c & 0x07;
	    }
	  else if ((c & 0xfc) == 0xf8)
	    {
	      count = 4;
	      code = c & 0x03;
	    }
	  else if ((c & 0xfe) == 0xfc)
	    {
	      count = 5;
	      code = c & 0x01;
	    }
	  else
	    {
	      /* invalid */
	      code = '?';
	      count = 0;
	    }
	}

      if (count == 0)
	{
	  *p++ = code;
	  destsize--;
	}
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}

static grub_uint8_t *bidi_types = NULL;

static void
unpack_bidi (void)
{
  unsigned i;
  struct grub_unicode_compact_range *cur;

  bidi_types = grub_zalloc (GRUB_UNICODE_MAX_CACHED_CHAR);
  if (!bidi_types)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  for (cur = grub_unicode_compact; cur->end; cur++)
    for (i = cur->start; i <= cur->end
	     && i < GRUB_UNICODE_MAX_CACHED_CHAR; i++)
      if (cur->bidi_mirror)
	bidi_types[i] = cur->bidi_type | 0x80;
      else
	bidi_types[i] = cur->bidi_type | 0x00;
}

static inline enum grub_bidi_type
get_bidi_type (grub_uint32_t c)
{
  struct grub_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return bidi_types[c] & 0x7f;

  for (cur = grub_unicode_compact; cur->end; cur++)
    if (cur->start <= c && c <= cur->end)
      return cur->bidi_type;

  return GRUB_BIDI_TYPE_L;
}

static inline int
is_mirrored (grub_uint32_t c)
{
  struct grub_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return !!(bidi_types[c] & 0x80);

  for (cur = grub_unicode_compact; cur->end; cur++)
    if (cur->start <= c && c <= cur->end)
      return cur->bidi_mirror;

  return 0;
}

enum grub_comb_type
grub_unicode_get_comb_type (grub_uint32_t c)
{
  static grub_uint8_t *comb_types = NULL;
  struct grub_unicode_compact_range *cur;

  if (!comb_types)
    {
      unsigned i;
      comb_types = grub_zalloc (GRUB_UNICODE_MAX_CACHED_CHAR);
      if (comb_types)
	for (cur = grub_unicode_compact; cur->end; cur++)
	  for (i = cur->start; i <= cur->end
		 && i < GRUB_UNICODE_MAX_CACHED_CHAR; i++)
	    comb_types[i] = cur->comb_type;
      else
	grub_errno = GRUB_ERR_NONE;
    }

  if (comb_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return comb_types[c];

  for (cur = grub_unicode_compact; cur->end; cur++)
    if (cur->start <= c && c <= cur->end)
      return cur->comb_type;

  return GRUB_UNICODE_COMB_NONE;
}

#ifdef HAVE_UNIFONT_WIDTHSPEC

grub_ssize_t
grub_unicode_estimate_width (const struct grub_unicode_glyph *c)
{
  if (grub_unicode_get_comb_type (c->base))
    return 0;
  if (widthspec[c->base >> 3] & (1 << (c->base & 7)))
    return 2;
  else
    return 1;
}

#endif

grub_size_t
grub_unicode_aglomerate_comb (const grub_uint32_t *in, grub_size_t inlen,
			      struct grub_unicode_glyph *out)
{
  int haveout = 0;
  const grub_uint32_t *ptr;
  unsigned last_comb_pointer = 0;

  grub_memset (out, 0, sizeof (*out));

  for (ptr = in; ptr < in + inlen; ptr++)
    {
      /* Variation selectors >= 17 are outside of BMP and SMP. 
	 Handle variation selectors first to avoid potentially costly lookups.
      */
      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_16)
	{
	  if (haveout)
	    out->variant = *ptr - GRUB_UNICODE_VARIATION_SELECTOR_1 + 1;
	  continue;

	}
      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_256)
	{
	  if (haveout)
	    out->variant = *ptr - GRUB_UNICODE_VARIATION_SELECTOR_17 + 17;
	  continue;
	}
	
      enum grub_comb_type comb_type;
      comb_type = grub_unicode_get_comb_type (*ptr);
      if (comb_type)
	{
	  struct grub_unicode_combining *n;
	  unsigned j;

	  if (!haveout)
	    continue;

	  if (comb_type == GRUB_UNICODE_COMB_MC
	      || comb_type == GRUB_UNICODE_COMB_ME
	      || comb_type == GRUB_UNICODE_COMB_MN)
	    last_comb_pointer = out->ncomb;
	  n = grub_realloc (out->combining, 
			    sizeof (n[0]) * (out->ncomb + 1));
	  if (!n)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  for (j = last_comb_pointer; j < out->ncomb; j++)
	    if (out->combining[j].type > comb_type)
	      break;
	  grub_memmove (out->combining + j + 1,
			out->combining + j,
			(out->ncomb - j)
			* sizeof (out->combining[0]));
	  out->combining = n;
	  out->combining[j].code = *ptr;
	  out->combining[j].type = comb_type;
	  out->ncomb++;
	  continue;
	}
      if (haveout)
	return ptr - in;
      haveout = 1;
      out->base = *ptr;
      out->variant = 0;
      out->attributes = 0;
      out->ncomb = 0;
      out->estimated_width = 1;
      out->combining = NULL;
    }
  return ptr - in;
}

static grub_ssize_t
bidi_line_wrap (struct grub_unicode_glyph *visual_out,
		struct grub_unicode_glyph *visual,
		grub_size_t visual_len, unsigned *levels,
		grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
		grub_size_t maxwidth, grub_size_t startwidth)
{
  struct grub_unicode_glyph *outptr = visual_out;
  unsigned line_start = 0;
  grub_ssize_t line_width = startwidth;
  unsigned k;
  grub_ssize_t last_space = -1;
  grub_ssize_t last_space_width = 0;

  auto void revert (unsigned start, unsigned end);
  void revert (unsigned start, unsigned end)
  {
    struct grub_unicode_glyph t;
    unsigned i, tl;
    for (i = 0; i <= (end - start) / 2; i++)
      {
	t = visual[start + i];
	visual[start + i] = visual[end - i];
	visual[end - i] = t;
	tl = levels[start + i];
	levels[start + i] = levels[end - i];
	levels[end - i] = tl;
      }
  }

  if (!visual_len)
    return 0;

  for (k = 0; k <= visual_len; k++)
    {
      grub_ssize_t last_width = 0;

      if (getcharwidth && k != visual_len)
	line_width += last_width = getcharwidth (&visual[k]);

      if (k != visual_len && visual[k].base == ' ')
	{
	  last_space = k;
	  last_space_width = line_width;
	}

      if (((grub_ssize_t) maxwidth > 0 
	   && line_width > (grub_ssize_t) maxwidth) || k == visual_len)
	{	  
	  unsigned min_odd_level = 0xffffffff;
	  unsigned max_level = 0;
	  unsigned j;
	  unsigned i;

	  if (k != visual_len && last_space > (signed) line_start)
	    k = last_space;
	  else if (k != visual_len && line_start == 0 && startwidth != 0)
	    {
	      k = 0;
	      last_space_width = startwidth;
	    }
	  else
	    last_space_width = line_width - last_width;

	  for (i = line_start; i < k; i++)
	    {
	      if (levels[i] > max_level)
		max_level = levels[i];
	      if (levels[i] < min_odd_level && (levels[i] & 1))
		min_odd_level = levels[i];
	    }
	  
	  /* FIXME: can be optimized.  */
	  for (j = max_level; j >= min_odd_level; j--)
	    {
	      unsigned in = 0;
	      for (i = line_start; i < k; i++)
		{
		  if (i != line_start && levels[i] >= j && levels[i-1] < j)
		    in = i;
		  if (levels[i] >= j && (i + 1 == k || levels[i+1] < j))
		    revert (in, i);
		}
	    }
	  
	  for (i = line_start; i < k; i++)
	    if (is_mirrored (visual[i].base) && levels[i])
	      visual[i].attributes |= GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR;

	  grub_memcpy (outptr, &visual[line_start],
		       (k - line_start) * sizeof (visual[0]));
	  outptr += k - line_start;
	  if (k != visual_len)
	    {
	      grub_memset (outptr, 0, sizeof (visual[0]));
	      outptr->base = '\n';
	      outptr++;
	    }

	  if ((signed) k == last_space)
	    k++;

	  line_start = k;
	  line_width -= last_space_width;
	}
    }

  return outptr - visual_out;
}


static grub_ssize_t
grub_bidi_line_logical_to_visual (const grub_uint32_t *logical,
				  grub_size_t logical_len,
				  struct grub_unicode_glyph *visual_out,
				  grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
				  grub_size_t maxwidth, grub_size_t startwidth)
{
  enum grub_bidi_type type = GRUB_BIDI_TYPE_L;
  enum override_status {OVERRIDE_NEUTRAL = 0, OVERRIDE_R, OVERRIDE_L};
  unsigned *levels;
  enum grub_bidi_type *resolved_types;
  unsigned base_level;
  enum override_status cur_override;
  unsigned i;
  unsigned stack_level[GRUB_BIDI_MAX_EXPLICIT_LEVEL + 3];
  enum override_status stack_override[GRUB_BIDI_MAX_EXPLICIT_LEVEL + 3];
  unsigned stack_depth = 0;
  unsigned invalid_pushes = 0;
  unsigned visual_len = 0;
  unsigned run_start, run_end;
  struct grub_unicode_glyph *visual;
  unsigned cur_level;

  auto void push_stack (unsigned new_override, unsigned new_level);
  void push_stack (unsigned new_override, unsigned new_level)
  {
    if (new_level > GRUB_BIDI_MAX_EXPLICIT_LEVEL)
      {
	invalid_pushes++;
	return;
      }
    stack_level[stack_depth] = cur_level;
    stack_override[stack_depth] = cur_override;
    stack_depth++;
    cur_level = new_level;
    cur_override = new_override;
  }

  auto void pop_stack (void);
  void pop_stack (void)
  {
    if (invalid_pushes)
      {
	invalid_pushes--;
	return;
      }
    if (!stack_depth)
      return;
    stack_depth--;
    cur_level = stack_level[stack_depth];
    cur_override = stack_override[stack_depth];
  }

  levels = grub_malloc (sizeof (levels[0]) * logical_len);
  if (!levels)
    return -1;

  resolved_types = grub_malloc (sizeof (resolved_types[0]) * logical_len);
  if (!resolved_types)
    {
      grub_free (levels);
      return -1;
    }

  visual = grub_malloc (sizeof (visual[0]) * logical_len);
  if (!visual)
    {
      grub_free (resolved_types);
      grub_free (levels);
      return -1;
    }

  for (i = 0; i < logical_len; i++)
    {
      type = get_bidi_type (logical[i]);
      if (type == GRUB_BIDI_TYPE_L || type == GRUB_BIDI_TYPE_AL
	  || type == GRUB_BIDI_TYPE_R)
	break;
    }
  if (type == GRUB_BIDI_TYPE_R || type == GRUB_BIDI_TYPE_AL)
    base_level = 1;
  else
    base_level = 0;
  
  cur_level = base_level;
  cur_override = OVERRIDE_NEUTRAL;
  {
    const grub_uint32_t *lptr;
    for (lptr = logical; lptr < logical + logical_len;)
      {
	grub_size_t p = grub_unicode_aglomerate_comb (lptr, logical 
						      + logical_len - lptr, 
						      &visual[visual_len]);
	
	type = get_bidi_type (visual[visual_len].base);
	switch (type)
	  {
	  case GRUB_BIDI_TYPE_RLE:
	    push_stack (cur_override, (cur_level | 1) + 1);
	    break;
	  case GRUB_BIDI_TYPE_RLO:
	    push_stack (OVERRIDE_R, (cur_level | 1) + 1);
	    break;
	  case GRUB_BIDI_TYPE_LRE:
	    push_stack (cur_override, (cur_level & ~1) + 2);
	    break;
	  case GRUB_BIDI_TYPE_LRO:
	    push_stack (OVERRIDE_L, (cur_level & ~1) + 2);
	    break;
	  case GRUB_BIDI_TYPE_PDF:
	    pop_stack ();
	    break;
	  case GRUB_BIDI_TYPE_BN:
	    break;
	  default:
	    {
	      levels[visual_len] = cur_level;
	      if (cur_override != OVERRIDE_NEUTRAL)
		resolved_types[visual_len] = 
		  (cur_override == OVERRIDE_L) ? GRUB_BIDI_TYPE_L
		  : GRUB_BIDI_TYPE_R;
	      else
		resolved_types[visual_len] = type;
	      visual_len++;
	    }
	  }
	lptr += p;
      }
  }

  for (run_start = 0; run_start < visual_len; run_start = run_end)
    {
      unsigned prev_level, next_level, cur_run_level;
      unsigned last_type, last_strong_type;
      for (run_end = run_start; run_end < visual_len &&
	     levels[run_end] == levels[run_start]; run_end++);
      if (run_start == 0)
	prev_level = base_level;
      else
	prev_level = levels[run_start - 1];
      if (run_end == visual_len)
	next_level = base_level;
      else
	next_level = levels[run_end];
      cur_run_level = levels[run_start];
      if (prev_level & 1)
	last_type = GRUB_BIDI_TYPE_R;
      else
	last_type = GRUB_BIDI_TYPE_L;
      last_strong_type = last_type;
      for (i = run_start; i < run_end; i++)
	{
	  switch (resolved_types[i])
	    {
	    case GRUB_BIDI_TYPE_NSM:
	      resolved_types[i] = last_type;
	      break;
	    case GRUB_BIDI_TYPE_EN:
	      if (last_strong_type == GRUB_BIDI_TYPE_AL)
		resolved_types[i] = GRUB_BIDI_TYPE_AN;
	      break;
	    case GRUB_BIDI_TYPE_L:
	    case GRUB_BIDI_TYPE_R:
	      last_strong_type = resolved_types[i];
	      break;
	    case GRUB_BIDI_TYPE_ES:
	      if (last_type == GRUB_BIDI_TYPE_EN
		  && i + 1 < run_end 
		  && resolved_types[i + 1] == GRUB_BIDI_TYPE_EN)
		resolved_types[i] = GRUB_BIDI_TYPE_EN;
	      else
		resolved_types[i] = GRUB_BIDI_TYPE_ON;
	      break;
	    case GRUB_BIDI_TYPE_ET:
	      {
		unsigned j;
		if (last_type == GRUB_BIDI_TYPE_EN)
		  {
		    resolved_types[i] = GRUB_BIDI_TYPE_EN;
		    break;
		  }
		for (j = i; j < run_end
		       && resolved_types[j] == GRUB_BIDI_TYPE_ET; j++);
		if (j != run_end && resolved_types[j] == GRUB_BIDI_TYPE_EN)
		  {
		    for (; i < run_end
			   && resolved_types[i] == GRUB_BIDI_TYPE_ET; i++)
		      resolved_types[i] = GRUB_BIDI_TYPE_EN;
		    i--;
		    break;
		  }
		for (; i < run_end
		       && resolved_types[i] == GRUB_BIDI_TYPE_ET; i++)
		  resolved_types[i] = GRUB_BIDI_TYPE_ON;
		i--;
		break;		
	      }
	      break;
	    case GRUB_BIDI_TYPE_CS:
	      if (last_type == GRUB_BIDI_TYPE_EN
		  && i + 1 < run_end 
		  && resolved_types[i + 1] == GRUB_BIDI_TYPE_EN)
		{
		  resolved_types[i] = GRUB_BIDI_TYPE_EN;
		  break;
		}
	      if (last_type == GRUB_BIDI_TYPE_AN
		  && i + 1 < run_end 
		  && (resolved_types[i + 1] == GRUB_BIDI_TYPE_AN
		      || (resolved_types[i + 1] == GRUB_BIDI_TYPE_EN
			  && last_strong_type == GRUB_BIDI_TYPE_AL)))
		{
		  resolved_types[i] = GRUB_BIDI_TYPE_EN;
		  break;
		}
	      resolved_types[i] = GRUB_BIDI_TYPE_ON;
	      break;
	    case GRUB_BIDI_TYPE_AL:
	      last_strong_type = resolved_types[i];
	      resolved_types[i] = GRUB_BIDI_TYPE_R;
	      break;
	    default: /* Make GCC happy.  */
	      break;
	    }
	  last_type = resolved_types[i];
	  if (resolved_types[i] == GRUB_BIDI_TYPE_EN
	      && last_strong_type == GRUB_BIDI_TYPE_L)
	    resolved_types[i] = GRUB_BIDI_TYPE_L;
	}
      if (prev_level & 1)
	last_type = GRUB_BIDI_TYPE_R;
      else
	last_type = GRUB_BIDI_TYPE_L;
      for (i = run_start; i < run_end; )
	{
	  unsigned j;
	  unsigned next_type;
	  for (j = i; j < run_end &&
	  	 (resolved_types[j] == GRUB_BIDI_TYPE_B
	  	  || resolved_types[j] == GRUB_BIDI_TYPE_S
	  	  || resolved_types[j] == GRUB_BIDI_TYPE_WS
	  	  || resolved_types[j] == GRUB_BIDI_TYPE_ON); j++);
	  if (j == i)
	    {
	      if (resolved_types[i] == GRUB_BIDI_TYPE_L)
		last_type = GRUB_BIDI_TYPE_L;
	      else
		last_type = GRUB_BIDI_TYPE_R;
	      i++;
	      continue;
	    }
	  if (j == run_end)
	    next_type = (next_level & 1) ? GRUB_BIDI_TYPE_R : GRUB_BIDI_TYPE_L;
	  else
	    {
	      if (resolved_types[j] == GRUB_BIDI_TYPE_L)
		next_type = GRUB_BIDI_TYPE_L;
	      else
		next_type = GRUB_BIDI_TYPE_R;
	    }
	  if (next_type == last_type)
	    for (; i < j; i++)
	      resolved_types[i] = last_type;
	  else
	    for (; i < j; i++)
	      resolved_types[i] = (cur_run_level & 1) ? GRUB_BIDI_TYPE_R
		: GRUB_BIDI_TYPE_L;
	}
    }

  for (i = 0; i < visual_len; i++)
    {
      if (!(levels[i] & 1) && resolved_types[i] == GRUB_BIDI_TYPE_R)
	{
	  levels[i]++;
	  continue;
	}
      if (!(levels[i] & 1) && (resolved_types[i] == GRUB_BIDI_TYPE_AN
			       || resolved_types[i] == GRUB_BIDI_TYPE_EN))
	{
	  levels[i] += 2;
	  continue;
	}
      if ((levels[i] & 1) && (resolved_types[i] == GRUB_BIDI_TYPE_L
			      || resolved_types[i] == GRUB_BIDI_TYPE_AN
			      || resolved_types[i] == GRUB_BIDI_TYPE_EN))
	{
	  levels[i]++;
	  continue;
	}
    }
  grub_free (resolved_types);

  {
    grub_ssize_t ret;
    ret = bidi_line_wrap (visual_out, visual, visual_len, levels, 
			  getcharwidth, maxwidth, startwidth);
    grub_free (levels);
    grub_free (visual);
    return ret;
  }
}

grub_ssize_t
grub_bidi_logical_to_visual (const grub_uint32_t *logical,
			     grub_size_t logical_len,
			     struct grub_unicode_glyph **visual_out,
			     grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
			     grub_size_t max_length, grub_size_t startwidth)
{
  const grub_uint32_t *line_start = logical, *ptr;
  struct grub_unicode_glyph *visual_ptr;
  *visual_out = visual_ptr = grub_malloc (2 * sizeof (visual_ptr[0])
					  * logical_len);
  for (ptr = logical; ptr <= logical + logical_len; ptr++)
    {
      if (ptr == logical + logical_len || *ptr == '\n')
	{
	  grub_ssize_t ret;
	  ret = grub_bidi_line_logical_to_visual (line_start,
						  ptr - line_start,
						  visual_ptr,
						  getcharwidth,
						  max_length,
						  startwidth);
	  startwidth = 0;

	  if (ret < 0)
	    {
	      grub_free (*visual_out);
	      return ret;
	    }
	  visual_ptr += ret; 
	  line_start = ptr;
	  if (ptr != logical + logical_len)
	    {
	      grub_memset (visual_ptr, 0, sizeof (visual_ptr[0]));
	      visual_ptr->base = '\n';
	      visual_ptr++;
	      line_start++;
	    }
	}
    }
  return visual_ptr - *visual_out;
}

static grub_uint32_t
map_code (grub_uint32_t in, struct grub_term_output *term)
{
  if (in <= 0x7f)
    return in;

  switch (term->flags & GRUB_TERM_CODE_TYPE_MASK)
    {
    case GRUB_TERM_CODE_TYPE_VGA:
      switch (in)
	{
	case GRUB_TERM_DISP_LEFT:
	  return 0x1b;
	case GRUB_TERM_DISP_UP:
	  return 0x18;
	case GRUB_TERM_DISP_RIGHT:
	  return 0x1a;
	case GRUB_TERM_DISP_DOWN:
	  return 0x19;
	case GRUB_TERM_DISP_HLINE:
	  return 0xc4;
	case GRUB_TERM_DISP_VLINE:
	  return 0xb3;
	case GRUB_TERM_DISP_UL:
	  return 0xda;
	case GRUB_TERM_DISP_UR:
	  return 0xbf;
	case GRUB_TERM_DISP_LL:
	  return  0xc0;
	case GRUB_TERM_DISP_LR:
	  return 0xd9;
	}
      return '?';
    case GRUB_TERM_CODE_TYPE_ASCII:
      /* Better than nothing.  */
      switch (in)
	{
	case GRUB_TERM_DISP_LEFT:
	  return '<';
		
	case GRUB_TERM_DISP_UP:
	  return '^';
	  
	case GRUB_TERM_DISP_RIGHT:
	  return '>';
		
	case GRUB_TERM_DISP_DOWN:
	  return 'v';
		  
	case GRUB_TERM_DISP_HLINE:
	  return '-';
		  
	case GRUB_TERM_DISP_VLINE:
	  return '|';
		  
	case GRUB_TERM_DISP_UL:
	case GRUB_TERM_DISP_UR:
	case GRUB_TERM_DISP_LL:
	case GRUB_TERM_DISP_LR:
	  return '+';
		
	}
      return '?';
    }
  return in;
}

static grub_uint32_t
mirror_code (grub_uint32_t in)
{
  int i;
  for (i = 0; grub_unicode_bidi_pairs[i].key; i++)
    if (grub_unicode_bidi_pairs[i].key == in)
      return grub_unicode_bidi_pairs[i].replace;
  return in;
}

static void
putglyph (const struct grub_unicode_glyph *c, struct grub_term_output *term)
{
  struct grub_unicode_glyph c2 =
    {
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .combining = 0,
      .estimated_width = 1
    };

  if (c->base == '\t' && term->getxy)
    {
      int n;

      n = 8 - ((term->getxy () >> 8) & 7);
      c2.base = ' ';
      while (n--)
	(term->putchar) (&c2);

      return;
    }

  if ((term->flags & GRUB_TERM_CODE_TYPE_MASK)
      == GRUB_TERM_CODE_TYPE_UTF8_LOGICAL 
      || (term->flags & GRUB_TERM_CODE_TYPE_MASK)
      == GRUB_TERM_CODE_TYPE_UTF8_VISUAL)
    {
      int i;
      c2.estimated_width = grub_term_getcharwidth (term, c);
      for (i = -1; i < (int) c->ncomb; i++)
	{
	  grub_uint8_t u8[20], *ptr;
	  grub_uint32_t code;
	      
	  if (i == -1)
	    {
	      code = c->base;
	      if (c->attributes & GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR)
		code = mirror_code (code);
	    }
	  else
	    code = c->combining[i].code;

	  grub_ucs4_to_utf8 (&code, 1, u8, sizeof (u8));

	  for (ptr = u8; *ptr; ptr++)
	    {
	      c2.base = *ptr;
	      (term->putchar) (&c2);
	      c2.estimated_width = 0;
	    }
	}
      c2.estimated_width = 1;
    }
  else
    (term->putchar) (c);

  if (c->base == '\n')
    {
      c2.base = '\r';
      (term->putchar) (&c2);
    }
}

static void
putcode_real (grub_uint32_t code, struct grub_term_output *term)
{
  struct grub_unicode_glyph c =
    {
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .combining = 0,
      .estimated_width = 1
    };

  c.base = map_code (code, term);
  putglyph (&c, term);
}

/* Put a Unicode character.  */
void
grub_putcode (grub_uint32_t code, struct grub_term_output *term)
{
  /* Combining character by itself?  */
  if (grub_unicode_get_comb_type (code) != GRUB_UNICODE_COMB_NONE)
    return;

  putcode_real (code, term);
}

void
grub_print_ucs4 (const grub_uint32_t * str,
		 const grub_uint32_t * last_position,
		 int margin_left, int margin_right,
		 struct grub_term_output *term)
{
  grub_ssize_t max_width;
  grub_ssize_t startwidth;

  {
    struct grub_unicode_glyph space_glyph = {
      .base = ' ',
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .combining = 0
      };
    max_width = grub_term_width (term)
      - grub_term_getcharwidth (term, &space_glyph) 
      * (margin_left + margin_right) - 1;
  }

  if (((term->getxy () >> 8) & 0xff) < margin_left)
    grub_print_spaces (term, margin_left - ((term->getxy () >> 8) & 0xff));

  startwidth = ((term->getxy () >> 8) & 0xff) - margin_left;

  if ((term->flags & GRUB_TERM_CODE_TYPE_MASK) 
      == GRUB_TERM_CODE_TYPE_VISUAL_GLYPHS
      || (term->flags & GRUB_TERM_CODE_TYPE_MASK) 
      == GRUB_TERM_CODE_TYPE_UTF8_VISUAL)
    {
      grub_ssize_t visual_len;
      struct grub_unicode_glyph *visual;
      struct grub_unicode_glyph *visual_ptr;

      auto grub_ssize_t getcharwidth (const struct grub_unicode_glyph *c);
      grub_ssize_t getcharwidth (const struct grub_unicode_glyph *c)
      {
	return grub_term_getcharwidth (term, c);
      }

      visual_len = grub_bidi_logical_to_visual (str, last_position - str,
						&visual, getcharwidth,
						max_width, startwidth);
      if (visual_len < 0)
	{
	  grub_print_error ();
	  return;
	}
      for (visual_ptr = visual; visual_ptr < visual + visual_len; visual_ptr++)
	{
	  if (visual_ptr->base == '\n')
	    grub_print_spaces (term, margin_right);
	  putglyph (visual_ptr, term);
	  if (visual_ptr->base == '\n')
	    grub_print_spaces (term, margin_left);
	  grub_free (visual_ptr->combining);
	}
      grub_free (visual);
      return;
    }

  {
    const grub_uint32_t *ptr;
    grub_ssize_t line_width = startwidth;
    grub_ssize_t lastspacewidth = 0;
    const grub_uint32_t *line_start = str, *last_space = str - 1;

    for (ptr = str; ptr < last_position; ptr++)
      {
	grub_ssize_t last_width = 0;
	if (grub_unicode_get_comb_type (*ptr) == GRUB_UNICODE_COMB_NONE)
	  {
	    struct grub_unicode_glyph c = {
	      .variant = 0,
	      .attributes = 0,
	      .ncomb = 0,
	      .combining = 0
	    };
	    c.base = *ptr;
	    line_width += last_width = grub_term_getcharwidth (term, &c);
	  }

	if (*ptr == ' ')
	  {
	    lastspacewidth = line_width;
	    last_space = ptr;
	  }

	if (line_width > max_width || *ptr == '\n')
	  {
	    const grub_uint32_t *ptr2;

	    if (line_width > max_width && last_space > line_start)
	      ptr = last_space;
	    else if (line_width > max_width 
		     && line_start == str && startwidth != 0)
	      {
		ptr = str;
		lastspacewidth = startwidth;
	      }
	    else
	      lastspacewidth = line_width - last_width;

	    for (ptr2 = line_start; ptr2 < ptr; ptr2++)
	      {
		/* Skip combining characters on non-UTF8 terminals.  */
		if ((term->flags & GRUB_TERM_CODE_TYPE_MASK) 
		    != GRUB_TERM_CODE_TYPE_UTF8_LOGICAL
		    && grub_unicode_get_comb_type (*ptr2)
		    != GRUB_UNICODE_COMB_NONE)
		  continue;
		putcode_real (*ptr2, term);
	      }

	    grub_print_spaces (term, margin_right);
	    grub_putcode ('\n', term);
	    line_width -= lastspacewidth;
	    grub_print_spaces (term, margin_left);
	    if (ptr == last_space || *ptr == '\n')
	      ptr++;
	    line_start = ptr;
	  }
      }

    {
      const grub_uint32_t *ptr2;
      for (ptr2 = line_start; ptr2 < last_position; ptr2++)
	{
	  /* Skip combining characters on non-UTF8 terminals.  */
	  if ((term->flags & GRUB_TERM_CODE_TYPE_MASK) 
	      != GRUB_TERM_CODE_TYPE_UTF8_LOGICAL
	      && grub_unicode_get_comb_type (*ptr2)
	      != GRUB_UNICODE_COMB_NONE)
	    continue;
	  putcode_real (*ptr2, term);
	}
    }
  }
}

void
grub_xputs_normal (const char *str)
{
  grub_term_output_t term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_uint32_t *unicode_str, *unicode_last_position;
    grub_utf8_to_ucs4_alloc (str, &unicode_str,
			     &unicode_last_position);
    grub_print_ucs4 (unicode_str, unicode_last_position, 0, 0, term);
    grub_free (unicode_str);
  }
}
