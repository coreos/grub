/* init.c - initialize an ia64-based EFI system */
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
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/cache.h>
#include <grub/kernel.h>
#include <grub/efi/efi.h>

void
grub_machine_init (void)
{
  grub_efi_init ();
  grub_install_get_time_ms (grub_rtc_get_time_ms);
}

void
grub_machine_fini (void)
{
  grub_efi_fini ();
}

void
grub_machine_set_prefix (void)
{
  grub_efi_set_prefix ();
}

void
grub_arch_sync_caches (void *address, grub_size_t len)
{
  /* Cache line length is at least 32.  */
  grub_uint64_t a = (grub_uint64_t)address & ~0x1f;

  /* Flush data.  */
  for (len = (len + 31) & ~0x1f; len > 0; len -= 0x20, a += 0x20)
    asm volatile ("fc.i %0" : : "r" (a));
  /* Sync and serialize.  Maybe extra.  */
  asm volatile (";; sync.i;; srlz.i;;");
}
