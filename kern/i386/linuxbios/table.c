/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/types.h>
#include <grub/err.h>

static grub_err_t
grub_linuxbios_table_iterate (grub_linuxbios_table_header_t table_header,
		      int (*hook) (grub_linuxbios_table_item_t))
{
  grub_linuxbios_table_item_t table_item;

  if (grub_memcmp (table_header->signature, "LBIO", 4))
    grub_fatal ("Could not find LinuxBIOS table\n");

  table_item =
    (grub_linuxbios_table_item_t) ((long) table_header +
			       (long) table_header->size);
  for (; table_item->size;
       table_item = (grub_linuxbios_table_item_t) ((long) table_item + (long) table_item->size))
    if (hook (table_item))
      return 1;

  return 0;
}

grub_err_t
grub_available_iterate (int (*hook) (mem_region_t))
{
  mem_region_t mem_region;

  auto int iterate_linuxbios_table (grub_linuxbios_table_item_t);
  int iterate_linuxbios_table (grub_linuxbios_table_item_t table_item)
  {
    if (table_item->tag != GRUB_LINUXBIOS_MEMBER_MEMORY)
      return 0;

    mem_region =
      (mem_region_t) ((long) table_item +
				 sizeof (struct grub_linuxbios_table_item));
    for (; (long) mem_region < (long) table_item + (long) table_item->size;
	 mem_region++)
      if (hook (mem_region))
	return 1;

    return 0;
  }

  grub_linuxbios_table_iterate (GRUB_MEMORY_MACHINE_LINUXBIOS_TABLE_ADDR,
			iterate_linuxbios_table);

  return 0;
}
