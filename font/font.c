/* font.c - Font API and font file loader.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/bufio.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/font.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/video.h>
#include <grub/bitmap.h>
#include <grub/charset.h>
#include <grub/unicode.h>

#ifndef FONT_DEBUG
#define FONT_DEBUG 0
#endif

struct char_index_entry
{
  grub_uint32_t code;
  grub_uint8_t storage_flags;
  grub_uint32_t offset;

  /* Glyph if loaded, or NULL otherwise.  */
  struct grub_font_glyph *glyph;
};

#define FONT_WEIGHT_NORMAL 100
#define FONT_WEIGHT_BOLD 200

struct grub_font
{
  char *name;
  grub_file_t file;
  char *family;
  short point_size;
  short weight;
  short max_char_width;
  short max_char_height;
  short ascent;
  short descent;
  short leading;
  grub_uint32_t num_chars;
  struct char_index_entry *char_index;
  grub_uint16_t *bmp_idx;
};

/* Definition of font registry.  */
struct grub_font_node *grub_font_list;

static int register_font (grub_font_t font);
static void font_init (grub_font_t font);
static void free_font (grub_font_t font);
static void remove_font (grub_font_t font);

struct font_file_section
{
  /* The file this section is in.  */
  grub_file_t file;

  /* FOURCC name of the section.  */
  char name[4];

  /* Length of the section contents.  */
  grub_uint32_t length;

  /* Set by open_section() on EOF.  */
  int eof;
};

/* Font file format constants.  */
static const char pff2_magic[4] = { 'P', 'F', 'F', '2' };
static const char section_names_file[4] = { 'F', 'I', 'L', 'E' };
static const char section_names_font_name[4] = { 'N', 'A', 'M', 'E' };
static const char section_names_point_size[4] = { 'P', 'T', 'S', 'Z' };
static const char section_names_weight[4] = { 'W', 'E', 'I', 'G' };
static const char section_names_max_char_width[4] = { 'M', 'A', 'X', 'W' };
static const char section_names_max_char_height[4] = { 'M', 'A', 'X', 'H' };
static const char section_names_ascent[4] = { 'A', 'S', 'C', 'E' };
static const char section_names_descent[4] = { 'D', 'E', 'S', 'C' };
static const char section_names_char_index[4] = { 'C', 'H', 'I', 'X' };
static const char section_names_data[4] = { 'D', 'A', 'T', 'A' };

/* Replace unknown glyphs with a rounded question mark.  */
static grub_uint8_t unknown_glyph_bitmap[] =
{
  /*       76543210 */
  0x7C, /*  ooooo   */
  0x82, /* o     o  */
  0xBA, /* o ooo o  */
  0xAA, /* o o o o  */
  0xAA, /* o o o o  */
  0x8A, /* o   o o  */
  0x9A, /* o  oo o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x82, /* o     o  */
  0x92, /* o  o  o  */
  0x82, /* o     o  */
  0x7C, /*  ooooo   */
  0x00  /*          */
};

/* The "unknown glyph" glyph, used as a last resort.  */
static struct grub_font_glyph *unknown_glyph;

/* The font structure used when no other font is loaded.  This functions
   as a "Null Object" pattern, so that code everywhere does not have to
   check for a NULL grub_font_t to avoid dereferencing a null pointer.  */
static struct grub_font null_font;

/* Flag to ensure module is initialized only once.  */
static grub_uint8_t font_loader_initialized;

void
grub_font_loader_init (void)
{
  /* Only initialize font loader once.  */
  if (font_loader_initialized)
    return;

  /* Make glyph for unknown glyph.  */
  unknown_glyph = grub_malloc(sizeof(struct grub_font_glyph)
                              + sizeof(unknown_glyph_bitmap));
  if (! unknown_glyph)
    return;

  unknown_glyph->width = 8;
  unknown_glyph->height = 16;
  unknown_glyph->offset_x = 0;
  unknown_glyph->offset_y = -3;
  unknown_glyph->device_width = 8;
  grub_memcpy(unknown_glyph->bitmap,
              unknown_glyph_bitmap, sizeof(unknown_glyph_bitmap));

  /* Initialize the null font.  */
  font_init (&null_font);
  null_font.name = "<No Font>";
  null_font.ascent = unknown_glyph->height-3;
  null_font.descent = 3;
  null_font.max_char_width = unknown_glyph->width;
  null_font.max_char_height = unknown_glyph->height;

  font_loader_initialized = 1;
}

/* Initialize the font object with initial default values.  */
static void
font_init (grub_font_t font)
{
  font->name = 0;
  font->file = 0;
  font->family = 0;
  font->point_size = 0;
  font->weight = 0;

  /* Default leading value, not in font file yet.  */
  font->leading = 1;

  font->max_char_width = 0;
  font->max_char_height = 0;
  font->ascent = 0;
  font->descent = 0;
  font->num_chars = 0;
  font->char_index = 0;
  font->bmp_idx = 0;
}

/* Open the next section in the file.

   On success, the section name is stored in section->name and the length in
   section->length, and 0 is returned.  On failure, 1 is returned and
   grub_errno is set appropriately with an error message.

   If 1 is returned due to being at the end of the file, then section->eof is
   set to 1; otherwise, section->eof is set to 0.  */
