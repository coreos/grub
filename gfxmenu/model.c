/* model.c - Graphical menu interface MVC model. */
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
#include <grub/normal.h>
#include <grub/menu.h>
#include <grub/time.h>
#include <grub/gfxmenu_model.h>

/* Model type definition.  */
struct grub_gfxmenu_model
{
  grub_menu_t menu;
  int num_entries;
  grub_menu_entry_t *entries;
  int selected_entry_index;
  int timeout_set;
  grub_uint64_t timeout_start;
  grub_uint64_t timeout_at;
};


grub_gfxmenu_model_t
grub_gfxmenu_model_new (grub_menu_t menu)
{
  grub_gfxmenu_model_t model;

  model = grub_malloc (sizeof (*model));
  if (! model)
    return 0;

  model->menu = menu;
  model->num_entries = menu->size;
  model->entries = 0;
  model->selected_entry_index = 0;
  model->timeout_set = 0;
  model->timeout_at = 0;
  if (model->num_entries > 0)
    {
      model->entries = grub_malloc (model->num_entries
                                    * sizeof (*model->entries));
      if (! model->entries)
        goto fail_and_free;

      int i;
      grub_menu_entry_t cur;
      for (i = 0, cur = menu->entry_list;
           i < model->num_entries;
           i++, cur = cur->next)
        {
          model->entries[i] = cur;
        }
    }

  return model;

fail_and_free:
  grub_free (model->entries);
  grub_free (model);
  return 0;
}

void
grub_gfxmenu_model_destroy (grub_gfxmenu_model_t model)
{
  if (! model)
    return;

  grub_free (model->entries);
  model->entries = 0;

  grub_free (model);
}

grub_menu_t
grub_gfxmenu_model_get_menu (grub_gfxmenu_model_t model)
{
  return model->menu;
}

void
grub_gfxmenu_model_set_timeout (grub_gfxmenu_model_t model)
{
  int timeout_sec = grub_menu_get_timeout ();
  if (timeout_sec >= 0)
    {
      model->timeout_start = grub_get_time_ms ();
      model->timeout_at = model->timeout_start + timeout_sec * 1000;
      model->timeout_set = 1;
    }
  else
    {
      model->timeout_set = 0;
    }
}

void
grub_gfxmenu_model_clear_timeout (grub_gfxmenu_model_t model)
{
  model->timeout_set = 0;
  grub_menu_set_timeout (-1);
}

int
grub_gfxmenu_model_get_timeout_ms (grub_gfxmenu_model_t model)
{
  if (!model->timeout_set)
    return -1;

  return model->timeout_at - model->timeout_start;
}

int
grub_gfxmenu_model_get_timeout_remaining_ms (grub_gfxmenu_model_t model)
{
  if (!model->timeout_set)
    return -1;

  return model->timeout_at - grub_get_time_ms ();
}

int
grub_gfxmenu_model_timeout_expired (grub_gfxmenu_model_t model)
{
  if (model->timeout_set
      && grub_get_time_ms () >= model->timeout_at)
    return 1;

  return 0;
}

int
grub_gfxmenu_model_get_num_entries (grub_gfxmenu_model_t model)
{
  return model->num_entries;
}

int
grub_gfxmenu_model_get_selected_index (grub_gfxmenu_model_t model)
{
  return model->selected_entry_index;
}

void
grub_gfxmenu_model_set_selected_index (grub_gfxmenu_model_t model, int index)
{
  model->selected_entry_index = index;
}

const char *
grub_gfxmenu_model_get_entry_title (grub_gfxmenu_model_t model, int index)
{
  if (index < 0 || index >= model->num_entries)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid menu index");
      return 0;
    }

  return model->entries[index]->title;
}

grub_menu_entry_t
grub_gfxmenu_model_get_entry (grub_gfxmenu_model_t model, int index)
{
  if (index < 0 || index >= model->num_entries)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid menu index");
      return 0;
    }

  return model->entries[index];
}
