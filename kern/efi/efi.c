/* efi.c - generic EFI support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/misc.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/console_control.h>
#include <grub/efi/pe32.h>
#include <grub/machine/time.h>
#include <grub/term.h>
#include <grub/kernel.h>

/* The handle of GRUB itself. Filled in by the startup code.  */
grub_efi_handle_t grub_efi_image_handle;

/* The pointer to a system table. Filled in by the startup code.  */
grub_efi_system_table_t *grub_efi_system_table;

static grub_efi_guid_t grub_efi_console_control_guid = GRUB_EFI_CONSOLE_CONTROL_GUID;
static grub_efi_guid_t grub_efi_loaded_image_guid = GRUB_EFI_LOADED_IMAGE_GUID;

void *
grub_efi_locate_protocol (grub_efi_guid_t *protocol, void *registration)
{
  void *interface;
  grub_efi_status_t status;
  
  status = grub_efi_system_table->boot_services->locate_protocol (protocol,
								  registration,
								  &interface);
  if (status != GRUB_EFI_SUCCESS)
    return 0;
  
  return interface;
}

int
grub_efi_set_text_mode (int on)
{
  grub_efi_console_control_protocol_t *c;
  grub_efi_screen_mode_t mode, new_mode;

  c = grub_efi_locate_protocol (&grub_efi_console_control_guid, 0);
  if (! c)
    return 0;
  
  if (c->get_mode (c, &mode, 0, 0) != GRUB_EFI_SUCCESS)
    return 0;

  new_mode = on ? GRUB_EFI_SCREEN_TEXT : GRUB_EFI_SCREEN_GRAPHICS;
  if (mode != new_mode)
    if (c->set_mode (c, new_mode) != GRUB_EFI_SUCCESS)
      return 0;

  return 1;
}

void
grub_efi_exit (void)
{
  grub_efi_system_table->boot_services->exit (grub_efi_image_handle,
					      GRUB_EFI_SUCCESS,
					      0, 0);
}

void
grub_efi_stall (grub_efi_uintn_t microseconds)
{
  grub_efi_system_table->boot_services->stall (microseconds);
}

grub_efi_loaded_image_t *
grub_efi_get_loaded_image (void)
{
  grub_efi_boot_services_t *b;
  void *interface;
  grub_efi_status_t status;
  
  b = grub_efi_system_table->boot_services;
  status = b->open_protocol (grub_efi_image_handle,
			     &grub_efi_loaded_image_guid,
			     &interface,
			     grub_efi_image_handle,
			     0,
			     GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (status != GRUB_EFI_SUCCESS)
    return 0;

  return interface;
}

void
grub_stop (void)
{
  grub_printf ("\nPress any key to abort.\n");
  grub_getkey ();
  
  grub_efi_fini ();
  grub_efi_exit ();
}

grub_uint32_t
grub_get_rtc (void)
{
  grub_efi_time_t time;
  grub_efi_runtime_services_t *r;

  r = grub_efi_system_table->runtime_services;
  if (r->get_time (&time, 0) != GRUB_EFI_SUCCESS)
    /* What is possible in this case?  */
    return 0;

  return (((time.minute * 60 + time.second) * 1000
	   + time.nanosecond / 1000000)
	  * GRUB_TICKS_PER_SECOND / 1000);
}

/* Search the mods section from the PE32/PE32+ image. This code uses
   a PE32 header, but should work with PE32+ as well.  */
grub_addr_t
grub_arch_modules_addr (void)
{
  grub_efi_loaded_image_t *image;
  struct grub_pe32_header *header;
  struct grub_pe32_coff_header *coff_header;
  struct grub_pe32_section_table *sections;
  struct grub_pe32_section_table *section;
  struct grub_module_info *info;
  grub_uint16_t i;
  
  image = grub_efi_get_loaded_image ();
  if (! image)
    return 0;

  header = image->image_base;
  coff_header = &(header->coff_header);
  sections
    = (struct grub_pe32_section_table *) ((char *) coff_header
					  + sizeof (*coff_header)
					  + coff_header->optional_header_size);

  for (i = 0, section = sections;
       i < coff_header->num_sections;
       i++, section++)
    {
      if (grub_strcmp (section->name, "mods") == 0)
	break;
    }

  if (i == coff_header->num_sections)
    return 0;

  info = (struct grub_module_info *) ((char *) image->image_base
				      + section->virtual_address);
  if (info->magic != GRUB_MODULE_MAGIC)
    return 0;

  return (grub_addr_t) info;
}
