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
#include <grub/mm.h>
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

/* FIXME: These should be dynamically obtained from a terminal.  */
#define TERM_WIDTH	80
#define TERM_HEIGHT	25

/* The number of lines of "GRUB version..." at the top.  */
#define TERM_INFO_HEIGHT	1

/* The number of columns/lines between messages/borders/etc.  */
#define TERM_MARGIN	1

/* The number of columns of scroll information.  */
#define TERM_SCROLL_WIDTH	1

/* The Y position of the top border.  */
#define TERM_TOP_BORDER_Y	(TERM_MARGIN + TERM_INFO_HEIGHT + TERM_MARGIN)

/* The X position of the left border.  */
#define TERM_LEFT_BORDER_X	TERM_MARGIN

/* The width of the border.  */
#define TERM_BORDER_WIDTH	(TERM_WIDTH \
                                 - TERM_MARGIN * 3 \
				 - TERM_SCROLL_WIDTH)

/* The number of lines of messages at the bottom.  */
#define TERM_MESSAGE_HEIGHT	8

/* The height of the border.  */
#define TERM_BORDER_HEIGHT	(TERM_HEIGHT \
                                 - TERM_TOP_BORDER_Y \
                                 - TERM_MESSAGE_HEIGHT)

/* The number of entries shown at a time.  */
#define TERM_NUM_ENTRIES	(TERM_BORDER_HEIGHT - 2)

/* The Y position of the first entry.  */
#define TERM_FIRST_ENTRY_Y	(TERM_TOP_BORDER_Y + 1)

/* The max column number of an entry. The last "-1" is for a
   continuation marker.  */
#define TERM_ENTRY_WIDTH	(TERM_BORDER_WIDTH - 2 - TERM_MARGIN * 2 - 1)

/* The standard X position of the cursor.  */
#define TERM_CURSOR_X		(TERM_LEFT_BORDER_X \
                                 + TERM_BORDER_WIDTH \
                                 - TERM_MARGIN \
                                 - 1)