static int
open_section (grub_file_t file, struct font_file_section *section)
{
  grub_ssize_t retval;
  grub_uint32_t raw_length;

  section->file = file;
  section->eof = 0;

  /* Read the FOURCC section name.  */
  retval = grub_file_read (file, section->name, 4);
  if (retval >= 0 && retval < 4)
    {
      /* EOF encountered.  */
      section->eof = 1;
      return 1;
    }
  else if (retval < 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font format error: can't read section name");
      return 1;
    }

  /* Read the big-endian 32-bit section length.  */
  retval = grub_file_read (file, &raw_length, 4);
  if (retval >= 0 && retval < 4)
    {
      /* EOF encountered.  */
      section->eof = 1;
      return 1;
    }
  else if (retval < 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font format error: can't read section length");
      return 1;
    }

  /* Convert byte-order and store in *length.  */
  section->length = grub_be_to_cpu32 (raw_length);

  return 0;
}

/* Size in bytes of each character index (CHIX section)
   entry in the font file.  */
#define FONT_CHAR_INDEX_ENTRY_SIZE (4 + 1 + 4)

/* Load the character index (CHIX) section contents from the font file.  This
   presumes that the position of FILE is positioned immediately after the
   section length for the CHIX section (i.e., at the start of the section
   contents).  Returns 0 upon success, nonzero for failure (in which case
   grub_errno is set appropriately).  */
static int
load_font_index (grub_file_t file, grub_uint32_t sect_length, struct
                 grub_font *font)
{
  unsigned i;
  grub_uint32_t last_code;

#if FONT_DEBUG >= 2
  grub_printf("load_font_index(sect_length=%d)\n", sect_length);
#endif

  /* Sanity check: ensure section length is divisible by the entry size.  */
  if ((sect_length % FONT_CHAR_INDEX_ENTRY_SIZE) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font file format error: character index length %d "
                  "is not a multiple of the entry size %d",
                  sect_length, FONT_CHAR_INDEX_ENTRY_SIZE);
      return 1;
    }

  /* Calculate the number of characters.  */
  font->num_chars = sect_length / FONT_CHAR_INDEX_ENTRY_SIZE;

  /* Allocate the character index array.  */
  font->char_index = grub_malloc (font->num_chars
                                  * sizeof (struct char_index_entry));
  if (! font->char_index)
    return 1;
  font->bmp_idx = grub_malloc (0x10000 * sizeof (grub_uint16_t));
  if (! font->bmp_idx)
    {
      grub_free (font->char_index);
      return 1;
    }
  grub_memset (font->bmp_idx, 0xff, 0x10000 * sizeof (grub_uint16_t));


#if FONT_DEBUG >= 2
  grub_printf("num_chars=%d)\n", font->num_chars);
#endif

  last_code = 0;

  /* Load the character index data from the file.  */
  for (i = 0; i < font->num_chars; i++)
    {
      struct char_index_entry *entry = &font->char_index[i];

      /* Read code point value; convert to native byte order.  */
      if (grub_file_read (file, &entry->code, 4) != 4)
        return 1;
      entry->code = grub_be_to_cpu32 (entry->code);

      /* Verify that characters are in ascending order.  */
      if (i != 0 && entry->code <= last_code)
        {
          grub_error (GRUB_ERR_BAD_FONT,
                      "font characters not in ascending order: %u <= %u",
                      entry->code, last_code);
          return 1;
        }

      if (entry->code < 0x10000)
	font->bmp_idx[entry->code] = i;

      last_code = entry->code;

      /* Read storage flags byte.  */
      if (grub_file_read (file, &entry->storage_flags, 1) != 1)
        return 1;

      /* Read glyph data offset; convert to native byte order.  */
      if (grub_file_read (file, &entry->offset, 4) != 4)
        return 1;
      entry->offset = grub_be_to_cpu32 (entry->offset);

      /* No glyph loaded.  Will be loaded on demand and cached thereafter.  */
      entry->glyph = 0;

#if FONT_DEBUG >= 5
      /* Print the 1st 10 characters.  */
      if (i < 10)
        grub_printf("c=%d o=%d\n", entry->code, entry->offset);
#endif
    }

  return 0;
}

/* Read the contents of the specified section as a string, which is
   allocated on the heap.  Returns 0 if there is an error.  */
static char *
read_section_as_string (struct font_file_section *section)
{
  char *str;
  grub_ssize_t ret;

  str = grub_malloc (section->length + 1);
  if (! str)
    return 0;

  ret = grub_file_read (section->file, str, section->length);
  if (ret < 0 || ret != (grub_ssize_t) section->length)
    {
      grub_free (str);
      return 0;
    }

  str[section->length] = '\0';
  return str;
}

/* Read the contents of the current section as a 16-bit integer value,
   which is stored into *VALUE.
   Returns 0 upon success, nonzero upon failure.  */
static int
read_section_as_short (struct font_file_section *section, grub_int16_t *value)
{
  grub_uint16_t raw_value;

  if (section->length != 2)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font file format error: section %c%c%c%c length "
                  "is %d but should be 2",
                  section->name[0], section->name[1],
                  section->name[2], section->name[3],
                  section->length);
      return 1;
    }
  if (grub_file_read (section->file, &raw_value, 2) != 2)
    return 1;

  *value = grub_be_to_cpu16 (raw_value);
  return 0;
}

/* Load a font and add it to the beginning of the global font list.
   Returns 0 upon success, nonzero upon failure.  */
