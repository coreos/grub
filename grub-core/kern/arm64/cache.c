/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#include <grub/cache.h>
#include <grub/misc.h>

grub_int64_t grub_arch_cache_dlinesz;
grub_int64_t grub_arch_cache_ilinesz;

/* Prototypes for asm functions. */
void grub_arch_sync_caches_real (grub_uint64_t address, grub_uint64_t end);

static void
probe_caches (void)
{
  grub_uint32_t cache_type;

  /* Read Cache Type Register */
  asm volatile ("mrs 	%0, ctr_el0": "=r"(cache_type));

  grub_arch_cache_dlinesz = 8 << ((cache_type >> 16) & 0xf);
  grub_arch_cache_ilinesz = 8 << (cache_type & 0xf);

  grub_dprintf("cache", "D$ line size: %lld\n",
	       (long long) grub_arch_cache_dlinesz);
  grub_dprintf("cache", "I$ line size: %lld\n",
	       (long long) grub_arch_cache_ilinesz);
}

void
grub_arch_sync_caches (void *address, grub_size_t len)
{
  grub_uint64_t start, end;

  if (grub_arch_cache_dlinesz == 0)
    probe_caches();
  if (grub_arch_cache_dlinesz == 0)
    grub_fatal ("Unknown cache line size!");

  grub_dprintf("cache", "syncing caches for %p-%lx\n",
	       address, (grub_addr_t) address + len);

  /* Align here to both cache lines. Saves a tiny bit of asm complexity and
     most of potential problems with different line sizes.  */
  start = (grub_uint64_t) address;
  end = (grub_uint64_t) address + len;
  start = ALIGN_DOWN (start, grub_arch_cache_dlinesz);
  start = ALIGN_DOWN (start, grub_arch_cache_ilinesz);

  end = ALIGN_UP (end, grub_arch_cache_dlinesz);
  end = ALIGN_UP (end, grub_arch_cache_ilinesz);

  grub_dprintf("cache", "aligned to: %lx-%lx\n",
	       start, end);

  grub_arch_sync_caches_real (start, end);
}
