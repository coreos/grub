/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/normal.h>
#include <pupa/term.h>
#include <pupa/misc.h>

/* FIXME: These below are all runaround.  */

#define DISP_LEFT	0x2190
#define DISP_UP		0x2191
#define DISP_RIGHT	0x2192
#define DISP_DOWN	0x2193
#define DISP_HLINE	0x2501
#define DISP_VLINE	0x2503
#define DISP_UL		0x250F
#define DISP_UR		0x2513
#define DISP_LL		0x2517
#define DISP_LR		0x251B

static void
draw_border (void)
{
  unsigned i;
  
  pupa_setcolorstate (PUPA_TERM_COLOR_NORMAL);
  
  pupa_gotoxy (1, 3);
  pupa_putcode (DISP_UL);
  for (i = 0; i < 73; i++)
    pupa_putcode (DISP_HLINE);
  pupa_putcode (DISP_UR);

  i = 1;
  while (1)
    {
      pupa_gotoxy (1, 3 + i);

      if (i > 12)
	break;

      pupa_putcode (DISP_VLINE);
      pupa_gotoxy (75, 3 + i);
      pupa_putcode (DISP_VLINE);

      i++;
    }

  pupa_putcode (DISP_LL);
  for (i = 0; i < 73; i++)
    pupa_putcode (DISP_HLINE);
  pupa_putcode (DISP_LR);

  pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);
}

static void
print_message (int nested)
{
  pupa_printf ("\n\
      Use the %C and %C keys to select which entry is highlighted.\n",
	       (pupa_uint32_t) DISP_UP, (pupa_uint32_t) DISP_DOWN);
  pupa_printf ("\
      Press enter to boot the selected OS, \'e\' to edit the\n\
      commands before booting, or \'c\' for a command-line.");
  if (nested)
    pupa_printf ("\
      ESC to return previous menu.");
}

static pupa_menu_entry_t
get_entry (pupa_menu_t menu, int no)
{
  pupa_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

static void
print_entry (int y, int highlight, pupa_menu_entry_t entry)
{
  int x;
  const char *title;

  title = entry ? entry->title : "";
  
  pupa_setcolorstate (highlight
		      ? PUPA_TERM_COLOR_HIGHLIGHT
		      : PUPA_TERM_COLOR_NORMAL);

  pupa_gotoxy (2, y);
  pupa_putchar (' ');
  for (x = 3; x < 75; x++)
    {
      if (*title && x <= 72)
	{
	  if (x == 72)
	    pupa_putcode (DISP_RIGHT);
	  else
	    pupa_putchar (*title++);
	}
      else
	pupa_putchar (' ');
    }
  pupa_gotoxy (74, y);

  pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);
}

static void
print_entries (pupa_menu_t menu, int first, int offset)
{
  pupa_menu_entry_t e;
  int i;
  
  pupa_gotoxy (77, 4);

  if (first)
    pupa_putcode (DISP_UP);
  else
    pupa_putchar (' ');

  e = get_entry (menu, first);

  for (i = 0; i < 12; i++)
    {
      print_entry (4 + i, offset == i, e);
      if (e)
	e = e->next;
    }

  pupa_gotoxy (77, 4 + 12);

  if (e)
    pupa_putcode (DISP_DOWN);
  else
    pupa_putchar (' ');

  pupa_gotoxy (74, 4 + offset);
}

static void
init_page (int nested)
{
  pupa_normal_init_page ();
  draw_border ();
  print_message (nested);
}

static int
run_menu (pupa_menu_t menu, int nested)
{
  int first, offset;
  
  pupa_setcursor (0);
  
  first = 0;
  offset = menu->default_entry;
  if (offset > 11)
    {
      first = offset - 11;
      offset = 11;
    }

  init_page (nested);
  print_entries (menu, first, offset);
  pupa_refresh ();
  
  while (1)
    {
      int c;

      c = PUPA_TERM_ASCII_CHAR (pupa_getkey ());
      switch (c)
	{
	case 16:
	case '^':
	  if (offset > 0)
	    {
	      print_entry (4 + offset, 0, get_entry (menu, first + offset));
	      offset--;
	      print_entry (4 + offset, 1, get_entry (menu, first + offset));
	    }
	  else if (first > 0)
	    {
	      first--;
	      print_entries (menu, first, offset);
	    }
	  break;

	case 14:
	case 'v':
	  if (menu->size > first + offset + 1)
	    {
	      if (offset < 11)
		{
		  print_entry (4 + offset, 0,
			       get_entry (menu, first + offset));
		  offset++;
		  print_entry (4 + offset, 1,
			       get_entry (menu, first + offset));
		}
	      else
		{
		  first++;
		  print_entries (menu, first, offset);
		}
	    }
	  break;

	case '\n':
	case '\r':
	case 6:
	  pupa_setcursor (1);
	  return first + offset;

	case '\e':
	  if (nested)
	    {
	      pupa_setcursor (1);
	      return -1;
	    }
	  break;

	case 'c':
	  pupa_setcursor (1);
	  pupa_cmdline_run (1);
	  pupa_setcursor (0);
	  init_page (nested);
	  print_entries (menu, first, offset);
	  break;

	default:
	  break;
	}
      
      pupa_refresh ();
    }

  /* Never reach here.  */
  return -1;
}

void
pupa_menu_run (pupa_menu_t menu, int nested)
{
  while (1)
    {
      int boot_entry;
      
      boot_entry = run_menu (menu, nested);
      if (boot_entry < 0)
	break;

      /* FIXME: Boot the entry.  */
    }
}
