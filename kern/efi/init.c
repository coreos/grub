/* init.c - generic EFI initialization and finalization */
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

#include <grub/efi/efi.h>
#include <grub/efi/console.h>
#include <grub/efi/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/machine/kernel.h>

void
grub_efi_init (void)
{
  /* First of all, initialize the console so that GRUB can display
     messages.  */
  grub_console_init ();

  /* Initialize the memory management system.  */
  grub_efi_mm_init ();

  grub_efidisk_init ();

  /* FIXME: this must be set to something meaningful.  */
  grub_env_set ("prefix", grub_prefix);
}

void
grub_efi_fini (void)
{
  grub_efidisk_fini ();
  grub_efi_mm_fini ();
  grub_console_fini ();
}
