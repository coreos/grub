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
#include <grub/cpu/time.h>

struct grub_term_output *grub_term_outputs_disabled;
struct grub_term_input *grub_term_inputs_disabled;
struct grub_term_output *grub_term_outputs;
struct grub_term_input *grub_term_inputs;

void (*grub_newline_hook) (void) = NULL;

/* Put a Unicode character.  */
void
grub_putcode (grub_uint32_t code, struct grub_term_output *term)
{
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
  grub_size_t ret;

  buf[size++] = c;
  ret = grub_utf8_to_ucs4 (&code, 1, buf, size, 0);

  if (ret != 0)
    {
      struct grub_term_output *term;
      size = 0;
      FOR_ACTIVE_TERM_OUTPUTS(term)
	grub_putcode (code, term);
      if (code == '\n' && grub_newline_hook)
	grub_newline_hook ();
    }
}

int
grub_getkey (void)
{
  grub_term_input_t term;

  while (1)
    {
      FOR_ACTIVE_TERM_INPUTS(term)
      {
	int key = term->checkkey ();
	if (key != -1)
	  return term->getkey ();
      }

      grub_cpu_idle ();
    }
}

int
grub_checkkey (void)
{
  grub_term_input_t term;

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    int key = term->checkkey ();
    if (key != -1)
      return key;
  }

  return -1;
}

int
grub_getkeystatus (void)
{
  int status = 0;
  grub_term_input_t term;

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    if (term->getkeystatus)
      status |= term->getkeystatus ();
  }

  return status;
}

void
grub_cls (void)
{
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)  
  {
    if ((term->flags & GRUB_TERM_DUMB) || (grub_env_get ("debug")))
      {
	grub_putcode ('\n', term);
	grub_term_refresh (term);
      }
    else
      (term->cls) ();
  }
}

void
grub_setcolorstate (grub_term_color_state state)
{
  struct grub_term_output *term;
  
  FOR_ACTIVE_TERM_OUTPUTS(term)
    grub_term_setcolorstate (term, state);
}

void
grub_refresh (void)
{
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
    grub_term_refresh (term);
}
