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

/* The handle of GRUB itself. Filled in by the startup code.  */
grub_efi_handle_t grub_efi_image_handle;

/* The pointer to a system table. Filled in by the startup code.  */
grub_efi_system_table_t *grub_efi_system_table;

static grub_efi_guid_t grub_efi_console_control_guid = GRUB_EFI_CONSOLE_CONTROL_GUID;

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

int
grub_efi_output_string (const char *str)
{
  grub_efi_simple_text_output_interface_t *o;
  grub_size_t len = grub_strlen (str);
  grub_efi_char16_t utf16_str[len + 1];
  grub_efi_status_t status;

  /* XXX Assume that STR is all ASCII characters.  */
  do
    {
      utf16_str[len] = str[len];
    }
  while (len--);
  
  o = grub_efi_system_table->con_out;
  status = o->output_string (o, utf16_str);
  return status >= 0;
}

