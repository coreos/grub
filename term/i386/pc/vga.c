/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2000,2001,2002,2003  Free Software Foundation, Inc.
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

#include <pupa/machine/vga.h>
#include <pupa/machine/console.h>
#include <pupa/term.h>
#include <pupa/types.h>
#include <pupa/dl.h>
#include <pupa/misc.h>
#include <pupa/normal.h>
#include <pupa/font.h>

#define DEBUG_VGA	0

#define VGA_WIDTH	640
#define VGA_HEIGHT	480
#define CHAR_WIDTH	8
#define CHAR_HEIGHT	16
#define TEXT_WIDTH	(VGA_WIDTH / CHAR_WIDTH)
#define TEXT_HEIGHT	(VGA_HEIGHT / CHAR_HEIGHT)
#define VGA_MEM		((unsigned char *) 0xA0000)

#define DEFAULT_FG_COLOR	0xa
#define DEFAULT_BG_COLOR	0x0

struct colored_char
{
  /* An Unicode codepoint.  */
  pupa_uint32_t code;

  /* Color indexes.  */
  unsigned char fg_color;
  unsigned char bg_color;

  /* The width of this character minus one.  */
  unsigned char width;

  /* The column index of this character.  */
  unsigned char index;
};

static pupa_dl_t my_mod;
static unsigned char text_mode;
static unsigned xpos, ypos;
static int cursor_state;
static unsigned char fg_color, bg_color;
static struct colored_char text_buf[TEXT_WIDTH * TEXT_HEIGHT];
static unsigned char *vga_font;
static unsigned char saved_map_mask;

/* Read a byte from a port.  */
static inline unsigned char
inb (unsigned short port)
{
  unsigned char value;

  asm volatile ("inb    %w1, %0" : "=a" (value) : "Nd" (port));
  asm volatile ("outb   %%al, $0x80" : : );
  
  return value;
}

/* Write a byte to a port.  */
static inline void
outb (unsigned short port, unsigned char value)
{
  asm volatile ("outb   %b0, %w1" : : "a" (value), "Nd" (port));
  asm volatile ("outb   %%al, $0x80" : : );
}

#define SEQUENCER_ADDR_PORT	0x3C4
#define SEQUENCER_DATA_PORT	0x3C5
#define MAP_MASK_REGISTER	0x02

/* Get Map Mask Register.  */
static unsigned char
get_map_mask (void)
{
  unsigned char old_addr;
  unsigned char old_data;
  
  old_addr = inb (SEQUENCER_ADDR_PORT);
  outb (SEQUENCER_ADDR_PORT, MAP_MASK_REGISTER);
  
  old_data = inb (SEQUENCER_DATA_PORT);
  
  outb (SEQUENCER_ADDR_PORT, old_addr);

  return old_data;
}

/* Set Map Mask Register.  */
static void
set_map_mask (unsigned char mask)
{
  unsigned char old_addr;
  
  old_addr = inb (SEQUENCER_ADDR_PORT);
  outb (SEQUENCER_ADDR_PORT, MAP_MASK_REGISTER);
  
  outb (SEQUENCER_DATA_PORT, mask);
  
  outb (SEQUENCER_ADDR_PORT, old_addr);
}

