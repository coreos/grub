/* view.c - Graphical menu interface MVC view. */
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
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/video.h>
#include <grub/gui_string_util.h>
#include <grub/gfxterm.h>
#include <grub/bitmap.h>
#include <grub/bitmap_scale.h>
#include <grub/term.h>
#include <grub/gfxwidgets.h>
#include <grub/time.h>
#include <grub/menu.h>
#include <grub/menu_viewer.h>
#include <grub/gfxmenu_view.h>
#include <grub/gui.h>
#include <grub/icon_manager.h>

/* The component ID identifying GUI components to be updated as the timeout
   status changes.  */
#define TIMEOUT_COMPONENT_ID "__timeout__"

static grub_gfxmenu_view_t term_view;

static void init_terminal (grub_gfxmenu_view_t view);
static void destroy_terminal (void);
static grub_err_t set_graphics_mode (void);
static grub_err_t set_text_mode (void);

/* Create a new view object, loading the theme specified by THEME_PATH and
   associating MODEL with the view.  */
grub_gfxmenu_view_t
grub_gfxmenu_view_new (const char *theme_path, grub_gfxmenu_model_t model)
{
  grub_gfxmenu_view_t view;
  grub_err_t err;
  struct grub_video_mode_info mode_info;

  view = grub_malloc (sizeof (*view));
  if (! view)
    return 0;

  set_graphics_mode ();
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);
  grub_video_get_viewport ((unsigned *) &view->screen.x,
                           (unsigned *) &view->screen.y,
                           (unsigned *) &view->screen.width,
                           (unsigned *) &view->screen.height);

  err = grub_video_get_info (&mode_info);
  if (err)
    {
      grub_free (view);
      return 0;
    }
  else
    view->double_repaint = (mode_info.mode_type
			    & GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED)
      && !(mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP);


  /* Clear the screen; there may be garbage left over in video memory, and
     loading the menu style (particularly the background) can take a while. */
  grub_video_fill_rect (grub_video_map_rgb (0, 0, 0),
                        view->screen.x, view->screen.y,
                        view->screen.width, view->screen.height);
  grub_video_swap_buffers ();

  grub_font_t default_font;
  grub_gui_color_t default_fg_color;
  grub_gui_color_t default_bg_color;

  default_font = grub_font_get ("Helvetica 12");
  default_fg_color = grub_gui_color_rgb (0, 0, 0);
  default_bg_color = grub_gui_color_rgb (255, 255, 255);

  view->model = model;
  view->canvas = 0;

  view->title_font = default_font;
  view->message_font = default_font;
  view->terminal_font_name = grub_strdup ("Fixed 10");
  view->title_color = default_fg_color;
  view->message_color = default_bg_color;
  view->message_bg_color = default_fg_color;
  view->desktop_image = 0;
  view->desktop_color = default_bg_color;
  view->terminal_box = grub_gfxmenu_create_box (0, 0);
  view->title_text = grub_strdup ("GRUB Boot Menu");
  view->progress_message_text = 0;
  view->theme_path = 0;
  view->last_seconds_remaining = -2;

  /* Set the timeout bar's frame.  */
  view->progress_message_frame.width = view->screen.width * 4 / 5;
  view->progress_message_frame.height = 50;
  view->progress_message_frame.x = view->screen.x
    + (view->screen.width - view->progress_message_frame.width) / 2;
  view->progress_message_frame.y = view->screen.y
    + view->screen.height - 90 - 20 - view->progress_message_frame.height;

  if (grub_gfxmenu_view_load_theme (view, theme_path) != 0)
    {
      grub_gfxmenu_view_destroy (view);
      return 0;
    }

  init_terminal (view);

  return view;
}

/* Destroy the view object.  All used memory is freed.  */
void
grub_gfxmenu_view_destroy (grub_gfxmenu_view_t view)
{
  if (!view)
    return;
  grub_video_bitmap_destroy (view->desktop_image);
  if (view->terminal_box)
    view->terminal_box->destroy (view->terminal_box);
  grub_free (view->terminal_font_name);
  grub_free (view->title_text);
  grub_free (view->progress_message_text);
  grub_free (view->theme_path);
  if (view->canvas)
    view->canvas->component.ops->destroy (view->canvas);
  grub_free (view);

  set_text_mode ();
  destroy_terminal ();
}

