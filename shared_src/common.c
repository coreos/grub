/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
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

#define _COMMON_C

#include "shared.h"


/*
 *  Shared BIOS/boot data.
 */

struct multiboot_info mbi;
unsigned long saved_drive;
unsigned long saved_partition;
unsigned long saved_mem_upper;
int mem_map = 0;

/*
 *  Error code stuff.
 */

int errnum = 0;

#ifndef NO_FANCY_STUFF

char *err_list[] =
{
  0,
  "Selected item won\'t fit into memory",
  "Selected disk doesn\'t exist",
  "Disk read error",
  "Disk write error",
  "Disk geometry error",
  "Attempt to access block outside partition",
  "Partition table invalid or corrupt",
  "No such partition",
  "Bad filename (must be absolute pathname or blocklist)",
  "Bad file or directory type",
  "File not found",
  "Cannot mount selected partition",
  "Inconsistent filesystem structure",
  "Filesystem compatibility error, can\'t read whole file",
  "Error while parsing number",
  "Device string unrecognizable",
  "Invalid device requested",
  "Invalid or unsupported executable format",
  "Loading below 1MB is not supported",
  "Unsupported Multiboot features requested",
  "Unknown boot failure",
  "Must load Multiboot kernel before modules",
  "Unrecognized command",
  "Bad or incompatible header on compressed file",
  "Bad or corrupt data while decompressing file",
  "Bad or corrupt version of stage1/stage2",
  0
};


/* static for BIOS memory map fakery */
static struct AddrRangeDesc fakemap[3] =
{
  { 20, 0, 0, 0, 0, MB_ARD_MEMORY },
  { 20, 0x100000, 0, 0, 0, MB_ARD_MEMORY },
  { 20, 0x1000000, 0, 0, 0, MB_ARD_MEMORY }
};
#endif  /* NO_FANCY_STUFF */


/*
 *  This queries for BIOS information.
 */

void
init_bios_info(void)
{
  /*
   *  Get information from BIOS on installed RAM.
   */

  mbi.mem_lower = get_memsize(0);
  mbi.mem_upper = get_memsize(1);

#ifndef NO_FANCY_STUFF
  /*
   *  We need to call this somewhere before trying to put data
   *  above 1 MB, since without calling it, address line 20 will be wired
   *  to 0.  Not too desirable.
   */

  gateA20(1);

  /*
   *  The "mem_upper" variable only recognizes upper memory in the
   *  first memory region.  If there are multiple memory regions,
   *  the rest are reported to a Multiboot-compliant OS, but otherwise
   *  unused by GRUB.
   */

  {
    int cont = 0, mem1, mem2, addr;

    mbi.mmap_addr = (addr = (((int) end) & ~3) + 4);
    mbi.mmap_length = 0;

    do
      {
	cont = get_mem_map(addr, cont);

	if ( ! *((int *)addr) )
	  break;

	/*
	 *  This is to get the upper memory up to the first memory
	 *  hole into the "mbi.mem_upper" element, for OS's that
	 *  don't care about the memory map, but might care about
	 *  RAM above 64MB.
	 */
	if (((struct AddrRangeDesc *)addr)->BaseAddrLow == 0x100000
	    && ((struct AddrRangeDesc *)addr)->BaseAddrHigh == 0
	    && ((struct AddrRangeDesc *)addr)->Type == MB_ARD_MEMORY)
	  {
	    /* limit to 4G, as most OS's would probably break with more */

	    if (!((struct AddrRangeDesc *)addr)->LengthHigh)
	      mbi.mem_upper = ((struct AddrRangeDesc *)addr)->LengthLow >> 10;
	    else
	      mbi.mem_upper = 0x3FBFC0;  /* 4G - 1M - 64K */
	  }

	mbi.mmap_length += *((int *)addr) + 4;
	addr += *((int *)addr) + 4;

	mem_map++;
      }
    while (cont);

    if (!mem_map && (mem1 = get_eisamemsize()) != -1)
      {
	mem2 = mem1 >> 16;
	mem1 &= 0xFFFF;
	mbi.mem_upper = mem1;

	if (!mem2 || (mem1 == 0x3c00))
	  mbi.mem_upper += (mem2 << 6);
	else
	  {
	    /* XXX should I do this at all ??? */

	    mbi.mmap_addr = (int)fakemap;
	    mbi.mmap_length = sizeof(fakemap);
	    fakemap[0].LengthLow = (mbi.mem_lower << 10);
	    fakemap[1].LengthLow = (mem1 << 10);
	    fakemap[2].LengthLow = (mem2 << 16);
	    mem_map++;
	  }
      }
  }

  saved_mem_upper = mbi.mem_upper;

  /*
   *  Initialize other Multiboot Info flags.
   */

  mbi.flags = MB_INFO_MEMORY | MB_INFO_CMDLINE | MB_INFO_BOOTDEV;

#endif  /* NO_FANCY_STUFF */

  /*
   *  Set boot drive and partition.
   */

  saved_drive = boot_drive;
  saved_partition = install_partition;

  /*
   *  Start main routine here.
   */

  cmain();
}

