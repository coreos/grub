/* init.c - initialize an arm-based EFI system */
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

#include <grub/env.h>
#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/efi/efi.h>

/*
 * A bit ugly, but functional - and should be completely portable.
 */
static grub_uint64_t
grub_efi_get_time_ms(void)
{
  grub_efi_time_t now;
  grub_uint64_t retval;
  grub_efi_status_t status;

  status = efi_call_2 (grub_efi_system_table->runtime_services->get_time,
		       &now, NULL);
  if (status != GRUB_EFI_SUCCESS)
    {
      grub_printf("No time!\n");
      return 0;
    }
  retval = now.year * 365 * 24 * 60 * 60 * 1000;
  retval += now.month * 30 * 24 * 60 * 60 * 1000;
  retval += now.day * 24 * 60 * 60 * 1000;
  retval += now.hour * 60 * 60 * 1000;
  retval += now.minute * 60 * 1000;
  retval += now.second * 1000;
  retval += now.nanosecond / 1000;
 
  grub_dprintf("timer", "timestamp: 0x%llx\n", retval);

  return retval;
}

void
grub_machine_init (void)
{
  grub_efi_init ();
  grub_install_get_time_ms (grub_efi_get_time_ms);
}

void
grub_machine_fini (void)
{
  grub_efi_fini ();
}