int
grub_font_load (const char *filename)
{
  grub_file_t file = 0;
  struct font_file_section section;
  char magic[4];
  grub_font_t font = 0;

#if FONT_DEBUG >= 1
  grub_printf("add_font(%s)\n", filename);
#endif

  file = grub_buffile_open (filename, 1024);
  if (!file)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("file opened\n");
#endif

  /* Read the FILE section.  It indicates the file format.  */
  if (open_section (file, &section) != 0)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("opened FILE section\n");
#endif
  if (grub_memcmp (section.name, section_names_file, 4) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font file format error: 1st section must be FILE");
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("section name ok\n");
#endif
  if (section.length != 4)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "font file format error (file type ID length is %d "
                  "but should be 4)", section.length);
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("section length ok\n");
#endif
  /* Check the file format type code.  */
  if (grub_file_read (file, magic, 4) != 4)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("read magic ok\n");
#endif

  if (grub_memcmp (magic, pff2_magic, 4) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT, "invalid font magic %x %x %x %x",
                  magic[0], magic[1], magic[2], magic[3]);
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("compare magic ok\n");
#endif

  /* Allocate the font object.  */
  font = (grub_font_t) grub_malloc (sizeof (struct grub_font));
  if (! font)
    goto fail;

  font_init (font);
  font->file = file;

#if FONT_DEBUG >= 3
  grub_printf("allocate font ok; loading font info\n");
#endif

  /* Load the font information.  */
  while (1)
    {
      if (open_section (file, &section) != 0)
        {
          if (section.eof)
            break;              /* Done reading the font file.  */
          else
            goto fail;
        }

#if FONT_DEBUG >= 2
      grub_printf("opened section %c%c%c%c ok\n",
                  section.name[0], section.name[1],
                  section.name[2], section.name[3]);
#endif

      if (grub_memcmp (section.name, section_names_font_name, 4) == 0)
        {
          font->name = read_section_as_string (&section);
          if (!font->name)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_point_size, 4) == 0)
        {
          if (read_section_as_short (&section, &font->point_size) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_weight, 4) == 0)
        {
          char *wt;
          wt = read_section_as_string (&section);
          if (!wt)
            continue;
          /* Convert the weight string 'normal' or 'bold' into a number.  */
          if (grub_strcmp (wt, "normal") == 0)
            font->weight = FONT_WEIGHT_NORMAL;
          else if (grub_strcmp (wt, "bold") == 0)
            font->weight = FONT_WEIGHT_BOLD;
          grub_free (wt);
        }
      else if (grub_memcmp (section.name, section_names_max_char_width, 4) == 0)
        {
          if (read_section_as_short (&section, &font->max_char_width) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_max_char_height, 4) == 0)
        {
          if (read_section_as_short (&section, &font->max_char_height) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_ascent, 4) == 0)
        {
          if (read_section_as_short (&section, &font->ascent) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_descent, 4) == 0)
        {
          if (read_section_as_short (&section, &font->descent) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_char_index, 4) == 0)
        {
          if (load_font_index (file, section.length, font) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_data, 4) == 0)
        {
          /* When the DATA section marker is reached, we stop reading.  */
          break;
        }
      else
        {
          /* Unhandled section type, simply skip past it.  */
#if FONT_DEBUG >= 3
          grub_printf("Unhandled section type, skipping.\n");
#endif
          grub_off_t section_end = grub_file_tell (file) + section.length;
          if ((int) grub_file_seek (file, section_end) == -1)
            goto fail;
        }
    }

  if (! font->name)
    {
      grub_printf ("Note: Font has no name.\n");
      font->name = grub_strdup ("Unknown");
    }

#if FONT_DEBUG >= 1
  grub_printf ("Loaded font `%s'.\n"
               "Ascent=%d Descent=%d MaxW=%d MaxH=%d Number of characters=%d.\n",
               font->name,
               font->ascent, font->descent,
               font->max_char_width, font->max_char_height,
               font->num_chars);
#endif

  if (font->max_char_width == 0
      || font->max_char_height == 0
      || font->num_chars == 0
      || font->char_index == 0
      || font->ascent == 0
      || font->descent == 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "invalid font file: missing some required data");
      goto fail;
    }

  /* Add the font to the global font registry.  */
  if (register_font (font) != 0)
    goto fail;

  return 0;

fail:
  free_font (font);
  return 1;
}

/* Read a 16-bit big-endian integer from FILE, convert it to native byte
   order, and store it in *VALUE.
   Returns 0 on success, 1 on failure.  */
static int
read_be_uint16 (grub_file_t file, grub_uint16_t * value)
{
  if (grub_file_read (file, value, 2) != 2)
    return 1;
  *value = grub_be_to_cpu16 (*value);
  return 0;
}

static int
read_be_int16 (grub_file_t file, grub_int16_t * value)
{
  /* For the signed integer version, use the same code as for unsigned.  */
  return read_be_uint16 (file, (grub_uint16_t *) value);
}

/* Return a pointer to the character index entry for the glyph corresponding to
   the codepoint CODE in the font FONT.  If not found, return zero.  */
static inline struct char_index_entry *
find_glyph (const grub_font_t font, grub_uint32_t code)
{
  struct char_index_entry *table;
  grub_size_t lo;
  grub_size_t hi;
  grub_size_t mid;

  table = font->char_index;

  /* Use BMP index if possible.  */
  if (code < 0x10000)
    {
      if (font->bmp_idx[code] == 0xffff)
	return 0;
      return &table[font->bmp_idx[code]];
    }

  /* Do a binary search in `char_index', which is ordered by code point.  */
  lo = 0;
  hi = font->num_chars - 1;

  if (! table)
    return 0;

  while (lo <= hi)
    {
      mid = lo + (hi - lo) / 2;
      if (code < table[mid].code)
        hi = mid - 1;
      else if (code > table[mid].code)
        lo = mid + 1;
      else
        return &table[mid];
    }

  return 0;
}

