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
#include <pupa/machine/kernel.h>
#include <pupa/types.h>
#include <pupa/err.h>
#include <pupa/dl.h>
#include <pupa/misc.h>
#include <pupa/loader.h>

struct mem_region
{
  pupa_addr_t addr;
  pupa_size_t size;
};

#define MAX_REGIONS	32

static struct mem_region mem_regions[MAX_REGIONS];
static int num_regions;

pupa_addr_t pupa_os_area_addr;
pupa_size_t pupa_os_area_size;
pupa_size_t pupa_lower_mem, pupa_upper_mem;

static char *
make_install_device (void)
{
  /* XXX: This should be enough.  */
  char dev[100];

  pupa_sprintf (dev, "(%cd%u",
		(pupa_boot_drive & 0x80) ? 'h' : 'f',
		pupa_boot_drive & 0x7f);
  
  if (pupa_install_dos_part >= 0)
    pupa_sprintf (dev + pupa_strlen (dev), ",%u", pupa_install_dos_part);

  if (pupa_install_bsd_part >= 0)
    pupa_sprintf (dev + pupa_strlen (dev), ",%c", pupa_install_bsd_part + 'a');

  pupa_sprintf (dev + pupa_strlen (dev), ")%s", pupa_prefix);
  pupa_strcpy (pupa_prefix, dev);
  
  return pupa_prefix;
}

/* Add a memory region.  */
static void
add_mem_region (pupa_addr_t addr, pupa_size_t size)
{
  if (num_regions == MAX_REGIONS)
    /* Ignore.  */
    return;

  mem_regions[num_regions].addr = addr;
  mem_regions[num_regions].size = size;
  num_regions++;
}

/* Compact memory regions.  */
static void
compact_mem_regions (void)
{
  int i, j;

  /* Sort them.  */
  for (i = 0; i < num_regions - 1; i++)
    for (j = i + 1; j < num_regions; j++)
      if (mem_regions[i].addr > mem_regions[j].addr)
	{
	  struct mem_region tmp = mem_regions[i];
	  mem_regions[i] = mem_regions[j];
	  mem_regions[j] = tmp;
	}

  /* Merge overlaps.  */
  for (i = 0; i < num_regions - 1; i++)
    if (mem_regions[i].addr + mem_regions[i].size >= mem_regions[i + 1].addr)
      {
	j = i + 1;
	
	if (mem_regions[i].addr + mem_regions[i].size
	    < mem_regions[j].addr + mem_regions[j].size)
	  mem_regions[i].size = (mem_regions[j].addr + mem_regions[j].size
				 - mem_regions[i].addr);

	pupa_memmove (mem_regions + j, mem_regions + j + 1,
		      (num_regions - j - 1) * sizeof (struct mem_region));
	i--;
      }
}

void
pupa_machine_init (void)
{
  pupa_uint32_t cont;
  struct pupa_machine_mmap_entry *entry
    = (struct pupa_machine_mmap_entry *) PUPA_MEMORY_MACHINE_SCRATCH_ADDR;
  pupa_addr_t end_addr = pupa_get_end_addr ();
  int i;
  
  /* Initialize the console as early as possible.  */
  pupa_console_init ();
  
  pupa_lower_mem = pupa_get_memsize (0) << 10;
  
  /* Sanity check.  */
  if (pupa_lower_mem < PUPA_MEMORY_MACHINE_RESERVED_END)
    pupa_fatal ("too small memory");

#if 0
  /* Turn on Gate A20 to access >1MB.  */
  pupa_gate_a20 (1);
#endif

  /* Add the lower memory into free memory.  */
  if (pupa_lower_mem >= PUPA_MEMORY_MACHINE_RESERVED_END)
    add_mem_region (PUPA_MEMORY_MACHINE_RESERVED_END,
		    pupa_lower_mem - PUPA_MEMORY_MACHINE_RESERVED_END);
  
  add_mem_region (end_addr, PUPA_MEMORY_MACHINE_RESERVED_START - end_addr);

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
	    add_mem_region (addr, len);
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
	    add_mem_region (0x100000, (eisa_mmap << 16) + 0x100000 * 15);
	  else
	    {
	      add_mem_region (0x100000, (eisa_mmap & 0xFFFF) << 10);
	      add_mem_region (0x1000000, eisa_mmap << 16);
	    }
	}
      else
	add_mem_region (0x100000, pupa_get_memsize (1) << 10);
    }

  compact_mem_regions ();

  /* Add the memory regions to free memory, except for the region starting
     from 1MB. This region is partially used for loading OS images.
     For now, 1/4 of this is added to free memory.  */
  for (i = 0; i < num_regions; i++)
    if (mem_regions[i].addr == 0x100000)
      {
	pupa_size_t quarter = mem_regions[i].size >> 2;

	pupa_upper_mem = mem_regions[i].size;
	pupa_os_area_addr = mem_regions[i].addr;
	pupa_os_area_size = mem_regions[i].size - quarter;
	pupa_mm_init_region ((void *) (pupa_os_area_addr + pupa_os_area_size),
			     quarter);
      }
    else
      pupa_mm_init_region ((void *) mem_regions[i].addr, mem_regions[i].size);
  
  if (! pupa_os_area_addr)
    pupa_fatal ("no upper memory");
  
  /* The memory system was initialized, thus register built-in devices.  */
  pupa_biosdisk_init ();

  /* Initialize the prefix.  */
  pupa_dl_set_prefix (make_install_device ());
}