static pupa_err_t
pupa_vga_init (void)
{
  vga_font = pupa_vga_get_font ();
  text_mode = pupa_vga_set_mode (0x12);
  cursor_state = 1;
  fg_color = DEFAULT_FG_COLOR;
  bg_color = DEFAULT_BG_COLOR;
  saved_map_mask = get_map_mask ();
  set_map_mask (0x0f);
  
  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_vga_fini (void)
{
  set_map_mask (saved_map_mask);
  pupa_vga_set_mode (text_mode);
  return PUPA_ERR_NONE;
}

static int
get_vga_glyph (pupa_uint32_t code, unsigned char bitmap[32], unsigned *width)
{
  if (code > 0x7f)
    {
      /* Map some unicode characters to the VGA font, if possible.  */
      switch (code)
	{
	case 0x2190:	/* left arrow */
	  code = 0x1b;
	  break;
	case 0x2191:	/* up arrow */
	  code = 0x18;
	  break;
	case 0x2192:	/* right arrow */
	  code = 0x1a;
	  break;
	case 0x2193:	/* down arrow */
	  code = 0x19;
	  break;
	case 0x2501:	/* horizontal line */
	  code = 0xc4;
	  break;
	case 0x2503:	/* vertical line */
	  code = 0xb3;
	  break;
	case 0x250F:	/* upper-left corner */
	  code = 0xda;
	  break;
	case 0x2513:	/* upper-right corner */
	  code = 0xbf;
	  break;
	case 0x2517:	/* lower-left corner */
	  code = 0xc0;
	  break;
	case 0x251B:	/* lower-right corner */
	  code = 0xd9;
	  break;

	default:
	  return pupa_font_get_glyph (code, bitmap, width);
	}
    }

  if (bitmap)
    pupa_memcpy (bitmap, vga_font + code * CHAR_HEIGHT, CHAR_HEIGHT);
  
  *width = 1;
  return 1;
}

static void
invalidate_char (struct colored_char *p)
{
  p->code = 0xFFFF;
  
  if (p->width)
    {
      struct colored_char *q;

      for (q = p + 1; q <= p + p->width; q++)
	{
	  q->code = 0xFFFF;
	  q->width = 0;
	  q->index = 0;
	}
    }

  p->width = 0;
}

static int
check_vga_mem (void *p)
{
  return p >= VGA_MEM && p <= VGA_MEM + VGA_WIDTH * VGA_HEIGHT / 8;
}

static void
write_char (void)
{
  struct colored_char *p = text_buf + xpos + ypos * TEXT_WIDTH;
  unsigned char bitmap[32];
  unsigned width;
  unsigned char *mem_base = VGA_MEM + xpos + ypos * CHAR_HEIGHT * TEXT_WIDTH;
  unsigned plane;

  mem_base -= p->index;
  p -= p->index;

  if (! get_vga_glyph (p->code, bitmap, &width))
    invalidate_char (p);
  
  for (plane = 0x01; plane <= 0x08; plane <<= 1)
    {
      unsigned y;
      unsigned offset;
      unsigned char *mem;

      set_map_mask (plane);

      for (y = 0, offset = 0, mem = mem_base;
	   y < CHAR_HEIGHT;
	   y++, mem += TEXT_WIDTH)
	{
	  unsigned i;

	  for (i = 0; i < width && offset < 32; i++)
	    {
	      unsigned char fg_mask, bg_mask;
	      
	      fg_mask = (p->fg_color & plane) ? bitmap[offset] : 0;
	      bg_mask = (p->bg_color & plane) ? ~(bitmap[offset]) : 0;
	      offset++;

	      if (check_vga_mem (mem + i))
		mem[i] = (fg_mask | bg_mask);
	    }
	}
    }

  set_map_mask (0x0f);
}

static void
write_cursor (void)
{
  unsigned char *mem = (VGA_MEM + xpos
			+ (ypos * CHAR_HEIGHT + CHAR_HEIGHT - 3) * TEXT_WIDTH);
  if (check_vga_mem (mem))
    *mem = 0xff;
  
  mem += TEXT_WIDTH;
  if (check_vga_mem (mem))
    *mem = 0xff;
}

static void
scroll_up (void)
{
  unsigned i;
  unsigned plane;
  
  pupa_memmove (text_buf, text_buf + TEXT_WIDTH,
		sizeof (struct colored_char) * TEXT_WIDTH * (TEXT_HEIGHT - 1));
      
  for (i = TEXT_WIDTH * (TEXT_HEIGHT - 1); i < TEXT_WIDTH * TEXT_HEIGHT; i++)
    {
      text_buf[i].code = ' ';
      text_buf[i].fg_color = 0;
      text_buf[i].bg_color = 0;
      text_buf[i].width = 0;
      text_buf[i].index = 0;
    }

  for (plane = 0x1; plane <= 0x8; plane <<= 1)
    {
      set_map_mask (plane);
      pupa_memmove (VGA_MEM, VGA_MEM + VGA_WIDTH * CHAR_HEIGHT / 8,
		    VGA_WIDTH * (VGA_HEIGHT - CHAR_HEIGHT) / 8);
    }

  set_map_mask (0x0f);
  pupa_memset (VGA_MEM + VGA_WIDTH * (VGA_HEIGHT - CHAR_HEIGHT) / 8, 0,
	       VGA_WIDTH * CHAR_HEIGHT / 8);
}

static void
pupa_vga_putchar (pupa_uint32_t c)
{
  static int show = 1;
  
  if (c == '\a')
    /* FIXME */
    return;

  if (c == '\b' || c == '\n' || c == '\r')
    {
      /* Erase current cursor, if any.  */
      if (cursor_state)
	write_char ();
  
      switch (c)
	{
	case '\b':
	  if (xpos > 0)
	    xpos--;
	  break;
	  
	case '\n':
	  if (ypos >= TEXT_HEIGHT)
	    scroll_up ();
	  else
	    ypos++;
	  break;
	  
	case '\r':
	  xpos = 0;
	  break;
	}

      if (cursor_state)
	write_cursor ();
    }
  else
    {
      unsigned width;
      struct colored_char *p;
      
      get_vga_glyph (c, 0, &width);

      if (xpos + width > TEXT_WIDTH)
	pupa_putchar ('\n');

      p = text_buf + xpos + ypos * TEXT_WIDTH;
      p->code = c;
      p->fg_color = fg_color;
      p->bg_color = bg_color;
      p->width = width - 1;
      p->index = 0;

      if (width > 1)
	{
	  unsigned i;

	  for (i = 1; i < width; i++)
	    {
	      p[i].code = ' ';
	      p[i].width = width - 1;
	      p[i].index = i;
	    }
	}
	  
      write_char ();
  
      xpos += width;
      if (xpos >= TEXT_WIDTH)
	{
	  xpos = 0;
	  
	  if (ypos >= TEXT_HEIGHT)
	    scroll_up ();
	  else
	    ypos++;
	}

      if (cursor_state)
	write_cursor ();
    }

#if DEBUG_VGA
  if (show)
    {
      pupa_uint16_t pos = pupa_getxy ();

      show = 0;
      pupa_gotoxy (0, 0);
      pupa_printf ("[%u:%u]", (unsigned) (pos >> 8), (unsigned) (pos & 0xff));
      pupa_gotoxy (pos >> 8, pos & 0xff);
      show = 1;
    }
#endif
}

 static pupa_uint16_t
 pupa_vga_getxy (void)
 {
   return ((xpos << 8) | ypos);
 }

 static void
 pupa_vga_gotoxy (pupa_uint8_t x, pupa_uint8_t y)
 {
   if (x >= TEXT_WIDTH || y >= TEXT_HEIGHT)
     {
       pupa_error (PUPA_ERR_OUT_OF_RANGE, "invalid point (%u,%u)",
		   (unsigned) x, (unsigned) y);
       return;
     }

   if (cursor_state)
     write_char ();

   xpos = x;
   ypos = y;

   if (cursor_state)
     write_cursor ();
 }

 static void
 pupa_vga_cls (void)
 {
   unsigned i;

   for (i = 0; i < TEXT_WIDTH * TEXT_HEIGHT; i++)
     {
       text_buf[i].code = ' ';
       text_buf[i].fg_color = 0;
       text_buf[i].bg_color = 0;
       text_buf[i].width = 0;
       text_buf[i].index = 0;
     }

   pupa_memset (VGA_MEM, 0, VGA_WIDTH * VGA_HEIGHT / 8);

   xpos = ypos = 0;
 }

 static void
 pupa_vga_setcolorstate (pupa_term_color_state state)
 {
   switch (state)
     {
     case PUPA_TERM_COLOR_STANDARD:
     case PUPA_TERM_COLOR_NORMAL:
       fg_color = DEFAULT_FG_COLOR;
       bg_color = DEFAULT_BG_COLOR;
       break;
     case PUPA_TERM_COLOR_HIGHLIGHT:
       fg_color = DEFAULT_BG_COLOR;
       bg_color = DEFAULT_FG_COLOR;
       break;
     default:
       break;
     }
 }

 static void
 pupa_vga_setcolor (pupa_uint8_t normal_color, pupa_uint8_t highlight_color)
 {
   /* FIXME */
 }

 static void
 pupa_vga_setcursor (int on)
 {
   if (cursor_state != on)
     {
       if (cursor_state)
	 write_char ();
       else
	 write_cursor ();

       cursor_state = on;
     }
 }

 static struct pupa_term pupa_vga_term =
   {
     .name = "vga",
     .init = pupa_vga_init,
     .fini = pupa_vga_fini,
     .putchar = pupa_vga_putchar,
     .checkkey = pupa_console_checkkey,
     .getkey = pupa_console_getkey,
     .getxy = pupa_vga_getxy,
     .gotoxy = pupa_vga_gotoxy,
     .cls = pupa_vga_cls,
     .setcolorstate = pupa_vga_setcolorstate,
     .setcolor = pupa_vga_setcolor,
     .setcursor = pupa_vga_setcursor,
     .flags = 0,
     .next = 0
   };

static int
debug_command (int argc, char *argv[])
{
  pupa_printf ("???????бу??n");

  return 0;
}

PUPA_MOD_INIT
{
  my_mod = mod;
  pupa_term_register (&pupa_vga_term);
  pupa_register_command ("debug", debug_command, PUPA_COMMAND_FLAG_CMDLINE,
			 "debug", "Debug it!");
}

PUPA_MOD_FINI
{
  pupa_term_unregister (&pupa_vga_term);
}
