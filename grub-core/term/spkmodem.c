/*  console.c -- Open Firmware console for GRUB.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/terminfo.h>
#include <grub/dl.h>
#include <grub/speaker.h>

GRUB_MOD_LICENSE ("GPLv3+");

extern struct grub_terminfo_output_state grub_spkmodem_terminfo_output;

static void
put (struct grub_term_output *term __attribute__ ((unused)), const int c)
{
  int i;

  for (i = 7; i >= 0; i--)
    {
      if ((c >> i) & 1)
	grub_speaker_beep_on (2000);
      else
	grub_speaker_beep_on (4000);
      grub_millisleep (10);
      grub_speaker_beep_on (1000);
      grub_millisleep (10);
    }
  grub_speaker_beep_on (50);
}

static grub_err_t
grub_spkmodem_init_output (struct grub_term_output *term)
{
  grub_speaker_beep_on (50);
  grub_millisleep (50);

  grub_terminfo_output_init (term);

  return 0;
}

static grub_err_t
grub_spkmodem_fini_output (struct grub_term_output *term __attribute__ ((unused)))
{
  grub_speaker_beep_off ();
  return 0;
}




struct grub_terminfo_output_state grub_spkmodem_terminfo_output =
  {
    .put = put,
    .width = 80,
    .height = 24
  };

static struct grub_term_output grub_spkmodem_term_output =
  {
    .name = "spkmodem",
    .init = grub_spkmodem_init_output,
    .fini = grub_spkmodem_fini_output,
    .putchar = grub_terminfo_putchar,
    .getxy = grub_terminfo_getxy,
    .getwh = grub_terminfo_getwh,
    .gotoxy = grub_terminfo_gotoxy,
    .cls = grub_terminfo_cls,
    .setcolorstate = grub_terminfo_setcolorstate,
    .setcursor = grub_terminfo_setcursor,
    .flags = GRUB_TERM_CODE_TYPE_ASCII,
    .data = &grub_spkmodem_terminfo_output,
    .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
    .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
  };

GRUB_MOD_INIT (spkmodem)
{
  grub_term_register_output ("spkmodem", &grub_spkmodem_term_output);
}


GRUB_MOD_FINI (spkmodem)
{
  grub_term_unregister_output (&grub_spkmodem_term_output);
}