/* Sets MESSAGE as the progress message for the view.
   MESSAGE can be 0, in which case no message is displayed.  */
static void
set_progress_message (grub_gfxmenu_view_t view, const char *message)
{
  grub_free (view->progress_message_text);
  if (message)
    view->progress_message_text = grub_strdup (message);
  else
    view->progress_message_text = 0;
}

static void
redraw_background (grub_gfxmenu_view_t view,
		   const grub_video_rect_t *bounds)
{
  if (view->desktop_image)
    {
      struct grub_video_bitmap *img = view->desktop_image;
      grub_video_blit_bitmap (img, GRUB_VIDEO_BLIT_REPLACE,
                              bounds->x, bounds->y,
			      bounds->x - view->screen.x,
			      bounds->y - view->screen.y,
			      bounds->width, bounds->height);
    }
  else
    {
      grub_video_fill_rect (grub_gui_map_color (view->desktop_color),
                            bounds->x, bounds->y,
                            bounds->width, bounds->height);
    }
}

static void
draw_title (grub_gfxmenu_view_t view)
{
  if (! view->title_text)
    return;

  /* Center the title. */
  int title_width = grub_font_get_string_width (view->title_font,
                                                view->title_text);
  int x = (view->screen.width - title_width) / 2;
  int y = 40 + grub_font_get_ascent (view->title_font);
  grub_font_draw_string (view->title_text,
                         view->title_font,
                         grub_gui_map_color (view->title_color),
                         x, y);
}

struct progress_value_data
{
  const char *visible;
  const char *start;
  const char *end;
  const char *value;
  const char *text;
};

static void
update_timeout_visit (grub_gui_component_t component,
                      void *userdata)
{
  struct progress_value_data *pv;
  pv = (struct progress_value_data *) userdata;

  component->ops->set_property (component, "visible", pv->visible);
  component->ops->set_property (component, "start", pv->start);
  component->ops->set_property (component, "end", pv->end);
  component->ops->set_property (component, "value", pv->value);
  component->ops->set_property (component, "text", pv->text);
}


static inline void
update_timeout (grub_gfxmenu_view_t view, int is_init)
{
  char startbuf[20];
  char valuebuf[20];
  char msgbuf[120];

  int timeout;
  int remaining;
  struct progress_value_data pv;
  int seconds_remaining_rounded_up;

  auto void redraw_timeout_visit (grub_gui_component_t component,
				  void *userdata __attribute__ ((unused)));

  auto void redraw_timeout_visit (grub_gui_component_t component,
				  void *userdata __attribute__ ((unused)))
  {
    grub_video_rect_t bounds;
    component->ops->get_bounds (component, &bounds);
    grub_gfxmenu_view_redraw (view, &bounds);
  }

  timeout = grub_gfxmenu_model_get_timeout_ms (view->model);
  if (timeout > 0)
    {
      remaining = grub_gfxmenu_model_get_timeout_remaining_ms (view->model);
      seconds_remaining_rounded_up = (remaining + 999) / 1000;
    }
  else
    {
      seconds_remaining_rounded_up = -1;
      remaining = -1;
    }

  if (view->last_seconds_remaining == seconds_remaining_rounded_up && !is_init)
    return;

  view->last_seconds_remaining = seconds_remaining_rounded_up;

  pv.visible = timeout > 0 ? "true" : "false";
  grub_sprintf (startbuf, "%d", -timeout);
  pv.start = startbuf;
  pv.end = "0";
  grub_sprintf (valuebuf, "%d", remaining > 0 ? -remaining : 0);
  pv.value = valuebuf;

  grub_sprintf (msgbuf,
                "The highlighted entry will be booted automatically in %d s.",
                seconds_remaining_rounded_up);
  pv.text = msgbuf;

  grub_gui_find_by_id ((grub_gui_component_t) view->canvas,
                       TIMEOUT_COMPONENT_ID, update_timeout_visit, &pv);
  if (!is_init)
    {
      grub_gui_find_by_id ((grub_gui_component_t) view->canvas,
			   TIMEOUT_COMPONENT_ID, redraw_timeout_visit, &pv);
      grub_video_swap_buffers ();
      if (view->double_repaint)
	grub_gui_find_by_id ((grub_gui_component_t) view->canvas,
			     TIMEOUT_COMPONENT_ID, redraw_timeout_visit, &pv);
    }
}

