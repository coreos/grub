/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_KEYBOARD_LAYOUTS_H
#define GRUB_KEYBOARD_LAYOUTS_H 1

#define GRUB_KEYBOARD_LAYOUTS_FILEMAGIC "GRUBLAYO"
#define GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE (sizeof(GRUB_KEYBOARD_LAYOUTS_FILEMAGIC) - 1)
#define GRUB_KEYBOARD_LAYOUTS_VERSION 8

#define GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE 128

struct grub_keyboard_layout
{
  grub_uint32_t keyboard_map[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  grub_uint32_t keyboard_map_shift[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  grub_uint32_t keyboard_map_l3[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  grub_uint32_t keyboard_map_shift_l3[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
};

unsigned EXPORT_FUNC(grub_term_map_key) (int code, int status);

#endif /* GRUB_KEYBOARD_LAYOUTS  */
