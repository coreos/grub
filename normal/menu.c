/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/machine/time.h>

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
  
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);
  
  grub_gotoxy (1, 3);
  grub_putcode (DISP_UL);
  for (i = 0; i < 73; i++)
    grub_putcode (DISP_HLINE);
  grub_putcode (DISP_UR);

  i = 1;
  while (1)
    {
      grub_gotoxy (1, 3 + i);

      if (i > 12)
	break;

      grub_putcode (DISP_VLINE);
      grub_gotoxy (75, 3 + i);
      grub_putcode (DISP_VLINE);

      i++;
    }

  grub_putcode (DISP_LL);
  for (i = 0; i < 73; i++)
    grub_putcode (DISP_HLINE);
  grub_putcode (DISP_LR);

  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
}

static void
print_message (int nested)
{
  grub_printf ("\n\
      Use the %C and %C keys to select which entry is highlighted.\n",
	       (grub_uint32_t) DISP_UP, (grub_uint32_t) DISP_DOWN);
  grub_printf ("\
      Press enter to boot the selected OS, \'e\' to edit the\n\
      commands before booting, or \'c\' for a command-line.");
  if (nested)
    grub_printf ("\n\
      ESC to return previous menu.");
}

static grub_menu_entry_t
get_entry (grub_menu_t menu, int no)
{
  grub_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

static void
print_entry (int y, int highlight, grub_menu_entry_t entry)
{
  int x;
  const char *title;

  title = entry ? entry->title : "";
  
  grub_setcolorstate (highlight
		      ? GRUB_TERM_COLOR_HIGHLIGHT
		      : GRUB_TERM_COLOR_NORMAL);

  grub_gotoxy (2, y);
  grub_putchar (' ');
  for (x = 3; x < 75; x++)
    {
      if (*title && x <= 72)
	{
	  if (x == 72)
	    grub_putcode (DISP_RIGHT);
	  else
	    grub_putchar (*title++);
	}
      else
	grub_putchar (' ');
    }
  grub_gotoxy (74, y);

  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
}

static void
print_entries (grub_menu_t menu, int first, int offset)
{
  grub_menu_entry_t e;
  int i;
  
  grub_gotoxy (77, 4);

  if (first)
    grub_putcode (DISP_UP);
  else
    grub_putchar (' ');

  e = get_entry (menu, first);

  for (i = 0; i < 12; i++)
    {
      print_entry (4 + i, offset == i, e);
      if (e)
	e = e->next;
    }

  grub_gotoxy (77, 4 + 12);

  if (e)
    grub_putcode (DISP_DOWN);
  else
    grub_putchar (' ');

  grub_gotoxy (74, 4 + offset);
}

static void
init_page (int nested)
{
  grub_normal_init_page ();
  draw_border ();
  print_message (nested);
}

static int
run_menu (grub_menu_t menu, int nested)
{
  int first, offset;
  unsigned long saved_time;
  
  grub_setcursor (0);
  
  first = 0;
  offset = menu->default_entry;
  if (offset > 11)
    {
      first = offset - 11;
      offset = 11;
    }

  init_page (nested);
  print_entries (menu, first, offset);
  grub_refresh ();

  /* Initialize the time.  */
  saved_time = grub_get_rtc ();
  
  while (1)
    {
      int c;

      if (menu->timeout > 0)
	{
	  unsigned long current_time;

	  current_time = grub_get_rtc ();
	  if (current_time - saved_time >= GRUB_TICKS_PER_SECOND)
	    {
	      menu->timeout--;
	      saved_time = current_time;
	    }
	  
	  grub_gotoxy (3, 22);
	  grub_printf ("The highlighted entry will be booted automatically in %d seconds.    ",
		       menu->timeout);
	  grub_gotoxy (74, 4 + offset);
	  grub_refresh ();
	}

      if (menu->timeout == 0)
	return menu->default_entry;
      
      if (grub_checkkey () >= 0 || menu->timeout < 0)
	{
	  c = GRUB_TERM_ASCII_CHAR (grub_getkey ());
	  
	  if (menu->timeout >= 0)
	    {
	      grub_gotoxy (3, 22);
              grub_printf ("                                                                 ");
              menu->timeout = -1;
              menu->fallback_entry = -1;
	      grub_gotoxy (74, 4 + offset);
	    }
	  
	  switch (c)
	    {
	    case 16:
	    case '^':
	      if (offset > 0)
		{
		  print_entry (4 + offset, 0,
			       get_entry (menu, first + offset));
		  offset--;
		  print_entry (4 + offset, 1,
			       get_entry (menu, first + offset));
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
	      grub_setcursor (1);
	      return first + offset;
	      
	    case '\e':
	      if (nested)
		{
		  grub_setcursor (1);
		  return -1;
		}
	      break;
	      
	    case 'c':
	      grub_setcursor (1);
	      grub_cmdline_run (1);
	      grub_setcursor (0);
	      init_page (nested);
	      print_entries (menu, first, offset);
	      break;
	      
	    default:
	      break;
	    }
	  
	  grub_refresh ();
	}
    }

  /* Never reach here.  */
  return -1;
}

/* Run a menu entry.  */
static void
run_menu_entry (grub_menu_entry_t entry)
{
  grub_command_list_t cl;

  for (cl = entry->command_list; cl != 0; cl = cl->next)
    {
      grub_command_t c;
      
      c = grub_command_find (cl->command);
      if (! c)
	break;
      
      if (! (c->flags & GRUB_COMMAND_FLAG_CMDLINE))
	{
	  grub_error (GRUB_ERR_INVALID_COMMAND,
		      "invalid command `%s'",
		      cl->command);
	  break;
	}
      
      if (! (c->flags & GRUB_COMMAND_FLAG_NO_ECHO))
	grub_printf ("%s\n", cl->command);
      
      if (grub_command_execute (cl->command) != 0)
	break;
    }
  
  if (grub_errno == GRUB_ERR_NONE && grub_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    grub_command_execute ("boot");
}

void
grub_menu_run (grub_menu_t menu, int nested)
{
  while (1)
    {
      int boot_entry;
      grub_menu_entry_t e;
      
      boot_entry = run_menu (menu, nested);
      if (boot_entry < 0)
	break;

      grub_cls ();
      grub_setcursor (1);

      e = get_entry (menu, boot_entry);
      grub_printf ("  Booting \'%s\'\n\n", e->title);
  
      run_menu_entry (e);

      /* Deal with a fallback entry.  */
      /* FIXME: Multiple fallback entries like GRUB Legacy.  */
      if (menu->fallback_entry >= 0)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  
	  e = get_entry (menu, menu->fallback_entry);
	  menu->fallback_entry = -1;
	  grub_printf ("\n  Falling back to \'%s\'\n\n", e->title);
	  run_menu_entry (e);
	}

      if (grub_errno != GRUB_ERR_NONE)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;

	  /* Wait until the user pushes any key so that the user
	     can see what happened.  */
	  grub_printf ("\nPress any key to continue...");
	  (void) grub_getkey ();
	}
    }
}
