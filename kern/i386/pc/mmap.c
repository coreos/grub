/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/init.h>
#include <grub/machine/int.h>
#include <grub/machine/memory.h>
#include <grub/err.h>
#include <grub/types.h>

/*
 * grub_get_ext_memsize() :  return the extended memory size in KB.
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *	The return value in AX.
 *
 */
static inline grub_uint16_t
grub_get_ext_memsize (void)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x8800;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);
  return regs.eax & 0xffff;
}

/* Get a packed EISA memory map. Lower 16 bits are between 1MB and 16MB
   in 1KB parts, and upper 16 bits are above 16MB in 64KB parts. If error, return zero.
   BIOS call "INT 15H, AH=E801H" to get EISA memory map,
     AX = memory between 1M and 16M in 1K parts.
     BX = memory above 16M in 64K parts. 
*/
 
static inline grub_uint32_t
grub_get_eisa_mmap (void)
{
  struct grub_bios_int_registers regs;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xe801;
  grub_bios_interrupt (0x15, &regs);

  if ((regs.eax & 0xff00) == 0x8600)
    return 0;

  return (regs.eax & 0xffff) | (regs.ebx << 16);
}

grub_err_t
grub_machine_mmap_iterate (int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, grub_uint64_t, grub_uint32_t))
{
  grub_uint32_t cont;
  struct grub_machine_mmap_entry *entry
    = (struct grub_machine_mmap_entry *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

  /* Check if grub_get_mmap_entry works.  */
  cont = grub_get_mmap_entry (entry, 0);

  if (entry->size)
    do
      {
	if (hook (entry->addr, entry->len,
		  /* Multiboot mmaps have been defined to match with the E820 definition.
		     Therefore, we can just pass type through.  */
		  entry->type))
	  break;

	if (! cont)
	  break;

	cont = grub_get_mmap_entry (entry, cont);
      }
    while (entry->size);
  else
    {
      grub_uint32_t eisa_mmap = grub_get_eisa_mmap ();

      if (eisa_mmap)
	{
	  if (hook (0x100000, (eisa_mmap & 0xFFFF) << 10, GRUB_MACHINE_MEMORY_AVAILABLE) == 0)
	    hook (0x1000000, eisa_mmap & ~0xFFFF, GRUB_MACHINE_MEMORY_AVAILABLE);
	}
      else
	hook (0x100000, grub_get_ext_memsize () << 10, GRUB_MACHINE_MEMORY_AVAILABLE);
    }

  return 0;
}
