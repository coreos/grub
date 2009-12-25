/* menu_text.c - Basic text menu implementation.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/env.h>
#include <grub/menu_viewer.h>
#include <grub/i18n.h>
#include <grub/charset.h>

static grub_uint8_t grub_color_menu_normal;
static grub_uint8_t grub_color_menu_highlight;

struct menu_viewer_data
{
  int first, offset;
  grub_menu_t menu;
  struct grub_term_output *term;
};

static void
print_spaces (int number_spaces, struct grub_term_output *term)
{
  int i;
  for (i = 0; i < number_spaces; i++)
    grub_putcode (' ', term);
}

void
grub_print_ucs4 (const grub_uint32_t * str,
		 const grub_uint32_t * last_position,
		 struct grub_term_output *term)
{
  while (str < last_position)
    {
      grub_putcode (*str, term);
      str++;
    }
}

grub_ssize_t
grub_getstringwidth (grub_uint32_t * str, const grub_uint32_t * last_position,
		     struct grub_term_output *term)
{
  grub_ssize_t width = 0;

  while (str < last_position)
    {
      width += grub_term_getcharwidth (term, *str);
      str++;
    }
  return width;
}

void
grub_print_message_indented (const char *msg, int margin_left, int margin_right,
			     struct grub_term_output *term)
{
  int line_len;

  grub_uint32_t *unicode_msg;
  grub_uint32_t *last_position;

  int msg_len;

  line_len = grub_term_width (term) - grub_term_getcharwidth (term, 'm') *
    (margin_left + margin_right);

  msg_len = grub_utf8_to_ucs4_alloc (msg, &unicode_msg, &last_position);

  if (msg_len < 0)
    {
      return;
    }

  grub_uint32_t *current_position = unicode_msg;

  grub_uint32_t *next_new_line = unicode_msg;

  int first_loop = 1;

  while (current_position < last_position)
    {
      if (! first_loop)
        grub_putcode ('\n', term);
     
      next_new_line = (grub_uint32_t *) last_position;

      while (grub_getstringwidth (current_position, next_new_line,term) 
	     > line_len
            || (*next_new_line != ' ' && next_new_line > current_position &&
                next_new_line != last_position))
       {
         next_new_line--;
       }

      if (next_new_line == current_position)
       {
         next_new_line = (next_new_line + line_len > last_position) ?
           (grub_uint32_t *) last_position : next_new_line + line_len;
       }

      print_spaces (margin_left, term);
      grub_print_ucs4 (current_position, next_new_line, term);

      next_new_line++;
      current_position = next_new_line;
      first_loop = 0;
    }
  grub_free (unicode_msg);
}


static void
draw_border (struct grub_term_output *term)
{
  unsigned i;

  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y);
  grub_putcode (GRUB_TERM_DISP_UL, term);
  for (i = 0; i < (unsigned) grub_term_border_width (term) - 2; i++)
    grub_putcode (GRUB_TERM_DISP_HLINE, term);
  grub_putcode (GRUB_TERM_DISP_UR, term);

  for (i = 0; i < (unsigned) grub_term_num_entries (term); i++)
    {
      grub_term_gotoxy (term, GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_TERM_DISP_VLINE, term);
      grub_term_gotoxy (term, GRUB_TERM_MARGIN + grub_term_border_width (term)
			- 1,
			GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_TERM_DISP_VLINE, term);
    }

  grub_term_gotoxy (term, GRUB_TERM_MARGIN,
		    GRUB_TERM_TOP_BORDER_Y + grub_term_num_entries (term) + 1);
  grub_putcode (GRUB_TERM_DISP_LL, term);
  for (i = 0; i < (unsigned) grub_term_border_width (term) - 2; i++)
    grub_putcode (GRUB_TERM_DISP_HLINE, term);
  grub_putcode (GRUB_TERM_DISP_LR, term);

  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_MARGIN,
		    (GRUB_TERM_TOP_BORDER_Y + grub_term_num_entries (term)
		     + GRUB_TERM_MARGIN + 1));
}

static void
print_message (int nested, int edit, struct grub_term_output *term)
{
  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  if (edit)
    {
      grub_putcode ('\n', term);
      grub_print_message_indented (_("Minimum Emacs-like screen editing is \
supported. TAB lists completions. Press Ctrl-x to boot, Ctrl-c for a \
command-line or ESC to return menu."), STANDARD_MARGIN, STANDARD_MARGIN,
				   term);
    }
  else
    {
      const char *msg = _("Use the %C and %C keys to select which \
entry is highlighted.\n");
      char *msg_translated =
       grub_malloc (sizeof (char) * grub_strlen (msg) + 1);

      grub_sprintf (msg_translated, msg, (grub_uint32_t) GRUB_TERM_DISP_UP,
                   (grub_uint32_t) GRUB_TERM_DISP_DOWN);
      grub_putchar ('\n');
      grub_print_message_indented (msg_translated, STANDARD_MARGIN,
				   STANDARD_MARGIN, term);

      grub_free (msg_translated);

      if (nested)
	{
	  grub_print_message_indented
	    (_("Press enter to boot the selected OS, "
	       "\'e\' to edit the commands before booting "
	       "or \'c\' for a command-line. ESC to return previous menu.\n"),
	     STANDARD_MARGIN, STANDARD_MARGIN, term);
	}
      else
	{
	  grub_print_message_indented
	    (_("Press enter to boot the selected OS, "
	       "\'e\' to edit the commands before booting "
	       "or \'c\' for a command-line.\n"),
	     STANDARD_MARGIN, STANDARD_MARGIN, term);
	}	
    }
}

static void
print_entry (int y, int highlight, grub_menu_entry_t entry,
	     struct grub_term_output *term)
{
  int x;
  const char *title;
  grub_size_t title_len;
  grub_ssize_t len;
  grub_uint32_t *unicode_title;
  grub_ssize_t i;
  grub_uint8_t old_color_normal, old_color_highlight;

  title = entry ? entry->title : "";
  title_len = grub_strlen (title);
  unicode_title = grub_malloc (title_len * sizeof (*unicode_title));
  if (! unicode_title)
    /* XXX How to show this error?  */
    return;

  len = grub_utf8_to_ucs4 (unicode_title, title_len,
                           (grub_uint8_t *) title, -1, 0);
  if (len < 0)
    {
      /* It is an invalid sequence.  */
      grub_free (unicode_title);
      return;
    }

  grub_term_getcolor (term, &old_color_normal, &old_color_highlight);
  grub_term_setcolor (term, grub_color_menu_normal, grub_color_menu_highlight);
  grub_term_setcolorstate (term, highlight
			   ? GRUB_TERM_COLOR_HIGHLIGHT
			   : GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN, y);

  for (x = GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + 1, i = 0;
       x < (int) (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term)
		  - GRUB_TERM_MARGIN);
       i++)
    {
      if (i < len
	  && x <= (int) (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term)
			 - GRUB_TERM_MARGIN - 1))
	{
	  grub_ssize_t width;

	  width = grub_term_getcharwidth (term, unicode_title[i]);

	  if (x + width > (int) (GRUB_TERM_LEFT_BORDER_X 
				 + grub_term_border_width (term)
				 - GRUB_TERM_MARGIN - 1))
	    grub_putcode (GRUB_TERM_DISP_RIGHT, term);
	  else
	    grub_putcode (unicode_title[i], term);

	  x += width;
	}
      else
	{
	  grub_putcode (' ', term);
	  x++;
	}
    }
  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);
  grub_putcode (' ', term);

  grub_term_gotoxy (term, grub_term_cursor_x (term), y);

  grub_term_setcolor (term, old_color_normal, old_color_highlight);
  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);
  grub_free (unicode_title);
}

