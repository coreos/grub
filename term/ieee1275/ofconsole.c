/*  ofconsole.c -- Open Firmware console for GRUB.  */
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
#include <grub/machine/console.h>
#include <grub/ieee1275/ieee1275.h>

static grub_ieee1275_ihandle_t stdout_ihandle;
static grub_ieee1275_ihandle_t stdin_ihandle;

static grub_uint8_t grub_ofconsole_width;
static grub_uint8_t grub_ofconsole_height;

static int grub_curr_x;
static int grub_curr_y;

static int grub_keybuf;
static int grub_buflen;

struct color
{
  int red;
  int green;
  int blue;
};

static struct color colors[] =
  {
    // {R, G, B}
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0xA8}, // 1 = blue
    {0x00, 0xA8, 0x00}, // 2 = green
    {0x00, 0xA8, 0xA8}, // 3 = cyan
    {0xA8, 0x00, 0x00}, // 4 = red
    {0xA8, 0x00, 0xA8}, // 5 = magenta
    {0xFE, 0xFE, 0x54}, // 6 = yellow
    {0xFE, 0xFE, 0xFE}  // 7 = white
  };

static grub_uint8_t grub_ofconsole_normal_color = 0x7;
static grub_uint8_t grub_ofconsole_highlight_color = 0x70;

/* Write control characters to the console.  */
static void
grub_ofconsole_writeesc (const char *str)
{
  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_ANSI))
    return;

  while (*str)
    {
      char chr = *(str++);
      grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
    }

}

static void
grub_ofconsole_putchar (grub_uint32_t c)
{
  char chr;

  if (c > 0x7F)
    {
      /* Better than nothing.  */
      switch (c)
	{
	case GRUB_TERM_DISP_LEFT:
	  c = '<';
	  break;
	  
	case GRUB_TERM_DISP_UP:
	  c = '^';
	  break;

	case GRUB_TERM_DISP_RIGHT:
	  c = '>';
	  break;

	case GRUB_TERM_DISP_DOWN:
	  c = 'v';
	  break;

	case GRUB_TERM_DISP_HLINE:
	  c = '-';
	  break;

	case GRUB_TERM_DISP_VLINE:
	  c = '|';
	  break;

	case GRUB_TERM_DISP_UL:
	case GRUB_TERM_DISP_UR:
	case GRUB_TERM_DISP_LL:
	case GRUB_TERM_DISP_LR:
	  c = '+';
	  break;

	default:
	  c = '?';
	  break;
	}
    }

  chr = c;

  if (c == '\n')
    {
      grub_curr_y++;
      grub_curr_x = 0;
    }
  else if (c == '\r')
    {
      grub_curr_x = 0;
    }
  else
    {
      grub_curr_x++;
      if (grub_curr_x >= grub_ofconsole_width)
        {
          grub_ofconsole_putchar ('\n');
          grub_ofconsole_putchar ('\r');
          grub_curr_x++;
        }
    }
  grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
}

static grub_ssize_t
grub_ofconsole_getcharwidth (grub_uint32_t c __attribute__((unused)))
{
  return 1;
}

static void
grub_ofconsole_setcolorstate (grub_term_color_state state)
{
  char setcol[256];
  int fg;
  int bg;

  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      fg = grub_ofconsole_normal_color & 0x0f;
      bg = grub_ofconsole_normal_color >> 4;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      fg = grub_ofconsole_highlight_color & 0x0f;
      bg = grub_ofconsole_highlight_color >> 4;
      break;
    default:
      return;
    }

  grub_snprintf (setcol, sizeof (setcol), "\e[3%dm\e[4%dm", fg, bg);
  grub_ofconsole_writeesc (setcol);
}

static void
grub_ofconsole_setcolor (grub_uint8_t normal_color,
			 grub_uint8_t highlight_color)
{
  /* Discard bright bit.  */
  grub_ofconsole_normal_color = normal_color & 0x77;
  grub_ofconsole_highlight_color = highlight_color & 0x77;
}

static void
grub_ofconsole_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color)
{
  *normal_color = grub_ofconsole_normal_color;
  *highlight_color = grub_ofconsole_highlight_color;
}

