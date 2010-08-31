/* memmap.c  - Display memory map.  */
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
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

#define ADD_MEMORY_DESCRIPTOR(desc, size)	\
  ((grub_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

static grub_err_t
grub_cmd_memmap (struct grub_arg_list *state, int argc, char **args)
{
  grub_efi_uintn_t map_size;
  grub_efi_memory_descriptor_t *memory_map;
  grub_efi_memory_descriptor_t *memory_map_end;
  grub_efi_memory_descriptor_t *desc;
  grub_efi_uintn_t desc_size;

  map_size = 0;
  if (grub_efi_get_memory_map (&map_size, NULL, NULL, &desc_size, 0) < 0)
    return 0;

  memory_map = grub_malloc (map_size);
  if (memory_map == NULL)
    return 0;
  if (grub_efi_get_memory_map (&map_size, memory_map, NULL, &desc_size, 0) < 0)
    goto fail;

  grub_set_more (1);

  grub_printf
    ("Type      Physical start  - end             #Pages   "
     "  Size Attributes\n");
  memory_map_end = ADD_MEMORY_DESCRIPTOR(memory_map, map_size);
  for (desc = memory_map;
       desc < memory_map_end;
       desc = ADD_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      grub_efi_uintn_t size;
      grub_efi_uint64_t attr;
      static const char types_str[][9] = 
	{
	  "reserved", 
	  "ldr-code",
	  "ldr-data",
	  "BS-code ",
	  "BS-data ",
	  "RT-code ",
	  "RT-data ",
	  "conv-mem",
	  "unusable",
	  "ACPI-rec",
	  "ACPI-nvs",
	  "MMIO    ",
	  "IO-ports",
	  "PAL-code"
	};
      if (desc->type < sizeof (types_str) / sizeof (types_str[0]))
	grub_printf ("%s ", types_str[desc->type]);
      else
	grub_printf ("Unk %02x   ", desc->type);
      
      grub_printf (" %016llx-%016llx %08lx",
		   desc->physical_start,
		   desc->physical_start + (desc->num_pages << 12) - 1,
		   desc->num_pages);

      size = desc->num_pages << (12 - 10);
      if (size < 1024)
	grub_printf (" %4uKB", size);
      else
	{
	  size /= 1024;
	  if (size < 1024)
	    grub_printf (" %4uMB", size);
	  else
	    {
	      size /= 1024;
	      grub_printf (" %4uGB", size);
	    }
	}

      attr = desc->attribute;
      if (attr & GRUB_EFI_MEMORY_RUNTIME)
	grub_printf (" RT");
      if (attr & GRUB_EFI_MEMORY_UC)
	grub_printf (" UC");
      if (attr & GRUB_EFI_MEMORY_WC)
	grub_printf (" WC");
      if (attr & GRUB_EFI_MEMORY_WT)
	grub_printf (" WT");
      if (attr & GRUB_EFI_MEMORY_WB)
	grub_printf (" WB");
      if (attr & GRUB_EFI_MEMORY_UCE)
	grub_printf (" UCE");
      if (attr & GRUB_EFI_MEMORY_WP)
	grub_printf (" WP");
      if (attr & GRUB_EFI_MEMORY_RP)
	grub_printf (" RP");
      if (attr & GRUB_EFI_MEMORY_XP)
	grub_printf (" XP");

      grub_printf ("\n");
    }

  grub_set_more (0);

 fail:
  grub_free (memory_map);
  return 0;
}

GRUB_MOD_INIT(memmap)
{
  (void)mod;			/* To stop warning. */
  grub_register_extcmd ("memmap", grub_cmd_memmap, GRUB_COMMAND_FLAG_BOTH,
			"memmap",
			"Display memory map.", NULL);
}

GRUB_MOD_FINI(memmap)
{
  grub_unregister_extcmd ("memmap");
}
