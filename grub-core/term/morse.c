/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011,2012,2013  Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/time.h>
#include <grub/speaker.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define BASE_TIME 250

static const char codes[0x80][6] =
  {
    ['0'] = { 3, 3, 3, 3, 3, 0 },
    ['1'] = { 1, 3, 3, 3, 3, 0 },
    ['2'] = { 1, 1, 3, 3, 3, 0 },
    ['3'] = { 1, 1, 1, 3, 3, 0 },
    ['4'] = { 1, 1, 1, 1, 3, 0 },
    ['5'] = { 1, 1, 1, 1, 1, 0 },
    ['6'] = { 3, 1, 1, 1, 1, 0 },
    ['7'] = { 3, 3, 1, 1, 1, 0 },
    ['8'] = { 3, 3, 3, 1, 1, 0 },
    ['9'] = { 3, 3, 3, 3, 1, 0 },
    ['a'] = { 1, 3, 0 },
    ['b'] = { 3, 1, 1, 1, 0 },
    ['c'] = { 3, 1, 3, 1, 0 },
    ['d'] = { 3, 1, 1, 0 },
    ['e'] = { 1, 0 },
    ['f'] = { 1, 1, 3, 1, 0 },
    ['g'] = { 3, 3, 1, 0 },
    ['h'] = { 1, 1, 1, 1, 0 },
    ['i'] = { 1, 1, 0 },
    ['j'] = { 1, 3, 3, 3, 0 },
    ['k'] = { 3, 1, 3, 0 },
    ['l'] = { 1, 3, 1, 1, 0 },
    ['m'] = { 3, 3, 0 },
    ['n'] = { 3, 1, 0 },
    ['o'] = { 3, 3, 3, 0 },
    ['p'] = { 1, 3, 3, 1, 0 },
    ['q'] = { 3, 3, 1, 3, 0 },
    ['r'] = { 1, 3, 1, 0 },
    ['s'] = { 1, 1, 1, 0 },
    ['t'] = { 3, 0 },
    ['u'] = { 1, 1, 3, 0 },
    ['v'] = { 1, 1, 1, 3, 0 },
    ['w'] = { 1, 3, 3, 0 },
    ['x'] = { 3, 1, 1, 3, 0 },
    ['y'] = { 3, 1, 3, 3, 0 },
    ['z'] = { 3, 3, 1, 1, 0 }
  };

static void
grub_audio_tone (int length)
{
  grub_speaker_beep_on (1000);
  grub_millisleep (length);
  grub_speaker_beep_off ();
}

static void
grub_audio_putchar (struct grub_term_output *term __attribute__ ((unused)),
		    const struct grub_unicode_glyph *c_in)
{
  grub_uint8_t c;
  int i;

  /* For now, do not try to use a surrogate pair.  */
  if (c_in->base > 0x7f)
    c = '?';
  else
    c = grub_tolower (c_in->base);
  for (i = 0; codes[c][i]; i++)
    {
      grub_audio_tone (codes[c][i] * BASE_TIME);
      grub_millisleep (BASE_TIME);
    }
  grub_millisleep (2 * BASE_TIME);
}


static int
dummy (void)
{
  return 0;
}

static struct grub_term_output grub_audio_term_output =
  {
   .name = "morse",
   .init = (void *) dummy,
   .fini = (void *) dummy,
   .putchar = grub_audio_putchar,
   .getwh = (void *) dummy,
   .getxy = (void *) dummy,
   .gotoxy = (void *) dummy,
   .cls = (void *) dummy,
   .setcolorstate = (void *) dummy,
   .setcursor = (void *) dummy,
   .flags = GRUB_TERM_CODE_TYPE_ASCII | GRUB_TERM_DUMB
  };

GRUB_MOD_INIT (morse)
{
  grub_term_register_output ("audio", &grub_audio_term_output);
}

GRUB_MOD_FINI (morse)
{
  grub_term_unregister_output (&grub_audio_term_output);
}
