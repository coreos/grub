/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <grub/emu/misc.h>
#include <grub/util/install.h>
#include <grub/util/misc.h>

#include "platform.c"

void
grub_install_register_ieee1275 (int is_prep, const char *install_device,
				int partno, const char *relpath)
{
  grub_util_error ("%s", "no IEEE1275 routines are available for your platform");
}

void
grub_install_register_efi (const char *efidir_disk, int efidir_part,
			   const char *efifile_path,
			   const char *efi_distributor)
{
  grub_util_error ("%s", "no EFI routines are available for your platform");
}

void
grub_install_sgi_setup (const char *install_device,
			const char *imgfile, const char *destname)
{
  grub_util_error ("%s", "no SGI routines are available for your platform");
}
