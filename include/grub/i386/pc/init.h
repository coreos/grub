/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef PUPA_INIT_MACHINE_HEADER
#define PUPA_INIT_MACHINE_HEADER	1

#include <pupa/types.h>
#include <pupa/symbol.h>

/* FIXME: Should these be declared in memory.h?  */
extern pupa_size_t EXPORT_VAR(pupa_lower_mem);
extern pupa_size_t EXPORT_VAR(pupa_upper_mem);

/* Get the memory size in KB. If EXTENDED is zero, return conventional
   memory, otherwise return extended memory.  */
pupa_uint16_t pupa_get_memsize (int extended);

/* Get a packed EISA memory map. Lower 16 bits are between 1MB and 16MB
   in 1KB parts, and upper 16 bits are above 16MB in 64KB parts.  */
pupa_uint32_t pupa_get_eisa_mmap (void);

struct pupa_machine_mmap_entry
{
  pupa_uint32_t size;
  pupa_uint64_t addr;
  pupa_uint64_t len;
  pupa_uint32_t type;
};

/* Get a memory map entry. Return next continuation value. Zero means
   the end.  */
pupa_uint32_t pupa_get_mmap_entry (struct pupa_machine_mmap_entry *entry,
				   pupa_uint32_t cont);

/* Turn on/off Gate A20.  */
void pupa_gate_a20 (int on);

#endif /* ! PUPA_INIT_MACHINE_HEADER */
