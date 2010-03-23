/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/file.h>
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/charset.h>

struct term_state
{
  struct term_state *next;
  struct grub_unicode_glyph *backlog;
  int numlines;
  char *term_name;
};

/* The amount of lines counted by the pager.  */
static unsigned grub_more_lines;

/* If the more pager is active.  */
static int grub_more;

static int grub_normal_char_counter = 0;

int
grub_normal_get_char_counter (void)
{
  return grub_normal_char_counter;
}

static void
process_newline (void)
{
  struct grub_term_output *cur;
  unsigned height = -1;

  FOR_ACTIVE_TERM_OUTPUTS(cur)
    if (grub_term_height (cur) < height)
      height = grub_term_height (cur);
  grub_more_lines++;

  if (grub_more && grub_more_lines >= height - 1)
    {
      char key;
      grub_uint16_t *pos;

      pos = grub_term_save_pos ();

      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
      grub_printf ("--MORE--");
      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

      key = grub_getkey ();

      /* Remove the message.  */
      grub_term_restore_pos (pos);
      grub_printf ("        ");
      grub_term_restore_pos (pos);

      /* Scroll one lines or an entire page, depending on the key.  */
      if (key == '\r' || key =='\n')
	grub_more_lines = height - 2;
      else
	grub_more_lines = 0;
    }
}

void
grub_set_more (int onoff)
{
  if (onoff == 1)
    grub_more++;
  else
    grub_more--;

  grub_more_lines = 0;
}

void
grub_install_newline_hook (void)
{
  grub_newline_hook = process_newline;
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

void
grub_puts_terminal (const char *str, struct grub_term_output *term)
{
  grub_uint32_t *unicode_str, *unicode_last_position;
  grub_utf8_to_ucs4_alloc (str, &unicode_str,
			   &unicode_last_position);

  grub_print_ucs4 (unicode_str, unicode_last_position, 0, 0, term);
  grub_free (unicode_str);
}

grub_uint16_t *
grub_term_save_pos (void)
{
  struct grub_term_output *cur;
  unsigned cnt = 0;
  grub_uint16_t *ret, *ptr;
  
  FOR_ACTIVE_TERM_OUTPUTS(cur)
    cnt++;

  ret = grub_malloc (cnt * sizeof (ret[0]));
  if (!ret)
    return NULL;

  ptr = ret;
  FOR_ACTIVE_TERM_OUTPUTS(cur)
    *ptr++ = grub_term_getxy (cur);

  return ret;
}

void
grub_term_restore_pos (grub_uint16_t *pos)
{
  struct grub_term_output *cur;
  grub_uint16_t *ptr = pos;

  if (!pos)
    return;

  FOR_ACTIVE_TERM_OUTPUTS(cur)
  {
    grub_term_gotoxy (cur, (*ptr & 0xff00) >> 8, *ptr & 0xff);
    ptr++;
  }
}

static void 
grub_terminal_autoload_free (void)
{
  struct grub_term_autoload *cur, *next;
  unsigned i;
  for (i = 0; i < 2; i++)
    for (cur = i ? grub_term_input_autoload : grub_term_output_autoload;
	 cur; cur = next)
      {
	next = cur->next;
	grub_free (cur->name);
	grub_free (cur->modname);
	grub_free (cur);
      }
  grub_term_input_autoload = NULL;
  grub_term_output_autoload = NULL;
}

/* Read the file terminal.lst for auto-loading.  */
void
read_terminal_list (void)
{
  const char *prefix;
  char *filename;
  grub_file_t file;
  char *buf = NULL;

  prefix = grub_env_get ("prefix");
  if (!prefix)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  
  filename = grub_xasprintf ("%s/terminal.lst", prefix);
  if (!filename)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  file = grub_file_open (filename);
  grub_free (filename);
  if (!file)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  /* Override previous terminal.lst.  */
  grub_terminal_autoload_free ();

  for (;; grub_free (buf))
    {
      char *p, *name;
      struct grub_term_autoload *cur;
      struct grub_term_autoload **target = NULL;
      
      buf = grub_file_getline (file);
	
      if (! buf)
	break;

      switch (buf[0])
	{
	case 'i':
	  target = &grub_term_input_autoload;
	  break;

	case 'o':
	  target = &grub_term_output_autoload;
	  break;
	}
      if (!target)
	continue;
      
      name = buf + 1;
            
      p = grub_strchr (name, ':');
      if (! p)
	continue;
      
      *p = '\0';
      while (*++p == ' ')
	;

      cur = grub_malloc (sizeof (*cur));
      if (!cur)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      
      cur->name = grub_strdup (name);
      if (! name)
	{
	  grub_errno = GRUB_ERR_NONE;
	  grub_free (cur);
	  continue;
	}
	
      cur->modname = grub_strdup (p);
      if (! cur->modname)
	{
	  grub_errno = GRUB_ERR_NONE;
	  grub_free (cur);
	  grub_free (cur->name);
	  continue;
	}
      cur->next = *target;
      *target = cur;
    }
  
  grub_file_close (file);

  grub_errno = GRUB_ERR_NONE;
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

  grub_normal_char_counter++;

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
	      if ((term->flags & GRUB_TERM_CODE_TYPE_MASK)
		  == GRUB_TERM_CODE_TYPE_UTF8_VISUAL
		  && (c->attributes & GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR))
		code = grub_unicode_mirror_code (code);
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
  grub_uint32_t *unicode_str, *unicode_last_position;
  grub_utf8_to_ucs4_alloc (str, &unicode_str,
			   &unicode_last_position);

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_print_ucs4 (unicode_str, unicode_last_position, 0, 0, term);
  }
  grub_free (unicode_str);
}