static void
print_entries (grub_menu_t menu, int first, int offset,
	       struct grub_term_output *term)
{
  grub_menu_entry_t e;
  int i;

  grub_term_gotoxy (term,
		    GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term),
		    GRUB_TERM_FIRST_ENTRY_Y);

  if (first)
    grub_putcode (GRUB_TERM_DISP_UP, term);
  else
    grub_putcode (' ', term);

  e = grub_menu_get_entry (menu, first);

  for (i = 0; i < grub_term_num_entries (term); i++)
    {
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + i, offset == i, e, term);
      if (e)
	e = e->next;
    }

  grub_term_gotoxy (term, GRUB_TERM_LEFT_BORDER_X
		    + grub_term_border_width (term),
		    GRUB_TERM_TOP_BORDER_Y + grub_term_num_entries (term));

  if (e)
    grub_putcode (GRUB_TERM_DISP_DOWN, term);
  else
    grub_putcode (' ', term);

  grub_term_gotoxy (term, grub_term_cursor_x (term),
		    GRUB_TERM_FIRST_ENTRY_Y + offset);
}

/* Initialize the screen.  If NESTED is non-zero, assume that this menu
   is run from another menu or a command-line. If EDIT is non-zero, show
   a message for the menu entry editor.  */
void
grub_menu_init_page (int nested, int edit,
		     struct grub_term_output *term)
{
  grub_uint8_t old_color_normal, old_color_highlight;

