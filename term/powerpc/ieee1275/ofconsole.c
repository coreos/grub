/*  ofconsole.c -- Open Firmware console for PUPA.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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
#include <pupa/machine/ieee1275.h>
#include <pupa/term.h>
#include <pupa/types.h>
#include <pupa/misc.h>

static pupa_ieee1275_ihandle_t stdout_ihandle;
static pupa_ieee1275_ihandle_t stdin_ihandle;

static int pupa_curr_x;
static int pupa_curr_y;

static int pupa_keybuf;
static int pupa_buflen;

struct color
{
  int red;
  int green;
  int blue;
};

#define	MAX 0xff
static struct color colors[8] =
  {
    { 0,   0,   0},
    { MAX, 0,   0},
    { 0,   MAX, 0},
    { MAX, MAX, 0},
    { 0,   0,   MAX},
    { MAX, 0,   MAX},
    { 0,   MAX, MAX},
    { MAX, MAX, MAX}
  };

static int fgcolor = 7;
static int bgcolor = 0;

/* Write control characters to the console.  */
static void
pupa_ofconsole_writeesc (const char *str)
{
  while (*str)
    {
      char chr = *(str++);
      pupa_ieee1275_write (stdout_ihandle, &chr, 1, 0);
    }
  
}

static void
pupa_ofconsole_putchar (pupa_uint32_t c)
{
  char chr = c;
  if (c == '\n')
    {
      pupa_curr_y++;
      pupa_curr_x = 0;
    }
  else
    pupa_curr_x++;
  pupa_ieee1275_write (stdout_ihandle, &chr, 1, 0);
}

static void
pupa_ofconsole_setcolorstate (pupa_term_color_state state)
{
  char setcol[20];
  int fg;
  int bg;

  switch (state)
    {
    case PUPA_TERM_COLOR_STANDARD:
    case PUPA_TERM_COLOR_NORMAL:
      fg = fgcolor;
      bg = bgcolor;
      break;
    case PUPA_TERM_COLOR_HIGHLIGHT:
      fg = bgcolor;
      bg = fgcolor;
      break;
    default:
      return;
    }

  pupa_sprintf (setcol, "\e[3%dm\e[4%dm", fg, bg);
  pupa_ofconsole_writeesc (setcol);
}

static void
pupa_ofconsole_setcolor (pupa_uint8_t normal_color,
			 pupa_uint8_t highlight_color)
{
  fgcolor = normal_color;
  bgcolor = highlight_color;
}

static int
pupa_ofconsole_readkey (int *key)
{
  char c;
  int actual = 0;

  pupa_ieee1275_read (stdin_ihandle, &c, 1, &actual);

  if (actual && c == '\e')
    {
      pupa_ieee1275_read (stdin_ihandle, &c, 1, &actual);
      if (! actual)
	{
	  *key = '\e';
	  return 1;
	}
      
      if (c != 91)
	return 0;
      
      pupa_ieee1275_read (stdin_ihandle, &c, 1, &actual);
      if (! actual)
	return 0;
      
      switch (c)
	{
	case 65:
	  /* Up: Ctrl-p.  */
	  c = 16; 
	  break;
	case 66:
	  /* Down: Ctrl-n.  */
	  c = 14;
	  break;
	case 67:
	  /* Right: Ctrl-f.  */
	  c = 6;
	  break;
	case 68:
	  /* Left: Ctrl-b.  */
	  c = 2;
	  break;
	}
    }
  
  *key = c;
  return actual;
}

static int
pupa_ofconsole_checkkey (void)
{
  int key;
  int read;
  
  if (pupa_buflen)
    return 1;

  read = pupa_ofconsole_readkey (&key);
  if (read)
    {
      pupa_keybuf = key;
      pupa_buflen = 1;
      return 1;
    }
    
  return 0;
}

static int
pupa_ofconsole_getkey (void)
{
  int key;

  if (pupa_buflen)
    {
      pupa_buflen  =0;
      return pupa_keybuf;
    }

  while (! pupa_ofconsole_readkey (&key));
  
  return key;
}

static pupa_uint16_t
pupa_ofconsole_getxy (void)
{
  return ((pupa_curr_x - 1) << 8) | pupa_curr_y;
}

static void
pupa_ofconsole_gotoxy (pupa_uint8_t x, pupa_uint8_t y)
{
  char s[11]; /* 5 + 3 + 3.  */
  pupa_curr_x = x;
  pupa_curr_y = y;

  pupa_sprintf (s, "\e[%d;%dH", y - 1, x + 1);
  pupa_ofconsole_writeesc (s);
}

static void
pupa_ofconsole_cls (void)
{
  /* Clear the screen.  */
  pupa_ofconsole_writeesc ("");
}

static void
pupa_ofconsole_setcursor (int on __attribute ((unused)))
{
  /* XXX: Not supported.  */
}

static void
pupa_ofconsole_refresh (void)
{
  /* Do nothing, the current console state is ok.  */
}

static pupa_err_t
pupa_ofconsole_init (void)
{
  pupa_ieee1275_phandle_t chosen;
  char data[4];
  pupa_size_t actual;
  int col;

  if (pupa_ieee1275_finddevice ("/chosen", &chosen))
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Cannot find /chosen");

  if (pupa_ieee1275_get_property (chosen, "stdout", data, sizeof data,
			     &actual)
      || actual != sizeof data)
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Cannot find stdout");

  stdout_ihandle = pupa_ieee1275_decode_int_4 (data);
  
   if (pupa_ieee1275_get_property (chosen, "stdin", data, sizeof data,
			     &actual)
      || actual != sizeof data)
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Cannot find stdin");

  stdin_ihandle = pupa_ieee1275_decode_int_4 (data);

  /* Initialize colors.  */
  for (col = 0; col < 7; col++)
    pupa_ieee1275_set_color (stdout_ihandle, col, colors[col].red,
			     colors[col].green, colors[col].blue);

  /* Set the right fg and bg colors.  */
  pupa_ofconsole_setcolorstate (PUPA_TERM_COLOR_NORMAL);

  return 0;
}

static pupa_err_t
pupa_ofconsole_fini (void)
{
  return 0;
}



static struct pupa_term pupa_ofconsole_term =
  {
    .name = "ofconsole",
    .init = pupa_ofconsole_init,
    .fini = pupa_ofconsole_fini,
    .putchar = pupa_ofconsole_putchar,
    .checkkey = pupa_ofconsole_checkkey,
    .getkey = pupa_ofconsole_getkey,
    .getxy = pupa_ofconsole_getxy,
    .gotoxy = pupa_ofconsole_gotoxy,
    .cls = pupa_ofconsole_cls,
    .setcolorstate = pupa_ofconsole_setcolorstate,
    .setcolor = pupa_ofconsole_setcolor,
    .setcursor = pupa_ofconsole_setcursor,
    .refresh = pupa_ofconsole_refresh,
    .flags = 0,
    .next = 0
  };

void
pupa_console_init (void)
{
  pupa_term_register (&pupa_ofconsole_term);
  pupa_term_set_current (&pupa_ofconsole_term);
}