/* Get a glyph for the Unicode character CODE in FONT.  The glyph is loaded
   from the font file if has not been loaded yet.
   Returns a pointer to the glyph if found, or 0 if it is not found.  */
static struct grub_font_glyph *
grub_font_get_glyph_internal (grub_font_t font, grub_uint32_t code)
{
  struct char_index_entry *index_entry;

  index_entry = find_glyph (font, code);
  if (index_entry)
    {
      struct grub_font_glyph *glyph = 0;
      grub_uint16_t width;
      grub_uint16_t height;
      grub_int16_t xoff;
      grub_int16_t yoff;
      grub_int16_t dwidth;
      int len;

      if (index_entry->glyph)
        /* Return cached glyph.  */
        return index_entry->glyph;

      if (! font->file)
        /* No open file, can't load any glyphs.  */
        return 0;

      /* Make sure we can find glyphs for error messages.  Push active
         error message to error stack and reset error message.  */
      grub_error_push ();

      grub_file_seek (font->file, index_entry->offset);

      /* Read the glyph width, height, and baseline.  */
      if (read_be_uint16(font->file, &width) != 0
          || read_be_uint16(font->file, &height) != 0
          || read_be_int16(font->file, &xoff) != 0
          || read_be_int16(font->file, &yoff) != 0
          || read_be_int16(font->file, &dwidth) != 0)
        {
          remove_font (font);
          return 0;
        }

      len = (width * height + 7) / 8;
      glyph = grub_malloc (sizeof (struct grub_font_glyph) + len);
      if (! glyph)
        {
          remove_font (font);
          return 0;
        }

      glyph->font = font;
      glyph->width = width;
      glyph->height = height;
      glyph->offset_x = xoff;
      glyph->offset_y = yoff;
      glyph->device_width = dwidth;

      /* Don't try to read empty bitmaps (e.g., space characters).  */
      if (len != 0)
        {
          if (grub_file_read (font->file, glyph->bitmap, len) != len)
            {
              remove_font (font);
              return 0;
            }
        }

      /* Restore old error message.  */
      grub_error_pop ();

      /* Cache the glyph.  */
      index_entry->glyph = glyph;

      return glyph;
    }

  return 0;
}

/* Free the memory used by FONT.
   This should not be called if the font has been made available to
   users (once it is added to the global font list), since there would
   be the possibility of a dangling pointer.  */
static void
free_font (grub_font_t font)
{
  if (font)
    {
      if (font->file)
        grub_file_close (font->file);
      grub_free (font->name);
      grub_free (font->family);
      grub_free (font->char_index);
      grub_free (font);
    }
}

/* Add FONT to the global font registry.
   Returns 0 upon success, nonzero on failure
   (the font was not registered).  */
static int
register_font (grub_font_t font)
{
  struct grub_font_node *node = 0;

  node = grub_malloc (sizeof (struct grub_font_node));
  if (! node)
    return 1;

  node->value = font;
  node->next = grub_font_list;
  grub_font_list = node;

  return 0;
}

/* Remove the font from the global font list.  We don't actually free the
   font's memory since users could be holding references to the font.  */
static void
remove_font (grub_font_t font)
{
  struct grub_font_node **nextp, *cur;

  for (nextp = &grub_font_list, cur = *nextp;
       cur;
       nextp = &cur->next, cur = cur->next)
    {
      if (cur->value == font)
        {
          *nextp = cur->next;

          /* Free the node, but not the font itself.  */
          grub_free (cur);

          return;
        }
    }
}

/* Get a font from the list of loaded fonts.  This function will return
   another font if the requested font is not available.  If no fonts are
   loaded, then a special 'null font' is returned, which contains no glyphs,
   but is not a null pointer so the caller may omit checks for NULL.  */
grub_font_t
grub_font_get (const char *font_name)
{
  struct grub_font_node *node;

  for (node = grub_font_list; node; node = node->next)
    {
      grub_font_t font = node->value;
      if (grub_strcmp (font->name, font_name) == 0)
        return font;
    }

  /* If no font by that name is found, return the first font in the list
     as a fallback.  */
  if (grub_font_list && grub_font_list->value)
    return grub_font_list->value;
  else
    /* The null_font is a last resort.  */
    return &null_font;
}

/* Get the full name of the font.  For instance, "Helvetica Bold 12".  */
const char *
grub_font_get_name (grub_font_t font)
{
  return font->name;
}

/* Get the maximum width of any character in the font in pixels.  */
int
grub_font_get_max_char_width (grub_font_t font)
{
  return font->max_char_width;
}

/* Get the maximum height of any character in the font in pixels.  */
int
grub_font_get_max_char_height (grub_font_t font)
{
  return font->max_char_height;
}

/* Get the distance in pixels from the top of characters to the baseline.  */
int
grub_font_get_ascent (grub_font_t font)
{
  return font->ascent;
}

/* Get the distance in pixels from the baseline to the lowest descenders
   (for instance, in a lowercase 'y', 'g', etc.).  */
int
grub_font_get_descent (grub_font_t font)
{
  return font->descent;
}

/* FIXME: not correct for all fonts.  */
int
grub_font_get_xheight (grub_font_t font)
{
  return font->ascent / 2;
}

/* Get the *standard leading* of the font in pixel, which is the spacing
   between two lines of text.  Specifically, it is the space between the
   descent of one line and the ascent of the next line.  This is included
   in the *height* metric.  */
int
grub_font_get_leading (grub_font_t font)
{
  return font->leading;
}

