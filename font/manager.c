/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/file.h>
#include <pupa/misc.h>
#include <pupa/dl.h>
#include <pupa/normal.h>
#include <pupa/types.h>
#include <pupa/mm.h>
#include <pupa/font.h>

struct entry
{
  pupa_uint32_t code;
  pupa_uint32_t offset;
};

struct font
{
  struct font *next;
  pupa_file_t file;
  pupa_uint32_t num;
  struct entry table[0];
};

static struct font *font_list;

static int
add_font (const char *filename)
{
  pupa_file_t file = 0;
  char magic[4];
  pupa_uint32_t num, i;
  struct font *font = 0;

  file = pupa_file_open (filename);
  if (! file)
    goto fail;

  if (pupa_file_read (file, magic, 4) != 4)
    goto fail;

  if (pupa_memcmp (magic, PUPA_FONT_MAGIC, 4) != 0)
    {
      pupa_error (PUPA_ERR_BAD_FONT, "invalid font magic");
      goto fail;
    }

  if (pupa_file_read (file, (char *) &num, 4) != 4)
    goto fail;

  num = pupa_le_to_cpu32 (num);
  font = (struct font *) pupa_malloc (sizeof (struct font)
				      + sizeof (struct entry) * num);
  if (! font)
    goto fail;

  font->file = file;
  font->num = num;

  for (i = 0; i < num; i++)
    {
      pupa_uint32_t code, offset;
      
      if (pupa_file_read (file, (char *) &code, 4) != 4)
	goto fail;

      if (pupa_file_read (file, (char *) &offset, 4) != 4)
	goto fail;

      font->table[i].code = pupa_le_to_cpu32 (code);
      font->table[i].offset = pupa_le_to_cpu32 (offset);
    }

  font->next = font_list;
  font_list = font;

  return 1;

 fail:
  if (font)
    pupa_free (font);

  if (file)
    pupa_file_close (file);

  return 0;
}

static void
remove_font (struct font *font)
{
  struct font **p, *q;

  for (p = &font_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == font)
      {
        *p = q->next;
	
	pupa_file_close (font->file);
	pupa_free (font);
	
        break;
      }
}

/* Return the offset of the glyph corresponding to the codepoint CODE
   in the font FONT. If no found, return zero.  */
static pupa_uint32_t
find_glyph (const struct font *font, pupa_uint32_t code)
{
  pupa_uint32_t start = 0;
  pupa_uint32_t end = font->num - 1;
  struct entry *table = font->table;
  
  /* This shouldn't happen.  */
  if (font->num == 0)
    return 0;

  /* Do a binary search.  */
  while (start <= end)
    {
      pupa_uint32_t i = (start + end) / 2;

      if (table[i].code < code)
	start = i + 1;
      else if (table[i].code > code)
	end = i - 1;
      else
	return table[i].offset;
    }

  return 0;
}

/* Set the glyph to something stupid.  */
static void
fill_with_default_glyph (unsigned char bitmap[32], unsigned *width)
{
  if (bitmap)
    {
      unsigned i;

      for (i = 0; i < 16; i++)
	bitmap[i] = (i & 1) ? 0x55 : 0xaa;
    }
      
  *width = 1;
}

/* Get a glyph corresponding to the codepoint CODE. Always fill BITMAP
   and WIDTH with something, even if no glyph is found.  */
int
pupa_font_get_glyph (pupa_uint32_t code,
		     unsigned char bitmap[32], unsigned *width)
{
  struct font *font;

  /* FIXME: It is necessary to cache glyphs!  */
  
 restart:
  for (font = font_list; font; font = font->next)
    {
      pupa_uint32_t offset;

      offset = find_glyph (font, code);
      if (offset)
	{
	  pupa_uint32_t w;
	  
	  pupa_file_seek (font->file, offset);
	  if (pupa_file_read (font->file, (char *) &w, 4) != 4)
	    {
	      remove_font (font);
	      goto restart;
	    }

	  w = pupa_le_to_cpu32 (w);
	  if (w != 1 && w != 2)
	    {
	      /* pupa_error (PUPA_ERR_BAD_FONT, "invalid width"); */
	      remove_font (font);
	      goto restart;
	    }

	  if (bitmap
	      && (pupa_file_read (font->file, bitmap, w * 16)
		  != (pupa_ssize_t) w * 16))
	    {
	      remove_font (font);
	      goto restart;
	    }

	  *width = w;
	  return 1;
	}
    }

  /* Uggh... No font was found.  */
  fill_with_default_glyph (bitmap, width);
  return 0;
}

static pupa_err_t
font_command (struct pupa_arg_list *state __attribute__ ((unused)),
	      int argc  __attribute__ ((unused)),
	      char **args __attribute__ ((unused)))
{
  if (argc == 0)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "no font specified");

  while (argc--)
    if (! add_font (*args++))
      return 1;

  return 0;
}

PUPA_MOD_INIT
{
  (void) mod; /* Stop warning.  */
  pupa_register_command ("font", font_command, PUPA_COMMAND_FLAG_BOTH,
			 "font FILE...", "Specify a font file to display.", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("font");
}