static int
grub_ofconsole_readkey (int *key)
{
  char c;
  grub_ssize_t actual = 0;

  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);

  if (actual > 0)
    switch(c)
      {
      case 0x7f:
        /* Backspace: Ctrl-h.  */
        c = '\b'; 
        break;
      case '\e':
	{
	  grub_uint64_t start;
	  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);

	  /* On 9600 we have to wait up to 12 milliseconds.  */
	  start = grub_get_time_ms ();
	  while (actual <= 0 && grub_get_time_ms () - start < 12)
	    grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);

	  if (actual <= 0)
	    {
	      *key = '\e';
	      return 1;
	    }

	  if (c != '[')
	    return 0;
	  
	  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);

	  /* On 9600 we have to wait up to 12 milliseconds.  */
	  start = grub_get_time_ms ();
	  while (actual <= 0 && grub_get_time_ms () - start < 12)
	    grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
	  if (actual <= 0)
	    return 0;

	  switch (c)
	    {
	    case 'A':
	      /* Up: Ctrl-p.  */
	      c = GRUB_TERM_UP;
	      break;
	    case 'B':
	      /* Down: Ctrl-n.  */
	      c = GRUB_TERM_DOWN;
	      break;
	    case 'C':
	      /* Right: Ctrl-f.  */
	      c = GRUB_TERM_RIGHT;
	      break;
	    case 'D':
	      /* Left: Ctrl-b.  */
	      c = GRUB_TERM_LEFT;
	      break;
	    case '3':
	      {
		grub_uint64_t start;		
		grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
		/* On 9600 we have to wait up to 12 milliseconds.  */
		start = grub_get_time_ms ();
		while (actual <= 0 && grub_get_time_ms () - start < 12)
		  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
		
		if (actual <= 0)
		  return 0;
  	    
		/* Delete: Ctrl-d.  */
		if (c == '~')
		  c = GRUB_TERM_DC;
		else
		  return 0;
		break;
	      }
	      break;
	    }
	}
      }

  *key = c;
  return actual > 0;
}

static int
grub_ofconsole_checkkey (void)
{
  int key;
  int read;

  if (grub_buflen)
    return 1;

  read = grub_ofconsole_readkey (&key);
  if (read)
    {
      grub_keybuf = key;
      grub_buflen = 1;
      return 1;
    }

  return -1;
}

static int
grub_ofconsole_getkey (void)
{
  int key;

  if (grub_buflen)
    {
      grub_buflen  =0;
      return grub_keybuf;
    }

  while (! grub_ofconsole_readkey (&key));

  return key;
}

static grub_uint16_t
grub_ofconsole_getxy (void)
{
  return ((grub_curr_x - 1) << 8) | grub_curr_y;
}

static void
grub_ofconsole_dimensions (void)
{
  grub_ieee1275_ihandle_t options;
  grub_ssize_t lval;

  if (! grub_ieee1275_finddevice ("/options", &options)
      && options != (grub_ieee1275_ihandle_t) -1)
    {
      if (! grub_ieee1275_get_property_length (options, "screen-#columns",
					       &lval)
	  && lval >= 0 && lval < 1024)
	{
	  char val[lval];

	  if (! grub_ieee1275_get_property (options, "screen-#columns",
					    val, lval, 0))
	    grub_ofconsole_width = (grub_uint8_t) grub_strtoul (val, 0, 10);
	}
      if (! grub_ieee1275_get_property_length (options, "screen-#rows", &lval)
	  && lval >= 0 && lval < 1024)
	{
	  char val[lval];
	  if (! grub_ieee1275_get_property (options, "screen-#rows",
					    val, lval, 0))
	    grub_ofconsole_height = (grub_uint8_t) grub_strtoul (val, 0, 10);
	}
    }

  /* Use a small console by default.  */
  if (! grub_ofconsole_width)
    grub_ofconsole_width = 80;
  if (! grub_ofconsole_height)
    grub_ofconsole_height = 24;
}

static grub_uint16_t
grub_ofconsole_getwh (void)
{
  return (grub_ofconsole_width << 8) | grub_ofconsole_height;
}

