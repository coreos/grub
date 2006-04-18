/* init.c - initialize an x86-based EFI system */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/cache.h>
#include <grub/kernel.h>
#include <grub/efi/efi.h>
#include <grub/machine/time.h>

void
grub_stop (void)
{
  grub_efi_exit ();
}

grub_uint32_t
grub_get_rtc (void)
{
  return 0; /* FIXME */
}

void
grub_machine_init (void)
{
  grub_efi_set_text_mode (1);
  grub_efi_output_string ("test!\r\n");

  /* Stop immediately at the moment...  */
  grub_stop ();
}

void
grub_machine_fini (void)
{
}

void
grub_arch_sync_caches (void *address __attribute__ ((unused)),
                       grub_size_t len __attribute__ ((unused)))
{
}

grub_addr_t
grub_arch_modules_addr (void)
{
  return 0;
}