/* Get the distance in pixels between baselines of adjacent lines of text.  */
int
grub_font_get_height (grub_font_t font)
{
  return font->ascent + font->descent + font->leading;
}

/* Get the width in pixels of the specified UTF-8 string, when rendered in
   in the specified font (but falling back on other fonts for glyphs that
   are missing).  */
int
grub_font_get_string_width (grub_font_t font, const char *str)
{
  int width;
  struct grub_font_glyph *glyph;
  grub_uint32_t code;
  const grub_uint8_t *ptr;

  for (ptr = (const grub_uint8_t *) str, width = 0;
       grub_utf8_to_ucs4 (&code, 1, ptr, -1, &ptr) > 0; )
    {
      glyph = grub_font_get_glyph_with_fallback (font, code);
      if (glyph)
	width += glyph->device_width;
      else
	width += unknown_glyph->device_width;
    }

  return width;
}

/* Get the glyph for FONT corresponding to the Unicode code point CODE.
   Returns a pointer to an glyph indicating there is no glyph available
   if CODE does not exist in the font.  The glyphs are cached once loaded.  */
struct grub_font_glyph *
grub_font_get_glyph (grub_font_t font, grub_uint32_t code)
{
  struct grub_font_glyph *glyph;
  glyph = grub_font_get_glyph_internal (font, code);
  if (glyph == 0)
    glyph = unknown_glyph;
  return glyph;
}


/* Calculate a subject value representing "how similar" two fonts are.
   This is used to prioritize the order that fonts are scanned for missing
   glyphs.  The object is to select glyphs from the most similar font
   possible, for the best appearance.
   The heuristic is crude, but it helps greatly when fonts of similar
   sizes are used so that tiny 8 point glyphs are not mixed into a string
   of 24 point text unless there is no other choice.  */
static int
get_font_diversity(grub_font_t a, grub_font_t b)
{
  int d;

  d = 0;

  if (a->ascent && b->ascent)
    d += grub_abs (a->ascent - b->ascent) * 8;
  else
    /* Penalty for missing attributes.  */
    d += 50;

  if (a->max_char_height && b->max_char_height)
    d += grub_abs (a->max_char_height - b->max_char_height) * 8;
  else
    /* Penalty for missing attributes.  */
    d += 50;

  /* Weight is a minor factor. */
  d += (a->weight != b->weight) ? 5 : 0;

  return d;
}

/* Get a glyph corresponding to the codepoint CODE.  If FONT contains the
   specified glyph, then it is returned.  Otherwise, all other loaded fonts
   are searched until one is found that contains a glyph for CODE.
   If no glyph is available for CODE in the loaded fonts, then a glyph
   representing an unknown character is returned.
   This function never returns NULL.
   The returned glyph is owned by the font manager and should not be freed
   by the caller.  The glyphs are cached.  */
struct grub_font_glyph *
grub_font_get_glyph_with_fallback (grub_font_t font, grub_uint32_t code)
{
  struct grub_font_glyph *glyph;
  struct grub_font_node *node;
  /* Keep track of next node, in case there's an I/O error in
     grub_font_get_glyph_internal() and the font is removed from the list.  */
  struct grub_font_node *next;
  /* Information on the best glyph found so far, to help find the glyph in
     the best matching to the requested one.  */
  int best_diversity;
  struct grub_font_glyph *best_glyph;

  if (font)
    {
      /* First try to get the glyph from the specified font.  */
      glyph = grub_font_get_glyph_internal (font, code);
      if (glyph)
        return glyph;
    }

  /* Otherwise, search all loaded fonts for the glyph and use the one from
     the font that best matches the requested font.  */
  best_diversity = 10000;
  best_glyph = 0;

  for (node = grub_font_list; node; node = next)
    {
      grub_font_t curfont;

      curfont = node->value;
      next = node->next;

      glyph = grub_font_get_glyph_internal (curfont, code);
      if (glyph)
        {
          int d;

          d = get_font_diversity (curfont, font);
          if (d < best_diversity)
            {
              best_diversity = d;
              best_glyph = glyph;
            }
        }
    }

  return best_glyph;
}

static struct grub_font_glyph *
grub_font_dup_glyph (struct grub_font_glyph *glyph)
{
  static struct grub_font_glyph *ret;
  ret = grub_malloc (sizeof (*ret) + (glyph->width * glyph->height + 7) / 8);
  if (!ret)
    return NULL;
  grub_memcpy (ret, glyph, sizeof (*ret)
	       + (glyph->width * glyph->height + 7) / 8);
  return ret;
}

/* FIXME: suboptimal.  */
static void
grub_font_blit_glyph (struct grub_font_glyph *target,
		      struct grub_font_glyph *src,
		      unsigned dx, unsigned dy)
{
  unsigned src_bit, tgt_bit, src_byte, tgt_byte;
  unsigned i, j;
  for (i = 0; i < src->height; i++)
    {
      src_bit = (src->width * i) % 8;
      src_byte = (src->width * i) / 8;
      tgt_bit = (target->width * (dy + i) + dx) % 8;
      tgt_byte = (target->width * (dy + i) + dx) / 8;
      for (j = 0; j < src->width; j++)
	{
	  target->bitmap[tgt_byte] |= ((src->bitmap[src_byte] << src_bit)
				       & 0x80) >> tgt_bit;
	  src_bit++;
	  tgt_bit++;
	  if (src_bit == 8)
	    {
	      src_byte++;
	      src_bit = 0;
	    }
	  if (tgt_bit == 8)
	    {
	      tgt_byte++;
	      tgt_bit = 0;
	    }
	}
    }
}