static void
draw_border (void)
{
  unsigned i;
  
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);
  
  grub_gotoxy (TERM_MARGIN, TERM_TOP_BORDER_Y);
  grub_putcode (DISP_UL);
  for (i = 0; i < TERM_BORDER_WIDTH - 2; i++)
    grub_putcode (DISP_HLINE);
  grub_putcode (DISP_UR);

  for (i = 0; i < (unsigned) TERM_NUM_ENTRIES; i++)
    {
      grub_gotoxy (TERM_MARGIN, TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (DISP_VLINE);
      grub_gotoxy (TERM_MARGIN + TERM_BORDER_WIDTH - 1, TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (DISP_VLINE);
    }

  grub_gotoxy (TERM_MARGIN, TERM_TOP_BORDER_Y + TERM_NUM_ENTRIES + 1);
  grub_putcode (DISP_LL);
  for (i = 0; i < TERM_BORDER_WIDTH - 2; i++)
    grub_putcode (DISP_HLINE);
  grub_putcode (DISP_LR);

  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

  grub_gotoxy (TERM_MARGIN, TERM_TOP_BORDER_Y + TERM_NUM_ENTRIES + TERM_MARGIN + 1);
}

static void
print_message (int nested, int edit)
{
  if (edit)
    {
      grub_printf ("\n\
      Minimum Emacs-like screen editing is supported. TAB lists\n\
      available completions. Press C-x (\'x\' with Ctrl) to boot,\n\
      C-c (\'c\' with Ctrl) for a command-line or ESC to return menu.");
    }
  else
    {
      grub_printf ("\n\
      Use the %C and %C keys to select which entry is highlighted.\n",
		   (grub_uint32_t) DISP_UP, (grub_uint32_t) DISP_DOWN);
      grub_printf ("\
      Press enter to boot the selected OS, \'e\' to edit the\n\
      commands before booting or \'c\' for a command-line.");
      if (nested)
	grub_printf ("\n\
      ESC to return previous menu.");
    }
  
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

  grub_gotoxy (TERM_LEFT_BORDER_X + TERM_MARGIN, y);
  grub_putchar (' ');
  for (x = TERM_LEFT_BORDER_X + TERM_MARGIN + 1;
       x < TERM_LEFT_BORDER_X + TERM_BORDER_WIDTH - TERM_MARGIN;
       x++)
    {
      if (*title && x <= TERM_LEFT_BORDER_X + TERM_BORDER_WIDTH - TERM_MARGIN - 1)
	{
	  if (x == TERM_LEFT_BORDER_X + TERM_BORDER_WIDTH - TERM_MARGIN - 1)
	    grub_putcode (DISP_RIGHT);
	  else
	    grub_putchar (*title++);
	}
      else
	grub_putchar (' ');
    }
  grub_gotoxy (TERM_CURSOR_X, y);

  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
}

static void
print_entries (grub_menu_t menu, int first, int offset)
{
  grub_menu_entry_t e;
  int i;
  
  grub_gotoxy (TERM_LEFT_BORDER_X + TERM_BORDER_WIDTH,
	       TERM_FIRST_ENTRY_Y);

  if (first)
    grub_putcode (DISP_UP);
  else
    grub_putchar (' ');

  e = get_entry (menu, first);

  for (i = 0; i < TERM_NUM_ENTRIES; i++)
    {
      print_entry (TERM_FIRST_ENTRY_Y + i, offset == i, e);
      if (e)
	e = e->next;
    }

  grub_gotoxy (TERM_LEFT_BORDER_X + TERM_BORDER_WIDTH,
	       TERM_TOP_BORDER_Y + TERM_NUM_ENTRIES);

  if (e)
    grub_putcode (DISP_DOWN);
  else
    grub_putchar (' ');

  grub_gotoxy (TERM_CURSOR_X, TERM_FIRST_ENTRY_Y + offset);
}

static void
init_page (int nested, int edit)
{
  grub_normal_init_page ();
  draw_border ();
  print_message (nested, edit);
}

/* Edit a menu entry with an Emacs-like interface.  */
static void
edit_menu_entry (grub_menu_entry_t entry)
{
  /* Not yet implemented.  */
}

static int
run_menu (grub_menu_t menu, int nested)
{
  int first, offset;
  unsigned long saved_time;
  
  first = 0;
  offset = menu->default_entry;
  if (offset > TERM_NUM_ENTRIES - 1)
    {
      first = offset - (TERM_NUM_ENTRIES - 1);
      offset = TERM_NUM_ENTRIES - 1;
    }

  /* Initialize the time.  */
  saved_time = grub_get_rtc ();

 refresh:
  grub_setcursor (0);
  init_page (nested, 0);
  print_entries (menu, first, offset);
  grub_refresh ();

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
	  
	  grub_gotoxy (0, TERM_HEIGHT - 3);
	  /* NOTE: Do not remove the trailing space characters.
	     They are required to clear the line.  */
	  grub_printf ("\
   The highlighted entry will be booted automatically in %d seconds.    ",
		       menu->timeout);
	  grub_gotoxy (TERM_CURSOR_X, TERM_FIRST_ENTRY_Y + offset);
	  grub_refresh ();
	}

      if (menu->timeout == 0)
	return menu->default_entry;
      
      if (grub_checkkey () >= 0 || menu->timeout < 0)
	{
	  c = GRUB_TERM_ASCII_CHAR (grub_getkey ());
	  
	  if (menu->timeout >= 0)
	    {
	      grub_gotoxy (0, TERM_HEIGHT - 3);
              grub_printf ("\
                                                                        ");
              menu->timeout = -1;
              menu->fallback_entry = -1;
	      grub_gotoxy (TERM_CURSOR_X, TERM_FIRST_ENTRY_Y + offset);
	    }
	  
	  switch (c)
	    {
	    case 16:
	    case '^':
	      if (offset > 0)
		{
		  print_entry (TERM_FIRST_ENTRY_Y + offset, 0,
			       get_entry (menu, first + offset));
		  offset--;
		  print_entry (TERM_FIRST_ENTRY_Y + offset, 1,
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
		  if (offset < TERM_NUM_ENTRIES - 1)
		    {
		      print_entry (TERM_FIRST_ENTRY_Y + offset, 0,
				   get_entry (menu, first + offset));
		      offset++;
		      print_entry (TERM_FIRST_ENTRY_Y + offset, 1,
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
	      goto refresh;

	    case 'e':
	      edit_menu_entry (get_entry (menu, first + offset));
	      goto refresh;
	      
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

      if (cl->command[0] == '\0')
	/* Ignore an empty command line.  */
	continue;
      
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
