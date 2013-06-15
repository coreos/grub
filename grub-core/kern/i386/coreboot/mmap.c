/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2013  Free Software Foundation, Inc.
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
#include <grub/machine/lbio.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/misc.h>

/* Context for grub_machine_mmap_iterate.  */
struct grub_machine_mmap_iterate_ctx
{
  grub_memory_hook_t hook;
  void *hook_data;
};

#define GRUB_MACHINE_MEMORY_BADRAM 	5

/* Helper for grub_machine_mmap_iterate.  */
static int
iterate_linuxbios_table (grub_linuxbios_table_item_t table_item, void *data)
{
  struct grub_machine_mmap_iterate_ctx *ctx = data;
  mem_region_t mem_region;

  if (table_item->tag != GRUB_LINUXBIOS_MEMBER_MEMORY)
    return 0;

  mem_region =
    (mem_region_t) ((long) table_item +
			       sizeof (struct grub_linuxbios_table_item));
  while ((long) mem_region < (long) table_item + (long) table_item->size)
    {
      if (ctx->hook (mem_region->addr, mem_region->size,
		     /* Multiboot mmaps match with the coreboot mmap
		        definition.  Therefore, we can just pass type
		        through.  */
		     (((mem_region->type <= GRUB_MACHINE_MEMORY_BADRAM) && (mem_region->type >= GRUB_MACHINE_MEMORY_AVAILABLE))
		      || mem_region->type == GRUB_MEMORY_COREBOOT_TABLES) ? mem_region->type : GRUB_MEMORY_RESERVED,
		     ctx->hook_data))
	return 1;

      mem_region++;
    }

  return 0;
}

grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook, void *hook_data)
{
  struct grub_machine_mmap_iterate_ctx ctx = { hook, hook_data };

  grub_linuxbios_table_iterate (iterate_linuxbios_table, &ctx);

  return 0;
}