static void
grub_font_blit_glyph_mirror (struct grub_font_glyph *target,
			     struct grub_font_glyph *src,
			     unsigned dx, unsigned dy)
{
  unsigned tgt_bit, src_byte, tgt_byte;
  signed src_bit;
  unsigned i, j;
  for (i = 0; i < src->height; i++)
    {
      src_bit = (src->width * i + src->width - 1) % 8;
      src_byte = (src->width * i + src->width - 1) / 8;
      tgt_bit = (target->width * (dy + i) + dx) % 8;
      tgt_byte = (target->width * (dy + i) + dx) / 8;
      for (j = 0; j < src->width; j++)
	{
	  target->bitmap[tgt_byte] |= ((src->bitmap[src_byte] << src_bit)
				       & 0x80) >> tgt_bit;
	  src_bit--;
	  tgt_bit++;
	  if (src_bit == -1)
	    {
	      src_byte--;
	      src_bit = 7;
	    }
	  if (tgt_bit == 8)
	    {
	      tgt_byte++;
	      tgt_bit = 0;
	    }
	}
    }
}

static inline enum grub_comb_type
get_comb_type (grub_uint32_t c)
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

  return GRUB_BIDI_TYPE_L;
}

static void
blit_comb (const struct grub_unicode_glyph *glyph_id,
	   struct grub_font_glyph *glyph,
	   struct grub_video_signed_rect *bounds_out,
	   struct grub_font_glyph *main_glyph,
	   struct grub_font_glyph **combining_glyphs)
{
  struct grub_video_signed_rect bounds;
  unsigned i;
  signed above_rightx, above_righty;
  auto void NESTED_FUNC_ATTR do_blit (struct grub_font_glyph *src,
				      signed dx, signed dy);
  void NESTED_FUNC_ATTR do_blit (struct grub_font_glyph *src,
				 signed dx, signed dy)
  {
    if (glyph)
      grub_font_blit_glyph (glyph, src, dx - glyph->offset_x,
			    (glyph->height + glyph->offset_y) + dy);
    if (dx < bounds.x)
      {
	bounds.width += bounds.x - dx;
	bounds.x = dx;
      }
    if (bounds.y > -src->height - dy)
      {
	bounds.height += bounds.y - (-src->height - dy);
	bounds.y = (-src->height - dy);
      }
    if (dx + src->width - bounds.x >= (signed) bounds.width)
      bounds.width = dx + src->width - bounds.x + 1;
    if ((signed) bounds.height < src->height + (-src->height - dy) - bounds.y)
      bounds.height = src->height + (-src->height - dy) - bounds.y;
  }

  auto void minimal_device_width (int val);
  void minimal_device_width (int val)
  {
    if (glyph && glyph->device_width < val)
      glyph->device_width = val;
  }
  auto void add_device_width (int val);
  void add_device_width (int val)
  {
    if (glyph)
      glyph->device_width += val;
  }

  bounds.x = main_glyph->offset_x;
  bounds.y = main_glyph->offset_y;
  bounds.width = main_glyph->width;
  bounds.height = main_glyph->height;

  above_rightx = bounds.x + bounds.width;
  above_righty = bounds.y + bounds.height;

  for (i = 0; i < glyph_id->ncomb; i++)
    {
      enum grub_comb_type combtype;
      grub_int16_t space = 0;
      grub_int16_t centerx = (bounds.width - combining_glyphs[i]->width) / 2
	+ bounds.x;
      
      if (!combining_glyphs[i])
	continue;
      combtype = get_comb_type (glyph_id->combining[i]);
      switch (combtype)
	{
	case GRUB_UNICODE_COMB_OVERLAY:
	  do_blit (combining_glyphs[i],
		   centerx,
		   (bounds.height - combining_glyphs[i]->height) / 2
		   -(bounds.height + bounds.y));
	  minimal_device_width (combining_glyphs[i]->width);
	  break;

	case GRUB_UNICODE_COMB_ABOVE_RIGHT:
	  do_blit (combining_glyphs[i], above_rightx,
		   -(above_righty + space
		     + combining_glyphs[i]->height));
	  above_rightx += combining_glyphs[i]->width;
	  minimal_device_width (above_rightx);
	  break;

	case GRUB_UNICODE_STACK_ABOVE:
	  space = combining_glyphs[i]->offset_y
	    - grub_font_get_xheight (combining_glyphs[i]->font);
	  if (space < 0)
	    space = 1 + (grub_font_get_xheight (main_glyph->font)) / 8;
	    
	case GRUB_UNICODE_STACK_ATTACHED_ABOVE:	    
	  do_blit (combining_glyphs[i], centerx,
		   -(bounds.height + bounds.y + space
		     + combining_glyphs[i]->height));
	  minimal_device_width (combining_glyphs[i]->width);
	  break;

	case GRUB_UNICODE_STACK_BELOW:
	  space = -(combining_glyphs[i]->offset_y 
		    + combining_glyphs[i]->height);
	  if (space < 0)
	    space = 1 + (grub_font_get_xheight (main_glyph->font)) / 8;
	  
	case GRUB_UNICODE_STACK_ATTACHED_BELOW:
	  do_blit (combining_glyphs[i], centerx,
		   -(bounds.y - space));
	  minimal_device_width (combining_glyphs[i]->width);
	  break;

	default:
	  {
	    /* Default handling. Just draw combining character on top
	       of base character.
	       FIXME: support more unicode types correctly.
	    */
	    do_blit (combining_glyphs[i],
		     main_glyph->device_width
		     + combining_glyphs[i]->offset_x,  
		     - (combining_glyphs[i]->height 
			+ combining_glyphs[i]->offset_y));
	    add_device_width (combining_glyphs[i]->device_width);
	  }
	}
    }
  if (bounds_out)
    *bounds_out = bounds;
}

