/* gfxmenu_model.h - gfxmenu model interface. */
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

#ifndef GRUB_GFXMENU_MODEL_HEADER
#define GRUB_GFXMENU_MODEL_HEADER 1

#include <grub/menu.h>

struct grub_gfxmenu_model;   /* Forward declaration of opaque type.  */
typedef struct grub_gfxmenu_model *grub_gfxmenu_model_t;


grub_gfxmenu_model_t grub_gfxmenu_model_new (grub_menu_t menu);

void grub_gfxmenu_model_destroy (grub_gfxmenu_model_t model);

grub_menu_t grub_gfxmenu_model_get_menu (grub_gfxmenu_model_t model);

void grub_gfxmenu_model_set_timeout (grub_gfxmenu_model_t model);

void grub_gfxmenu_model_clear_timeout (grub_gfxmenu_model_t model);

int grub_gfxmenu_model_get_timeout_ms (grub_gfxmenu_model_t model);

int grub_gfxmenu_model_get_timeout_remaining_ms (grub_gfxmenu_model_t model);

int grub_gfxmenu_model_timeout_expired (grub_gfxmenu_model_t model);

int grub_gfxmenu_model_get_num_entries (grub_gfxmenu_model_t model);

int grub_gfxmenu_model_get_selected_index (grub_gfxmenu_model_t model);

void grub_gfxmenu_model_set_selected_index (grub_gfxmenu_model_t model,
                                            int index);

const char *grub_gfxmenu_model_get_entry_title (grub_gfxmenu_model_t model,
                                                int index);

grub_menu_entry_t grub_gfxmenu_model_get_entry (grub_gfxmenu_model_t model,
                                                int index);

#endif /* GRUB_GFXMENU_MODEL_HEADER */

