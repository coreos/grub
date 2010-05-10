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

static int keyboard_map[128] =
{
  '\0', GRUB_TERM_ESC, '1', '2', '3', '4', '5', '6',
  '7', '8', '9', '0', '-', '=', GRUB_TERM_BACKSPACE, GRUB_TERM_TAB,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
  'o', 'p', '[', ']', '\n', '\0', 'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
  '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
  'b', 'n', 'm', ',', '.', '/', '\0', '*',
  '\0', ' ', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', GRUB_TERM_KEY_HOME,
  GRUB_TERM_KEY_UP, GRUB_TERM_KEY_NPAGE, '-', GRUB_TERM_KEY_LEFT, '\0', GRUB_TERM_KEY_RIGHT, '+', GRUB_TERM_KEY_END,
  GRUB_TERM_KEY_DOWN, GRUB_TERM_KEY_PPAGE, '\0', GRUB_TERM_KEY_DC
};

/* Define scan codes.  */
#define GRUB_TERM_AT_KEY_LEFT		0x4B00
#define GRUB_TERM_AT_KEY_RIGHT		0x4D00
#define GRUB_TERM_AT_KEY_UP		0x4800
#define GRUB_TERM_AT_KEY_DOWN		0x5000
#define GRUB_TERM_AT_KEY_IC		0x5200
#define GRUB_TERM_AT_KEY_DC		0x5300
#define GRUB_TERM_AT_KEY_BACKSPACE	0x0008
#define GRUB_TERM_AT_KEY_HOME		0x4700
#define GRUB_TERM_AT_KEY_END		0x4F00
#define GRUB_TERM_AT_KEY_NPAGE		0x5100
#define GRUB_TERM_AT_KEY_PPAGE		0x4900

static int
get_abstract_code (grub_term_input_t term, int in)
{
  unsigned flags = 0;
  switch (term->flags & GRUB_TERM_INPUT_FLAGS_TYPE_MASK)
    {
    default:
      return in;
    case GRUB_TERM_INPUT_FLAGS_TYPE_BIOS:
      {
	unsigned status = 0;
	if (term->getkeystatus)
	  status = term->getkeystatus ();
	if (status & GRUB_TERM_CAPS)
	  flags |= GRUB_TERM_CAPS;
      }
      /* Fall through.  */
    case GRUB_TERM_INPUT_FLAGS_TYPE_AT:
      {
	struct {
	  int from, to; 
	} translations[] =
	    {  
	      {GRUB_TERM_AT_KEY_LEFT, GRUB_TERM_KEY_LEFT},
	      {GRUB_TERM_AT_KEY_RIGHT, GRUB_TERM_KEY_RIGHT},
	      {GRUB_TERM_AT_KEY_UP, GRUB_TERM_KEY_UP},
	      {GRUB_TERM_AT_KEY_DOWN, GRUB_TERM_KEY_DOWN},
	      {GRUB_TERM_AT_KEY_HOME, GRUB_TERM_KEY_HOME},
	      {GRUB_TERM_AT_KEY_END, GRUB_TERM_KEY_END},
	      {GRUB_TERM_AT_KEY_DC, GRUB_TERM_KEY_DC},
	      {GRUB_TERM_AT_KEY_PPAGE, GRUB_TERM_KEY_PPAGE},
	      {GRUB_TERM_AT_KEY_NPAGE, GRUB_TERM_KEY_NPAGE},
	      {0x5600 | '\\', GRUB_TERM_KEY_102},
	      {0x5600 | '|', GRUB_TERM_KEY_SHIFT_102},
	    };
	unsigned i;
	for (i = 0; i < ARRAY_SIZE (translations); i++)
	  if (translations[i].from == (in & 0xffff))
	    return translations[i].to | flags;
	if ((term->flags & GRUB_TERM_INPUT_FLAGS_TYPE_MASK)
	    == GRUB_TERM_INPUT_FLAGS_TYPE_AT)
	  return in & ~0xff00;
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

	return (in & 0xff) | flags;
      }
    }
}

static int
map (grub_term_input_t term __attribute__ ((unused)), int in)
{
  if (in == GRUB_TERM_KEY_102)
    return '\\';
  if (in == GRUB_TERM_KEY_SHIFT_102)
    return '|';

  return in;
}

static int
translate (grub_term_input_t term, int in)
{
  int code, code2;
  code = get_abstract_code (term, in);
  if ((code & GRUB_TERM_CAPS) && (code & 0xff) >= 'a' && (code & 0xff) <= 'z')
    code = (code & 0xff) + 'A' - 'a';
  else if ((code & GRUB_TERM_CAPS) && (code & 0xff) >= 'A'
	   && (code & 0xff) <= 'Z')
    code = (code & 0xff) + 'a' - 'A';    

  code2 = map (term, code & 0x1ffffff);
  if ((code & GRUB_TERM_CAPS) && (code2 & 0xff) >= 'a' && (code2 & 0xff) <= 'z')
    code2 = code2 + 'A' - 'a';
  else if ((code & GRUB_TERM_CAPS) && (code2 & 0xff) >= 'A'
	   && (code2 & 0xff) <= 'Z')
    code2 = code2 + 'a' - 'A';
  return code2 | (code & ~0x1ffffff);
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
	int key = term->checkkey ();
	if (key != -1)
	  return translate (term, term->getkey ());
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