static struct grub_font_glyph *
grub_font_construct_glyph (grub_font_t hinted_font,
			   const struct grub_unicode_glyph *glyph_id)
{
  grub_font_t font;
  struct grub_video_signed_rect bounds;
  struct grub_font_glyph *main_glyph;
  struct grub_font_glyph **combining_glyphs;
  struct grub_font_glyph *glyph;

  main_glyph = grub_font_get_glyph_with_fallback (hinted_font, glyph_id->base);

  if (!main_glyph)
    {
      /* Glyph not available in any font.  Return unknown glyph.  */
      return grub_font_dup_glyph (unknown_glyph);
    }

  if (!glyph_id->ncomb && !glyph_id->attributes)
    return grub_font_dup_glyph (main_glyph);

  combining_glyphs = grub_malloc (sizeof (combining_glyphs[0])
				  * glyph_id->ncomb);
  if (glyph_id->ncomb && !combining_glyphs)
    {
      grub_errno = GRUB_ERR_NONE;
      return grub_font_dup_glyph (main_glyph);
    }
  
  font = main_glyph->font;

  {
    unsigned i;
    for (i = 0; i < glyph_id->ncomb; i++)
      combining_glyphs[i]
	= grub_font_get_glyph_with_fallback (font, glyph_id->combining[i]);
  }

  blit_comb (glyph_id, NULL, &bounds, main_glyph, combining_glyphs);

  glyph = grub_zalloc (sizeof (*glyph) + (bounds.width * bounds.height + 7) / 8);
  if (!glyph)
    {
      grub_errno = GRUB_ERR_NONE;
      return grub_font_dup_glyph (main_glyph);
    }

  glyph->font = font;
  glyph->width = bounds.width;
  glyph->height = bounds.height;
  glyph->offset_x = bounds.x;
  glyph->offset_y = bounds.y;
  glyph->device_width = main_glyph->device_width;

  if (glyph_id->attributes & GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR)
    grub_font_blit_glyph_mirror (glyph, main_glyph,
				 main_glyph->offset_x - glyph->offset_x,
				 (glyph->height + glyph->offset_y)
				 - (main_glyph->height + main_glyph->offset_y));
  else
    grub_font_blit_glyph (glyph, main_glyph,
			  main_glyph->offset_x - glyph->offset_x,
			  (glyph->height + glyph->offset_y)
			  - (main_glyph->height + main_glyph->offset_y));

  blit_comb (glyph_id, glyph, NULL, main_glyph, combining_glyphs);

  return glyph;
}

/* Draw the specified glyph at (x, y).  The y coordinate designates the
   baseline of the character, while the x coordinate designates the left
   side location of the character.  */
