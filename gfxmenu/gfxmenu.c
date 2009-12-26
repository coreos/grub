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

void 
grub_gfxmenu_viewer_fini (void *data)
{
  grub_gfxmenu_view_t view = data;

  grub_gfxmenu_view_destroy (view);
}

/* FIXME: 't' and 'c'.  */
grub_err_t
grub_gfxmenu_try (int entry, grub_menu_t menu, int nested)
{
  grub_gfxmenu_view_t view;
  const char *theme_path;
  struct grub_menu_viewer *instance;

  theme_path = grub_env_get ("theme");
  if (! theme_path)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "no theme specified");

  instance = grub_zalloc (sizeof (*instance));
  if (!instance)
    return grub_errno;

  /* Create the view.  */
  view = grub_gfxmenu_view_new (theme_path, menu, entry, nested);

  if (! view)
    {
      grub_free (instance);
      return grub_errno;
    }

  grub_gfxmenu_view_draw (view);

  instance->data = view;
  instance->set_chosen_entry = grub_gfxmenu_set_chosen_entry;
  instance->fini = grub_gfxmenu_viewer_fini;
  instance->print_timeout = grub_gfxmenu_print_timeout;
  instance->clear_timeout = grub_gfxmenu_clear_timeout;

  grub_menu_register_viewer (instance);

  return GRUB_ERR_NONE;
}

GRUB_MOD_INIT (gfxmenu)
{
  grub_gfxmenu_try_hook = grub_gfxmenu_try;
}

GRUB_MOD_FINI (gfxmenu)
{
  grub_gfxmenu_try_hook = NULL;
}
