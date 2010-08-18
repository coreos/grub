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
#include <grub/time.h>
#include <grub/dl.h>
#include <grub/at_keyboard.h>

GRUB_AT_KEY_KEYBOARD_MAP (keyboard_map);

static int
get_abstract_code (grub_term_input_t term, int in)
{
  switch (term->flags & GRUB_TERM_INPUT_FLAGS_TYPE_MASK)
    {
    case GRUB_TERM_INPUT_FLAGS_TYPE_TERMCODES:
    default:
      return in;
    case GRUB_TERM_INPUT_FLAGS_TYPE_BIOS:
      {
	unsigned status = 0;
	unsigned flags = 0;

	if (term->getkeystatus)
	  status = term->getkeystatus (term);
	if (status & GRUB_TERM_CAPS)
	  flags |= GRUB_TERM_CAPS;

	if ((0x5600 | '\\') == (in & 0xffff))
	  return GRUB_TERM_KEY_102 | flags;

	if ((0x5600 | '|') == (in & 0xffff))
	  return GRUB_TERM_KEY_SHIFT_102 | flags;

	/* Detect CTRL'ed keys.  */
	if ((in & 0xff) > 0 && (in & 0xff) < 0x20
	    && ((in & 0xffff) != (0x0100 | '\e'))
	    && ((in & 0xffff) != (0x0f00 | '\t'))
	    && ((in & 0xffff) != (0x0e00 | '\b'))
	    && ((in & 0xffff) != (0x1c00 | '\r'))
	    && ((in & 0xffff) != (0x1c00 | '\n')))
	  return ((in & 0xff) - 1 + 'a') | flags | GRUB_TERM_CTRL;
	/* Detect ALT'ed keys.  */
	/* XXX no way to distinguish left and right ALT.  */
	if (((in & 0xff) == 0) && keyboard_map[(in & 0xff00) >> 8] >= 'a'
	    && keyboard_map[(in & 0xff00) >> 8] <= 'z')
	  return keyboard_map[(in & 0xff00) >> 8] | flags | GRUB_TERM_ALT_GR;

	if ((in & 0xff) == 0)
	  return keyboard_map[(in & 0xff00) >> 8] | flags;

	return (in & 0xff) | flags;
      }
    }
}

static int
map (grub_term_input_t term, int in)
{
  /* No match with AltGr. Interpret it as Alt rather than as L3 modifier then.  
   */
  if (in & GRUB_TERM_ALT_GR)
    return map (term, in & ~GRUB_TERM_ALT_GR) | GRUB_TERM_ALT_GR;

  if (in == GRUB_TERM_KEY_102)
    return '\\';
  if (in == GRUB_TERM_KEY_SHIFT_102)
    return '|';

  return in;
}

static int
translate (grub_term_input_t term, int in)
{
  int code, flags;
  code = get_abstract_code (term, in);
  if ((code & GRUB_TERM_CAPS) && (code & 0xff) >= 'a' && (code & 0xff) <= 'z')
    code = (code & 0xff) + 'A' - 'a';
  else if ((code & GRUB_TERM_CAPS) && (code & 0xff) >= 'A'
	   && (code & 0xff) <= 'Z')
    code = (code & 0xff) + 'a' - 'A';    

  flags = code & ~(GRUB_TERM_KEY_MASK | GRUB_TERM_ALT_GR);
  code &= (GRUB_TERM_KEY_MASK | GRUB_TERM_ALT_GR);
  code = map (term, code);
  /* Transform unconsumed AltGr into Alt.  */
  if (code & GRUB_TERM_ALT_GR)
    {
      flags |= GRUB_TERM_ALT;
      code &= ~GRUB_TERM_ALT_GR;
    }
  if ((flags & GRUB_TERM_CAPS) && code >= 'a' && code <= 'z')
    code += 'A' - 'a';
  else if ((flags & GRUB_TERM_CAPS) && code >= 'A'
	   && code <= 'Z')
    code += 'a' - 'A';
  return code | flags;
}

static int
grub_getkey_smart (void)
{
  grub_term_input_t term;

  grub_refresh ();

  while (1)
    {
      FOR_ACTIVE_TERM_INPUTS(term)
      {
	int key = term->checkkey (term);
	if (key != -1)
	  return translate (term, term->getkey (term));
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
    int key = term->checkkey (term);
    if (key != -1)
      return translate (term, key);
  }

  return -1;
}

static int (*grub_getkey_saved) (void);

GRUB_MOD_INIT(keylayouts)
{
  grub_getkey_saved = grub_getkey;
  grub_getkey = grub_getkey_smart;
}

GRUB_MOD_FINI(keylayouts)
{
  grub_getkey = grub_getkey_saved;
}