static void
grub_ofconsole_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_ANSI))
    {
      char s[256];
      grub_curr_x = x;
      grub_curr_y = y;

      grub_snprintf (s, sizeof (s), "\e[%d;%dH", y + 1, x + 1);
      grub_ofconsole_writeesc (s);
    }
  else
    {
      if ((y == grub_curr_y) && (x == grub_curr_x - 1))
        {
          char chr;

          chr = '\b';
          grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
        }

      grub_curr_x = x;
      grub_curr_y = y;
    }
}

static void
grub_ofconsole_cls (void)
{
  /* Clear the screen.  Using serial console, screen(1) only recognizes the
   * ANSI escape sequence.  Using video console, Apple Open Firmware (version
   * 3.1.1) only recognizes the literal ^L.  So use both.  */
  grub_ofconsole_writeesc ("\e[2J");
  grub_ofconsole_gotoxy (0, 0);
}

static void
grub_ofconsole_setcursor (int on)
{
  /* Understood by the Open Firmware flavour in OLPC.  */
  if (on)
    grub_ieee1275_interpret ("cursor-on", 0);
  else
    grub_ieee1275_interpret ("cursor-off", 0);
}

static void
grub_ofconsole_refresh (void)
{
  /* Do nothing, the current console state is ok.  */
}

static grub_err_t
grub_ofconsole_init_input (void)
{
  grub_ssize_t actual;

  if (grub_ieee1275_get_integer_property (grub_ieee1275_chosen, "stdin", &stdin_ihandle,
					  sizeof stdin_ihandle, &actual)
      || actual != sizeof stdin_ihandle)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot find stdin");

  return 0;
}

static grub_err_t
grub_ofconsole_init_output (void)
{
  grub_ssize_t actual;

  /* The latest PowerMacs don't actually initialize the screen for us, so we
   * use this trick to re-open the output device (but we avoid doing this on
   * platforms where it's known to be broken). */
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_BROKEN_OUTPUT))
    grub_ieee1275_interpret ("output-device output", 0);

  if (grub_ieee1275_get_integer_property (grub_ieee1275_chosen, "stdout", &stdout_ihandle,
					  sizeof stdout_ihandle, &actual)
      || actual != sizeof stdout_ihandle)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot find stdout");

  /* Initialize colors.  */
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_SET_COLORS))
    {
      unsigned col;
      for (col = 0; col < ARRAY_SIZE (colors); col++)
	grub_ieee1275_set_color (stdout_ihandle, col, colors[col].red,
				 colors[col].green, colors[col].blue);

    /* Set the right fg and bg colors.  */
      grub_ofconsole_setcolorstate (GRUB_TERM_COLOR_NORMAL);
    }

  grub_ofconsole_dimensions ();

  return 0;
}

static grub_err_t
grub_ofconsole_fini (void)
{
  return 0;
}



static struct grub_term_input grub_ofconsole_term_input =
  {
    .name = "ofconsole",
    .init = grub_ofconsole_init_input,
    .fini = grub_ofconsole_fini,
    .checkkey = grub_ofconsole_checkkey,
    .getkey = grub_ofconsole_getkey,
  };

static struct grub_term_output grub_ofconsole_term_output =
  {
    .name = "ofconsole",
    .init = grub_ofconsole_init_output,
    .fini = grub_ofconsole_fini,
    .putchar = grub_ofconsole_putchar,
    .getcharwidth = grub_ofconsole_getcharwidth,
    .getxy = grub_ofconsole_getxy,
    .getwh = grub_ofconsole_getwh,
    .gotoxy = grub_ofconsole_gotoxy,
    .cls = grub_ofconsole_cls,
    .setcolorstate = grub_ofconsole_setcolorstate,
    .setcolor = grub_ofconsole_setcolor,
    .getcolor = grub_ofconsole_getcolor,
    .setcursor = grub_ofconsole_setcursor,
    .refresh = grub_ofconsole_refresh
  };

void
grub_console_init (void)
{
  grub_term_register_input ("ofconsole", &grub_ofconsole_term_input);
  grub_term_register_output ("ofconsole", &grub_ofconsole_term_output);
}

void
grub_console_fini (void)
{
  grub_term_unregister_input (&grub_ofconsole_term_input);
  grub_term_unregister_output (&grub_ofconsole_term_output);
}
