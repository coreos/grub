/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/machine/console.h>
#include <pupa/term.h>
#include <pupa/types.h>

pupa_uint8_t pupa_console_cur_color = 0x7;
static pupa_uint8_t pupa_console_standard_color = 0x7;
static pupa_uint8_t pupa_console_normal_color = 0x7;
static pupa_uint8_t pupa_console_highlight_color = 0x70;

static void
pupa_console_setcolorstate (pupa_term_color_state state)
{
  switch (state) {
    case PUPA_TERM_COLOR_STANDARD:
      pupa_console_cur_color = pupa_console_standard_color;
      break;
    case PUPA_TERM_COLOR_NORMAL:
      pupa_console_cur_color = pupa_console_normal_color;
      break;
    case PUPA_TERM_COLOR_HIGHLIGHT:
      pupa_console_cur_color = pupa_console_highlight_color;
      break;
    default:
      break;
  }
}

static void
pupa_console_setcolor (pupa_uint8_t normal_color, pupa_uint8_t highlight_color)
{
  pupa_console_normal_color = normal_color;
  pupa_console_highlight_color = highlight_color;
}

static struct pupa_term pupa_console_term =
  {
    .name = "console",
    .putchar = pupa_console_putchar,
    .checkkey = pupa_console_checkkey,
    .getkey = pupa_console_getkey,
    .getxy = pupa_console_getxy,
    .gotoxy = pupa_console_gotoxy,
    .cls = pupa_console_cls,
    .setcolorstate = pupa_console_setcolorstate,
    .setcolor = pupa_console_setcolor,
    .setcursor = pupa_console_setcursor,
    .flags = 0,
    .next = 0
  };

void
pupa_console_init (void)
{
  pupa_term_register (&pupa_console_term);
  pupa_term_set_current (&pupa_console_term);
}
