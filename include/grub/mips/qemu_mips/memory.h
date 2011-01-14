/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GRUB_MEMORY_MACHINE_HEADER
#define GRUB_MEMORY_MACHINE_HEADER	1

#ifndef ASM_FILE
#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>
#endif

#define GRUB_MACHINE_MEMORY_STACK_HIGH       0x80f00000
#define GRUB_MACHINE_MEMORY_USABLE       0x81000000

#define GRUB_MACHINE_MEMORY_AVAILABLE        1
#define GRUB_MACHINE_MEMORY_MAX_TYPE         1
  /* This one is special: it's used internally but is never reported
     by firmware. */
#define GRUB_MACHINE_MEMORY_HOLE 	2
#define GRUB_MACHINE_MEMORY_RESERVED GRUB_MACHINE_MEMORY_HOLE

#ifndef ASM_FILE

typedef grub_addr_t grub_phys_addr_t;

static inline grub_phys_addr_t
grub_vtop (void *a)
{
  return ((grub_phys_addr_t) a) & 0x1fffffff;
}

static inline void *
grub_map_memory (grub_phys_addr_t a, grub_size_t size __attribute__ ((unused)))
{
  return (void *) (a | 0x80000000);
}

static inline void
grub_unmap_memory (void *a __attribute__ ((unused)),
		   grub_size_t size __attribute__ ((unused)))
{
}

grub_err_t EXPORT_FUNC (grub_machine_mmap_iterate)
(int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, grub_uint64_t, grub_uint32_t));

static inline grub_err_t
grub_machine_mmap_register (grub_uint64_t start __attribute__ ((unused)),
			    grub_uint64_t size __attribute__ ((unused)),
			    int type __attribute__ ((unused)),
			    int handle __attribute__ ((unused)))
{
  return GRUB_ERR_NONE;
}
static inline grub_err_t
grub_machine_mmap_unregister (int handle  __attribute__ ((unused)))
{
  return GRUB_ERR_NONE;
}

#endif

#endif
