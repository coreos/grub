/* gfxmenu.c - Graphical menu interface controller. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/video.h>
#include <grub/gfxterm.h>
#include <grub/bitmap.h>
#include <grub/bitmap_scale.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/gfxwidgets.h>
#include <grub/menu.h>
#include <grub/menu_viewer.h>
#include <grub/gfxmenu_model.h>
#include <grub/gfxmenu_view.h>
#include <grub/time.h>

static void switch_to_text_menu (void)
{
  grub_env_set ("menuviewer", "text");
}

static void
process_key_press (int c,
                   grub_gfxmenu_model_t model,
                   grub_gfxmenu_view_t view,
                   int nested,
                   int *should_exit)
{
  /* When a key is pressed, stop the timeout.  */
  grub_gfxmenu_model_clear_timeout (model);

  switch (c)
    {
    case 'j':
    case GRUB_TERM_DOWN:
      {
	int i = grub_gfxmenu_model_get_selected_index (model);
	int num_items = grub_gfxmenu_model_get_num_entries (model);
	if (i < num_items - 1)
	  {
	    i++;
	    grub_gfxmenu_model_set_selected_index (model, i);
	    grub_gfxmenu_redraw_menu (view);
	  }
      }
    break;

    case 'k':
    case GRUB_TERM_UP:
      {
	int i = grub_gfxmenu_model_get_selected_index (model);
	if (i > 0)
	  {
	    i--;
	    grub_gfxmenu_model_set_selected_index (model, i);
	    grub_gfxmenu_redraw_menu (view);
	  }
      }
    break;

    case '\r':
    case '\n':
    case GRUB_TERM_RIGHT:
      {
	int selected = grub_gfxmenu_model_get_selected_index (model);
	int num_entries = grub_gfxmenu_model_get_num_entries (model);
	if (selected >= 0 && selected < num_entries)
	  {
	    grub_menu_entry_t entry =
	      grub_gfxmenu_model_get_entry (model, selected);
	    grub_gfxmenu_view_execute_entry (view, entry);
	  }
      }
    break;

    case 'c':
      grub_gfxmenu_view_run_terminal (view);
      break;

    case 't':
      /* The write hook for 'menuviewer' will cause
       * grub_menu_viewer_should_return to return nonzero. */
      switch_to_text_menu ();
      *should_exit = 1;
      break;

    case GRUB_TERM_ESC:
      if (nested)
	*should_exit = 1;
      break;
    }

  if (grub_errno != GRUB_ERR_NONE)
    *should_exit = 1;
}

static void
handle_key_events (grub_gfxmenu_model_t model,
                   grub_gfxmenu_view_t view,
                   int nested,
                   int *should_exit)
{
  while ((! *should_exit) && (grub_checkkey () != -1))
    {
      int key = grub_getkey ();
      int c = GRUB_TERM_ASCII_CHAR (key);
      process_key_press (c, model, view, nested, should_exit);
    }
}

static grub_err_t
show_menu (grub_menu_t menu, int nested)
{
  grub_gfxmenu_model_t model;
  grub_gfxmenu_view_t view;
  const char *theme_path;

  theme_path = grub_env_get ("theme");
  if (! theme_path)
    {
      switch_to_text_menu ();
      return grub_error (GRUB_ERR_FILE_NOT_FOUND, "no theme specified");
    }

  model = grub_gfxmenu_model_new (menu);
  if (! model)
    {
      switch_to_text_menu ();
      return grub_errno;
    }

  /* Create the view.  */
  view = grub_gfxmenu_view_new (theme_path, model);

  if (! view)
    {
      grub_print_error ();
      grub_gfxmenu_model_destroy (model);
      switch_to_text_menu ();
      return grub_errno;
    }

  /* Initially select the default menu entry.  */
  int default_index = grub_menu_get_default_entry_index (menu);
  grub_gfxmenu_model_set_selected_index (model, default_index);

  /* Start the timer to execute the default entry.  */
  grub_gfxmenu_model_set_timeout (model);

  /* Main event loop.  */
  int exit_requested = 0;

  grub_gfxmenu_view_draw (view);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_draw (view);

  while ((! exit_requested) && (! grub_menu_viewer_should_return ()))
    {
      grub_gfxmenu_redraw_timeout (view);
      if (grub_gfxmenu_model_timeout_expired (model))
        {
          grub_gfxmenu_model_clear_timeout (model);
          int i = grub_gfxmenu_model_get_selected_index (model);
          grub_menu_entry_t e = grub_gfxmenu_model_get_entry (model, i);
          grub_gfxmenu_view_execute_with_fallback (view, e);
          continue;
        }

      handle_key_events (model, view, nested, &exit_requested);
      grub_cpu_idle ();
    }

  grub_gfxmenu_view_destroy (view);
  grub_gfxmenu_model_destroy (model);

  return grub_errno;
}

static grub_err_t
grub_cmd_gfxmenu (grub_command_t cmd __attribute__ ((unused)),
                  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  grub_menu_t menu = grub_env_get_data_slot ("menu");
  if (! menu)
    return grub_error (GRUB_ERR_MENU, "no menu context");

  return show_menu (menu, 1);
}

static struct grub_menu_viewer menu_viewer =
{
  .name = "gfxmenu",
  .show_menu = show_menu
};

static grub_command_t cmd;

GRUB_MOD_INIT (gfxmenu)
{
  (void) mod;                   /* To stop warning. */
  grub_menu_viewer_register (&menu_viewer);
  cmd = grub_register_command ("gfxmenu", grub_cmd_gfxmenu,
                               "gfxmenu",
                               "Show graphical menu interface");
}

GRUB_MOD_FINI (gfxmenu)
{
  grub_unregister_command (cmd);
}
