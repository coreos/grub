/* term_console.c - console input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#include <shared.h>
#include <term.h>

/* These functions are defined in asm.S instead of this file:
   console_putchar, console_checkkey, console_getkey, console_getxy,
   console_gotoxy, console_cls, and console_nocursor.  */

int console_current_color = A_NORMAL;
static int console_normal_color = A_NORMAL;
static int console_highlight_color = A_REVERSE;
static int console_highlight_state = 0;

void
console_highlight (int state)
{
  console_current_color
    = state ? console_highlight_color : console_normal_color;
  console_highlight_state = state;
}

void
console_setcolor (int normal_color, int highlight_color)
{
  console_normal_color = normal_color;
  console_highlight_color = highlight_color;

  console_highlight (console_highlight_state);
}
