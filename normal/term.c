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

/* The amount of lines counted by the pager.  */
static unsigned grub_more_lines;

/* If the more pager is active.  */
static int grub_more;

static void
process_newline (void)
{
  struct grub_term_output *cur;
  unsigned height = -1;

  FOR_ACTIVE_TERM_OUTPUTS(cur)
    if (grub_term_height (cur) < height)
      height = grub_term_height (cur);
  grub_more_lines++;

  if (grub_more && grub_more_lines == height - 1)
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
	grub_more_lines--;
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
  grub_newline_hook = process_newline;
}

void
grub_puts_terminal (const char *str, struct grub_term_output *term)
{
  grub_uint32_t code;
  grub_ssize_t ret;
  const grub_uint8_t *ptr = (const grub_uint8_t *) str;
  const grub_uint8_t *end;
  end = (const grub_uint8_t *) (str + grub_strlen (str));

  while (*ptr)
    {
      ret = grub_utf8_to_ucs4 (&code, 1, ptr, end - ptr, &ptr);
      grub_putcode (code, term);	
    }
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
