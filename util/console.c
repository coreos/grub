/*  console.c -- Ncurses console for PUPA.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Marco Gerards  <metgerards@student.han.nl>
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

#include <curses.h>
#include <pupa/machine/console.h>
#include <pupa/term.h>
#include <pupa/types.h>

static int pupa_console_attr = A_NORMAL;

static void
pupa_ncurses_putchar (pupa_uint32_t c)
{
  addch (c | pupa_console_attr);
}

static void
pupa_ncurses_setcolorstate (pupa_term_color_state state)
{
  switch (state) 
    {
    case PUPA_TERM_COLOR_STANDARD:
      pupa_console_attr = A_NORMAL;
      break;
    case PUPA_TERM_COLOR_NORMAL:
      pupa_console_attr = A_NORMAL;
      break;
    case PUPA_TERM_COLOR_HIGHLIGHT:
      pupa_console_attr = A_STANDOUT;
      break;
    default:
      break;
    }
}

/* XXX: This function is never called.  */
static void
pupa_ncurses_setcolor (pupa_uint8_t normal_color, pupa_uint8_t highlight_color)
{
  color_set (normal_color << 8 | highlight_color, 0);
}

static int
pupa_ncurses_checkkey (void)
{
  return 1;
}

static int
pupa_ncurses_getkey (void)
{
  int c = getch ();

  switch (c)
    {
    case KEY_LEFT:
      c = PUPA_CONSOLE_KEY_LEFT;
      break;

    case KEY_RIGHT:
      c = PUPA_CONSOLE_KEY_RIGHT;
      break;
      
    case KEY_UP:
      c = PUPA_CONSOLE_KEY_UP;
      break;

    case KEY_DOWN:
      c = PUPA_CONSOLE_KEY_DOWN;
      break;

    case KEY_IC:
      c = PUPA_CONSOLE_KEY_IC;
      break;

    case KEY_DC:
      c = PUPA_CONSOLE_KEY_DC;
      break;

    case KEY_BACKSPACE:
      /* XXX: For some reason ncurses on xterm does not return
	 KEY_BACKSPACE.  */
    case 127: 
      c = PUPA_CONSOLE_KEY_BACKSPACE;
      break;

    case KEY_HOME:
      c = PUPA_CONSOLE_KEY_HOME;
      break;

    case KEY_END:
      c = PUPA_CONSOLE_KEY_END;
      break;

    case KEY_NPAGE:
      c = PUPA_CONSOLE_KEY_NPAGE;
      break;

    case KEY_PPAGE:
      c = PUPA_CONSOLE_KEY_PPAGE;
      break;
    }

  return c;
}

static pupa_uint16_t
pupa_ncurses_getxy (void)
{
  int x;
  int y;

  getyx (stdscr, y, x);

  return (x << 8) | y;
}

static void
pupa_ncurses_gotoxy (pupa_uint8_t x, pupa_uint8_t y)
{
  move (y, x);
}

static void
pupa_ncurses_cls (void)
{
  clear ();
  refresh ();
}

static void
pupa_ncurses_setcursor (int on)
{
  curs_set (on ? 1 : 0);
}

static void
pupa_ncurses_refresh (void)
{
  refresh ();
}

static pupa_err_t
pupa_ncurses_init (void)
{
  initscr ();
  cbreak (); 
  noecho ();
  scrollok (stdscr, TRUE);

  nonl ();
  intrflush (stdscr, FALSE);
  keypad (stdscr, TRUE);
  start_color ();

  return 0;
}

static pupa_err_t
pupa_ncurses_fini (void)
{
  endwin ();
}


static struct pupa_term pupa_ncurses_term =
  {
    .name = "console",
    .init = pupa_ncurses_init,
    .fini = pupa_ncurses_fini,
    .putchar = pupa_ncurses_putchar,
    .checkkey = pupa_ncurses_checkkey,
    .getkey = pupa_ncurses_getkey,
    .getxy = pupa_ncurses_getxy,
    .gotoxy = pupa_ncurses_gotoxy,
    .cls = pupa_ncurses_cls,
    .setcolorstate = pupa_ncurses_setcolorstate,
    .setcolor = pupa_ncurses_setcolor,
    .setcursor = pupa_ncurses_setcursor,
    .refresh = pupa_ncurses_refresh,
    .flags = 0,
    .next = 0
  };

void
pupa_console_init (void)
{
  pupa_term_register (&pupa_ncurses_term);
  pupa_term_set_current (&pupa_ncurses_term);
}