grub_err_t
grub_font_draw_glyph (struct grub_font_glyph *glyph,
                      grub_video_color_t color,
                      int left_x, int baseline_y)
{
  struct grub_video_bitmap glyph_bitmap;

  /* Don't try to draw empty glyphs (U+0020, etc.).  */
  if (glyph->width == 0 || glyph->height == 0)
    return GRUB_ERR_NONE;

  glyph_bitmap.mode_info.width = glyph->width;
  glyph_bitmap.mode_info.height = glyph->height;
  glyph_bitmap.mode_info.mode_type =
    (1 << GRUB_VIDEO_MODE_TYPE_DEPTH_POS)
    | GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP;
  glyph_bitmap.mode_info.blit_format = GRUB_VIDEO_BLIT_FORMAT_1BIT_PACKED;
  glyph_bitmap.mode_info.bpp = 1;

  /* Really 1 bit per pixel.  */
  glyph_bitmap.mode_info.bytes_per_pixel = 0;

  /* Packed densely as bits.  */
  glyph_bitmap.mode_info.pitch = glyph->width;

  glyph_bitmap.mode_info.number_of_colors = 2;
  glyph_bitmap.mode_info.bg_red = 0;
  glyph_bitmap.mode_info.bg_green = 0;
  glyph_bitmap.mode_info.bg_blue = 0;
  glyph_bitmap.mode_info.bg_alpha = 0;
  grub_video_unmap_color(color,
                         &glyph_bitmap.mode_info.fg_red,
                         &glyph_bitmap.mode_info.fg_green,
                         &glyph_bitmap.mode_info.fg_blue,
                         &glyph_bitmap.mode_info.fg_alpha);
  glyph_bitmap.data = glyph->bitmap;

  int bitmap_left = left_x + glyph->offset_x;
  int bitmap_bottom = baseline_y - glyph->offset_y;
  int bitmap_top = bitmap_bottom - glyph->height;

  return grub_video_blit_bitmap (&glyph_bitmap, GRUB_VIDEO_BLIT_BLEND,
                                 bitmap_left, bitmap_top,
                                 0, 0,
                                 glyph->width, glyph->height);
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

static grub_ssize_t
grub_err_bidi_logical_to_visual (grub_uint32_t *logical,
				 grub_size_t logical_len,
				 struct grub_unicode_glyph **visual_out)
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

  auto void revert (unsigned start, unsigned end);
  void revert (unsigned start, unsigned end)
  {
    struct grub_unicode_glyph t;
    unsigned k, tl;
    for (k = 0; k <= (end - start) / 2; k++)
      {
	t = visual[start+k];
	visual[start+k] = visual[end-k];
	visual[end-k] = t;
	tl = levels[start+k];
	levels[start+k] = levels[end-k];
	levels[end-k] = tl;
      }
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
    unsigned last_comb_pointer = 0;
    for (i = 0; i < logical_len; i++)
      {
	/* Variation selectors >= 17 are outside of BMP and SMP. 
	   Handle variation selectors first to avoid potentially costly lookups.
	*/
	if (logical[i] >= GRUB_UNICODE_VARIATION_SELECTOR_1
	    && logical[i] <= GRUB_UNICODE_VARIATION_SELECTOR_16)
	  {
	    if (!visual_len)
	      continue;
	    visual[visual_len - 1].variant
	      = logical[i] - GRUB_UNICODE_VARIATION_SELECTOR_1 + 1;
	  }
	if (logical[i] >= GRUB_UNICODE_VARIATION_SELECTOR_17
	    && logical[i] <= GRUB_UNICODE_VARIATION_SELECTOR_256)
	  {
	    if (!visual_len)
	      continue;
	    visual[visual_len - 1].variant
	      = logical[i] - GRUB_UNICODE_VARIATION_SELECTOR_17 + 17;
	    continue;
	  }
	
	type = get_bidi_type (logical[i]);
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
	      enum grub_comb_type comb_type;
	      comb_type = get_comb_type (logical[i]);
	      if (comb_type)
		{
		  grub_uint32_t *n;
		  unsigned j;

		  if (!visual_len)
		    break;
		  if (comb_type == GRUB_UNICODE_COMB_MC
		      || comb_type == GRUB_UNICODE_COMB_ME
		      || comb_type == GRUB_UNICODE_COMB_MN)
		    last_comb_pointer = visual[visual_len - 1].ncomb;
		  n = grub_realloc (visual[visual_len - 1].combining, 
				    sizeof (grub_uint32_t)
				    * (visual[visual_len - 1].ncomb + 1));
		  if (!n)
		    {
		      grub_errno = GRUB_ERR_NONE;
		      break;
		    }

		  for (j = last_comb_pointer;
		       j < visual[visual_len - 1].ncomb; j++)
		    if (get_comb_type (visual[visual_len - 1].combining[j])
			> comb_type)
		      break;
		  grub_memmove (visual[visual_len - 1].combining + j + 1,
				visual[visual_len - 1].combining + j,
				(visual[visual_len - 1].ncomb - j)
				* sizeof (visual[visual_len - 1].combining[0]));
		  visual[visual_len - 1].combining = n;
		  visual[visual_len - 1].combining[j] = logical[i];
		  visual[visual_len - 1].ncomb++;
		  break;
		}
	      last_comb_pointer = 0;
	      levels[visual_len] = cur_level;
	      if (cur_override != OVERRIDE_NEUTRAL)
		resolved_types[visual_len] = 
		  (cur_override == OVERRIDE_L) ? GRUB_BIDI_TYPE_L
		  : GRUB_BIDI_TYPE_R;
	      else
		resolved_types[visual_len] = type;
	      visual[visual_len].base = logical[i];
	      visual[visual_len].variant = 0;
	      visual[visual_len].attributes = 0;
	      visual[visual_len].ncomb = 0;
	      visual[visual_len].combining = NULL;
	      visual_len++;
	    }
	  }
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
  /* TODO: put line-wrapping here.  */
  {
    unsigned min_odd_level = 0xffffffff;
    unsigned max_level = 0;
    unsigned j;
    for (i = 0; i < visual_len; i++)
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
	for (i = 0; i < visual_len; i++)
	  {
	    if (i != 0 && levels[i] >= j && levels[i-1] < j)
	      in = i;
	    if (levels[i] >= j && (i + 1 == visual_len || levels[i+1] < j))
	      revert (in, i);
	  }
      }
  }

  for (i = 0; i < visual_len; i++)
    if (is_mirrored (visual[i].base) && levels[i])
      visual[i].attributes |= GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR;

  grub_free (levels);

  *visual_out = visual;
  return visual_len;
}

/* Draw a UTF-8 string of text on the current video render target.
   The x coordinate specifies the starting x position for the first character,
   while the y coordinate specifies the baseline position.
   If the string contains a character that FONT does not contain, then
   a glyph from another loaded font may be used instead.  */
grub_err_t
grub_font_draw_string (const char *str, grub_font_t font,
                       grub_video_color_t color,
                       int left_x, int baseline_y)
{
  int x;
  struct grub_font_glyph *glyph;
  grub_uint32_t *logical;
  grub_ssize_t logical_len, visual_len;
  struct grub_unicode_glyph *visual, *ptr;

  logical_len = grub_utf8_to_ucs4_alloc (str, &logical, 0);
  if (logical_len < 0)
    return grub_errno;

  visual_len = grub_err_bidi_logical_to_visual (logical, logical_len, &visual);
  grub_free (logical);
  if (visual_len < 0)
    return grub_errno;

  for (ptr = visual, x = left_x; ptr < visual + visual_len; ptr++)
    {
      grub_err_t err;
      glyph = grub_font_construct_glyph (font, ptr);
      if (!glyph)
	return grub_errno;
      err = grub_font_draw_glyph (glyph, color, x, baseline_y);
      grub_free (glyph);
      if (err)
	return err;
      x += glyph->device_width;
    }

  grub_free (visual);

  return GRUB_ERR_NONE;
}

