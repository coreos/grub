/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/kernel.h>
#include <pupa/mm.h>
#include <pupa/machine/init.h>
#include <pupa/machine/memory.h>
#include <pupa/machine/console.h>
#include <pupa/machine/biosdisk.h>
#include <pupa/types.h>
#include <pupa/err.h>

void
pupa_machine_init (void)
{
  pupa_uint32_t cont;
  struct pupa_machine_mmap_entry *entry
    = (struct pupa_machine_mmap_entry *) PUPA_MEMORY_MACHINE_SCRATCH_ADDR;
  pupa_size_t lower_mem = (pupa_get_memsize (0) << 10);
  pupa_addr_t end_addr = pupa_get_end_addr ();

  /* Initialize the console as early as possible.  */
  pupa_console_init ();
  
  /* Sanity check.  */
  if (lower_mem < PUPA_MEMORY_MACHINE_RESERVED_END)
    pupa_fatal ("too small memory");
  
  /* Turn on Gate A20 to access >1MB.  */
  pupa_gate_a20 (1);

  /* Add the lower memory into free memory.  */
  if (lower_mem >= PUPA_MEMORY_MACHINE_RESERVED_END)
    pupa_mm_init_region ((void *) PUPA_MEMORY_MACHINE_RESERVED_END,
			 lower_mem - PUPA_MEMORY_MACHINE_RESERVED_END);
  
  pupa_mm_init_region ((void *) end_addr,
		       PUPA_MEMORY_MACHINE_RESERVED_START - end_addr);

  /* Check if pupa_get_mmap_entry works.  */
  cont = pupa_get_mmap_entry (entry, 0);

  if (entry->size)
    do
      {
	/* Avoid the lower memory.  */
	if (entry->addr < 0x100000)
	  {
	    if (entry->len <= 0x100000 - entry->addr)
	      goto next;

	    entry->len -= 0x100000 - entry->addr;
	    entry->addr = 0x100000;
	  }
	
	/* Ignore >4GB.  */
	if (entry->addr <= 0xFFFFFFFF && entry->type == 1)
	  {
	    pupa_addr_t addr;
	    pupa_size_t len;

	    addr = (pupa_addr_t) entry->addr;
	    len = ((addr + entry->len > 0xFFFFFFFF)
		   ? 0xFFFFFFFF - addr
		   : (pupa_size_t) entry->len);
	    pupa_mm_init_region ((void *) addr, len);
	  }
	
      next:
	if (! cont)
	  break;
	
	cont = pupa_get_mmap_entry (entry, cont);
      }
    while (entry->size);
  else
    {
      pupa_uint32_t eisa_mmap = pupa_get_eisa_mmap ();

      if (eisa_mmap)
	{
	  if ((eisa_mmap & 0xFFFF) == 0x3C00)
	    pupa_mm_init_region ((void *) 0x100000,
				 (eisa_mmap << 16) + 0x100000 * 15);
	  else
	    {
	      pupa_mm_init_region ((void *) 0x100000,
				   (eisa_mmap & 0xFFFF) << 10);
	      pupa_mm_init_region ((void *) 0x1000000, eisa_mmap << 16);
	    }
	}
      else
	pupa_mm_init_region ((void *) 0x100000,
			     (pupa_size_t) pupa_get_memsize (1) << 10);
    }

  /* The memory system was initialized, thus register built-in devices.  */
  pupa_biosdisk_init ();
}