void
grub_gfxmenu_redraw_timeout (grub_gfxmenu_view_t view)
{
  update_timeout (view, 0);
}

static void
update_menu_visit (grub_gui_component_t component,
                   void *userdata)
{
  grub_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      grub_gui_list_t list = (grub_gui_list_t) component;
      list->ops->set_view_info (list, view->theme_path, view->model);
    }
}

/* Update any boot menu components with the current menu model and
   theme path.  */
static void
update_menu_components (grub_gfxmenu_view_t view)
{
  grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
                                update_menu_visit, view);
}

static void
draw_message (grub_gfxmenu_view_t view)
{
  char *text = view->progress_message_text;
  grub_video_rect_t f = view->progress_message_frame;
  if (! text)
    return;

  grub_font_t font = view->message_font;
  grub_video_color_t color = grub_gui_map_color (view->message_color);

  /* Border.  */
  grub_video_fill_rect (color,
                        f.x-1, f.y-1, f.width+2, f.height+2);
  /* Fill.  */
  grub_video_fill_rect (grub_gui_map_color (view->message_bg_color),
                        f.x, f.y, f.width, f.height);

  /* Center the text. */
  int text_width = grub_font_get_string_width (font, text);
  int x = f.x + (f.width - text_width) / 2;
  int y = (f.y + (f.height - grub_font_get_descent (font)) / 2
           + grub_font_get_ascent (font) / 2);
  grub_font_draw_string (text, font, color, x, y);
}

void
grub_gfxmenu_view_redraw (grub_gfxmenu_view_t view,
			  const grub_video_rect_t *region)
{
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  redraw_background (view, region);
  if (view->canvas)
    view->canvas->component.ops->paint (view->canvas, region);
  draw_title (view);
  if (grub_video_have_common_points (&view->progress_message_frame, region))
    draw_message (view);
}

void
grub_gfxmenu_view_draw (grub_gfxmenu_view_t view)
{
  update_timeout (view, 1);
  update_menu_components (view);

  grub_gfxmenu_view_redraw (view, &view->screen);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->screen);
}

static void
redraw_menu_visit (grub_gui_component_t component,
                   void *userdata)
{
  grub_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      grub_gui_list_t list;
      grub_video_rect_t bounds;

      list = (grub_gui_list_t) component;
      component->ops->get_bounds (component, &bounds);
      grub_gfxmenu_view_redraw (view, &bounds);
    }
}

void
grub_gfxmenu_redraw_menu (grub_gfxmenu_view_t view)
{
  update_menu_components (view);

  grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
                                redraw_menu_visit, view);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    {
      grub_gui_iterate_recursively ((grub_gui_component_t) view->canvas,
				    redraw_menu_visit, view);
    }
}

static grub_err_t
set_graphics_mode (void)
{
  const char *modestr = grub_env_get ("gfxmode");
  if (!modestr || !modestr[0])
    modestr = "auto";
  return grub_video_set_mode (modestr, GRUB_VIDEO_MODE_TYPE_PURE_TEXT, 0);
}

static grub_err_t
set_text_mode (void)
{
  return grub_video_restore ();
}

static int term_target_width;
static int term_target_height;
static int term_initialized;
static grub_term_output_t term_original;

static void
draw_terminal_box (grub_gfxmenu_view_t view)
{
  grub_gfxmenu_box_t term_box;
  int termx;
  int termy;

  term_box = term_view->terminal_box;
  if (!term_box)
    return;

  termx = term_view->screen.x + term_view->screen.width * (10 - 7) / 10 / 2;
  termy = term_view->screen.y + term_view->screen.height * (10 - 7) / 10 / 2;
  
  term_box->set_content_size (term_box, term_target_width,
			      term_target_height);
  
  term_box->draw (term_box,
		  termx - term_box->get_left_pad (term_box),
		  termy - term_box->get_top_pad (term_box));
  grub_video_swap_buffers ();
  if (view->double_repaint)
    term_box->draw (term_box,
		    termx - term_box->get_left_pad (term_box),
		    termy - term_box->get_top_pad (term_box));
}

