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
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>

struct grub_handler_class grub_term_input_class =
  {
    .name = "terminal_input"
  };

#define grub_cur_term_input	grub_term_get_current_input ()

struct grub_term_output *grub_term_outputs;

void (*grub_newline_hook) (void) = NULL;

/* Put a Unicode character.  */
void
grub_putcode (grub_uint32_t code, struct grub_term_output *term)
{
  int height;

  height = grub_term_height (term);

  if (code == '\t' && term->getxy)
    {
      int n;

      n = 8 - ((term->getxy () >> 8) & 7);
      while (n--)
	grub_putcode (' ', term);

      return;
    }

  if (code == '\n')
    (term->putchar) ('\r');
  (term->putchar) (code);
}

/* Put a character. C is one byte of a UTF-8 stream.
   This function gathers bytes until a valid Unicode character is found.  */
void
grub_putchar (int c)
{
  static grub_size_t size = 0;
  static grub_uint8_t buf[6];
  grub_uint32_t code;
  grub_ssize_t ret;

  buf[size++] = c;
  ret = grub_utf8_to_ucs4 (&code, 1, buf, size, 0);

  if (ret != 0)
    {
      struct grub_term_output *term;
      size = 0;
      for (term = grub_term_outputs; term; term = term->next)
	if (grub_term_is_active (term))
	  grub_putcode (code, term);
    }
  if (ret == '\n' && grub_newline_hook)
    grub_newline_hook ();
}

int
grub_getkey (void)
{
  return (grub_cur_term_input->getkey) ();
}

int
grub_checkkey (void)
{
  return (grub_cur_term_input->checkkey) ();
}

int
grub_getkeystatus (void)
{
  if (grub_cur_term_input->getkeystatus)
    return (grub_cur_term_input->getkeystatus) ();
  else
    return 0;
}

void
grub_cls (void)
{
  struct grub_term_output *term;
  
  for (term = grub_term_outputs; term; term = term->next)
    {
      if (! grub_term_is_active (term))
	continue;

      if ((term->flags & GRUB_TERM_DUMB) || (grub_env_get ("debug")))
	{
	  grub_putcode ('\n', term);
	  grub_refresh ();
	}
      else
	(term->cls) ();
    }
}

void
grub_setcolorstate (grub_term_color_state state)
{
  struct grub_term_output *term;
  
  for (term = grub_term_outputs; term; term = term->next)
    if (grub_term_is_active (term))
      grub_term_setcolorstate (term, state);
}

void
grub_refresh (void)
{
  struct grub_term_output *term;

  for (term = grub_term_outputs; term; term = term->next)
    if (grub_term_is_active (term))
      grub_term_refresh (term);
}
