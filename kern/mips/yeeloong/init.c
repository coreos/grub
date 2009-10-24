/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/cpu/kernel.h>

#define RAMSIZE (64 << 20)

grub_uint32_t
grub_get_rtc (void)
{
  static grub_uint64_t calln = 0;

  return (calln++) >> 8;
}

void
grub_machine_init (void)
{
  grub_mm_init_region ((void *) GRUB_MACHINE_MEMORY_USABLE,
		       RAMSIZE - (GRUB_MACHINE_MEMORY_USABLE & 0x7fffffff));
  grub_install_get_time_ms (grub_rtc_get_time_ms);
}

void
grub_machine_fini (void)
{
}

void
grub_exit (void)
{
  while (1);
}

void
grub_halt (void)
{
  while (1);
}

void
grub_reboot (void)
{
  while (1);
}

grub_err_t 
grub_machine_mmap_iterate (int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, 
							 grub_uint64_t, 
							 grub_uint32_t))
{
  hook (0, RAMSIZE, GRUB_MACHINE_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}