static void
init_terminal (grub_gfxmenu_view_t view)
{
  int termx;
  int termy;

  term_original = grub_term_get_current_output ();

  term_target_width = view->screen.width * 7 / 10;
  term_target_height = view->screen.height * 7 / 10;

  termx = view->screen.x + view->screen.width * (10 - 7) / 10 / 2;
  termy = view->screen.y + view->screen.height * (10 - 7) / 10 / 2;

  /* Note: currently there is no API for changing the gfxterm font
     on the fly, so whatever font the initially loaded theme specifies
     will be permanent.  */
  grub_gfxterm_init_window (GRUB_VIDEO_RENDER_TARGET_DISPLAY, termx, termy,
                            term_target_width, term_target_height,
			    view->double_repaint, view->terminal_font_name, 3);
  if (grub_errno != GRUB_ERR_NONE)
    return;
  term_initialized = 1;

  term_view = view;

  grub_term_set_current_output (grub_gfxterm_get_term ());
  grub_refresh ();
}

static void destroy_terminal (void)
{
  if (term_initialized)
    grub_gfxterm_destroy_window ();
  if (term_original)
    grub_term_set_current_output (term_original);
}


static void
notify_booting (grub_menu_entry_t entry, void *userdata)
{
  grub_gfxmenu_view_t view = (grub_gfxmenu_view_t) userdata;

  char *s = grub_malloc (100 + grub_strlen (entry->title));
  if (!s)
    return;

  grub_sprintf (s, "Booting '%s'", entry->title);
  set_progress_message (view, s);
  grub_free (s);
  grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_fallback (grub_menu_entry_t entry, void *userdata)
{
  grub_gfxmenu_view_t view = (grub_gfxmenu_view_t) userdata;

  char *s = grub_malloc (100 + grub_strlen (entry->title));
  if (!s)
    return;

  grub_sprintf (s, "Falling back to '%s'", entry->title);
  set_progress_message (view, s);
  grub_free (s);
  grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_execution_failure (void *userdata __attribute__ ((unused)))
{
}


static struct grub_menu_execute_callback execute_callback =
{
  .notify_booting = notify_booting,
  .notify_fallback = notify_fallback,
  .notify_failure = notify_execution_failure
};

int
grub_gfxmenu_view_execute_with_fallback (grub_gfxmenu_view_t view,
                                         grub_menu_entry_t entry)
{
  draw_terminal_box (view);

  grub_menu_execute_with_fallback (grub_gfxmenu_model_get_menu (view->model),
                                   entry, &execute_callback, (void *) view);

  if (set_graphics_mode () != GRUB_ERR_NONE)
    return 0;  /* Failure.  */

  /* If we returned, there was a failure.  */
  set_progress_message (view,
                        "Unable to automatically boot.  "
                        "Press SPACE to continue.");
  grub_gfxmenu_view_draw (view);
  while (GRUB_TERM_ASCII_CHAR(grub_getkey ()) != ' ')
    {
      /* Wait for SPACE to be pressed.  */
    }

  set_progress_message (view, 0);   /* Clear the message.  */

  grub_gfxmenu_view_redraw (view, &view->progress_message_frame);
  grub_video_swap_buffers ();
  if (view->double_repaint)
    grub_gfxmenu_view_redraw (view, &view->progress_message_frame);

  return 1;   /* Ok.  */
}

int
grub_gfxmenu_view_execute_entry (grub_gfxmenu_view_t view,
                                 grub_menu_entry_t entry)
{
  draw_terminal_box (view);

  grub_menu_execute_entry (entry);
  if (grub_errno != GRUB_ERR_NONE)
    grub_wait_after_message ();

  if (set_graphics_mode () != GRUB_ERR_NONE)
    return 0;  /* Failure.  */

  grub_gfxmenu_view_draw (view);
  return 1;   /* Ok.  */
}

void
grub_gfxmenu_view_run_terminal (grub_gfxmenu_view_t view)
{
  draw_terminal_box (view);
  grub_cmdline_run (1);
  grub_gfxmenu_view_draw (view);
}