  grub_term_getcolor (term, &old_color_normal, &old_color_highlight);

  /* By default, use the same colors for the menu.  */
  grub_color_menu_normal = old_color_normal;
  grub_color_menu_highlight = old_color_highlight;

  /* Then give user a chance to replace them.  */
  grub_parse_color_name_pair (&grub_color_menu_normal,
			      grub_env_get ("menu_color_normal"));
  grub_parse_color_name_pair (&grub_color_menu_highlight,
			      grub_env_get ("menu_color_highlight"));

  grub_normal_init_page (term);
  grub_term_setcolor (term, grub_color_menu_normal, grub_color_menu_highlight);
  draw_border (term);
  grub_term_setcolor (term, old_color_normal, old_color_highlight);
  print_message (nested, edit, term);
}

static void
menu_text_print_timeout (int timeout, void *dataptr)
{
  const char *msg =
    _("The highlighted entry will be booted automatically in %ds.");
  struct menu_viewer_data *data = dataptr;
  char *msg_translated;
  int posx;

  grub_term_gotoxy (data->term, 0, grub_term_height (data->term) - 3);

  msg_translated = grub_malloc (sizeof (char) * grub_strlen (msg) + 5);
  if (!msg_translated)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_sprintf (msg_translated, msg, timeout);
  grub_print_message_indented (msg_translated, 3, 0, data->term);
 
  posx = grub_term_getxy (data->term) >> 8;
  print_spaces (grub_term_width (data->term) - posx - 1, data->term);

  grub_term_gotoxy (data->term,
		    grub_term_cursor_x (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y + data->offset);
  grub_term_refresh (data->term);
}

static void
menu_text_set_chosen_entry (int entry, void *dataptr)
{
  struct menu_viewer_data *data = dataptr;
  int oldoffset = data->offset;
  int complete_redraw = 0;

  data->offset = entry - data->first;
  if (data->offset > grub_term_num_entries (data->term) - 1)
    {
      data->first = entry - (grub_term_num_entries (data->term) - 1);
      data->offset = grub_term_num_entries (data->term) - 1;
      complete_redraw = 1;
    }
  if (data->offset < 0)
    {
      data->offset = 0;
      data->first = entry;
      complete_redraw = 1;
    }
  if (complete_redraw)
    print_entries (data->menu, data->first, data->offset, data->term);
  else
    {
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + oldoffset, 0,
		   grub_menu_get_entry (data->menu, data->first + oldoffset),
		   data->term);
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + data->offset, 1,
		   grub_menu_get_entry (data->menu, data->first + data->offset),
		   data->term);
    }
  grub_term_refresh (data->term);
}

static void
menu_text_fini (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;

  grub_term_setcursor (data->term, 1);
  grub_term_cls (data->term);

}

static void
menu_text_clear_timeout (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;

  grub_term_gotoxy (data->term, 0, grub_term_height (data->term) - 3);
  print_spaces (grub_term_width (data->term) - 1, data->term);
  grub_term_gotoxy (data->term, grub_term_cursor_x (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y + data->offset);
  grub_term_refresh (data->term);
}

void
grub_menu_text_register_instances (int entry, grub_menu_t menu, int nested)
{
  struct menu_viewer_data *data;
  struct grub_menu_viewer *instance;
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    instance = grub_zalloc (sizeof (*instance));
    if (!instance)
      {
	grub_print_error ();
	grub_errno = GRUB_ERR_NONE;
	continue;
      }
    data = grub_zalloc (sizeof (*data));
    if (!data)
      {
	grub_free (instance);
	grub_print_error ();
	grub_errno = GRUB_ERR_NONE;
	continue;
      }

    data->term = term;
    instance->data = data;
    instance->set_chosen_entry = menu_text_set_chosen_entry;
    instance->print_timeout = menu_text_print_timeout;
    instance->clear_timeout = menu_text_clear_timeout;
    instance->fini = menu_text_fini;

    data->menu = menu;
    
    data->offset = entry;
    data->first = 0;
    if (data->offset > grub_term_num_entries (data->term) - 1)
      {
	data->first = data->offset - (grub_term_num_entries (data->term) - 1);
	data->offset = grub_term_num_entries (data->term) - 1;
      }

    grub_term_setcursor (data->term, 0);
    grub_menu_init_page (nested, 0, data->term);
    print_entries (menu, data->first, data->offset, data->term);
    grub_term_refresh (data->term);
    grub_menu_register_viewer (instance);
  }
}
