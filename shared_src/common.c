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

/*
 *  Error code stuff.
 */

int errnum = 0;

#ifndef STAGE1_5

char *err_list[] =
{
  [ERR_NONE] = 0,
  [ERR_BAD_FILENAME] =
  "Bad filename (must be absolute pathname or blocklist)",
  [ERR_BAD_FILETYPE] = "Bad file or directory type",
  [ERR_BAD_GZIP_DATA] = "Bad or corrupt data while decompressing file",
  [ERR_BAD_GZIP_HEADER] = "Bad or incompatible header on compressed file",
  [ERR_BAD_PART_TABLE] = "Partition table invalid or corrupt",
  [ERR_BAD_VERSION] = "Bad or corrupt version of stage1/stage2",
  [ERR_BELOW_1MB] = "Loading below 1MB is not supported",
  [ERR_BOOT_COMMAND] = "Cannot boot without kernel loaded",
  [ERR_BOOT_FAILURE] = "Unknown boot failure",
  [ERR_BOOT_FEATURES] = "Unsupported Multiboot features requested",
  [ERR_DEV_FORMAT] = "Device string unrecognizable",
  [ERR_DEV_VALUES] = "Invalid device requested",
  [ERR_EXEC_FORMAT] = "Invalid or unsupported executable format",
  [ERR_FILELENGTH] =
  "Filesystem compatibility error, cannot read whole file",
  [ERR_FILE_NOT_FOUND] = "File not found",
  [ERR_FSYS_CORRUPT] = "Inconsistent filesystem structure",
  [ERR_FSYS_MOUNT] = "Cannot mount selected partition",
  [ERR_GEOM] = "Selected cylinder exceeds maximum supported by BIOS",
  [ERR_NEED_LX_KERNEL] = "Must load Linux kernel before initrd",
  [ERR_NEED_MB_KERNEL] = "Must load Multiboot kernel before modules",
  [ERR_NO_DISK] = "Selected disk does not exist",
  [ERR_NO_PART] = "No such partition",
  [ERR_NUMBER_PARSING] = "Error while parsing number",
  [ERR_OUTSIDE_PART] = "Attempt to access block outside partition",
  [ERR_READ] = "Disk read error",
  [ERR_SYMLINK_LOOP] = "Too many symbolic links",
  [ERR_UNRECOGNIZED] = "Unrecognized command",
  [ERR_WONT_FIT] = "Selected item cannot fit into memory",
  [ERR_WRITE] = "Disk write error",
};


/* static for BIOS memory map fakery */
static struct AddrRangeDesc fakemap[3] =
{
  {20, 0, 0, 0, 0, MB_ARD_MEMORY},
  {20, 0x100000, 0, 0, 0, MB_ARD_MEMORY},
  {20, 0x1000000, 0, 0, 0, MB_ARD_MEMORY}
};
#endif /* STAGE1_5 */


/*
 *  This queries for BIOS information.
 */

void
init_bios_info (void)
{
  int cont, memtmp, addr;

  /*
   *  Get information from BIOS on installed RAM.
   */

  mbi.mem_lower = get_memsize (0);
  mbi.mem_upper = get_memsize (1);

#ifndef STAGE1_5
  /*
   *  We need to call this somewhere before trying to put data
   *  above 1 MB, since without calling it, address line 20 will be wired
   *  to 0.  Not too desirable.
   */

  gateA20 (1);

  /*
   *  The "mbi.mem_upper" variable only recognizes upper memory in the
   *  first memory region.  If there are multiple memory regions,
   *  the rest are reported to a Multiboot-compliant OS, but otherwise
   *  unused by GRUB.
   */

  addr = get_code_end ();
  mbi.mmap_addr = addr;
  mbi.mmap_length = 0;
  cont = 0;

  do
    {
      cont = get_mem_map (addr, cont);

      /* If the returned buffer's base is zero, quit. */
      if (!*((int *) addr))
	break;

      mbi.mmap_length += *((int *) addr) + 4;
      addr += *((int *) addr) + 4;
    }
  while (cont);

  if (mbi.mmap_length)
    {
      /*
       *  This is to get the upper memory up to the first memory
       *  hole into the "mbi.mem_upper" element, for OS's that
       *  don't care about the memory map, but might care about
       *  RAM above 64MB.
       *
       *  A big problem is that the memory areas aren't guaranteed
       *  to be:  (1) contiguous, (2) sorted in ascending order, or
       *  (3) non-overlapping.
       */
      memtmp = 0x100000;

      do
	{
	  for (cont = 0, addr = mbi.mmap_addr;
	       addr < mbi.mmap_addr + mbi.mmap_length;
	       addr += *((int *) addr) + 4)
	    {
	      if (((struct AddrRangeDesc *) addr)->BaseAddrHigh == 0
		  && ((struct AddrRangeDesc *) addr)->Type == MB_ARD_MEMORY
		  && ((struct AddrRangeDesc *) addr)->BaseAddrLow <= memtmp
		  && (((struct AddrRangeDesc *) addr)->BaseAddrLow
		      + ((struct AddrRangeDesc *) addr)->LengthLow) > memtmp)
		{
		  memtmp = (((struct AddrRangeDesc *) addr)->BaseAddrLow
			    + ((struct AddrRangeDesc *) addr)->LengthLow);
		  cont++;
		}
	    }
	}
      while (cont);

      mbi.mem_upper = (memtmp - 0x100000) >> 10;
    }
  else if ((memtmp = get_eisamemsize ()) != -1)
    {
      cont = memtmp & ~0xFFFF;
      memtmp = memtmp & 0xFFFF;

      if (!cont || (memtmp == 0x3c00))
	memtmp += (cont >> 10);
      else
	{
	  /* XXX should I do this at all ??? */

	  mbi.mmap_addr = (int) fakemap;
	  mbi.mmap_length = sizeof (fakemap);
	  fakemap[0].LengthLow = (mbi.mem_lower << 10);
	  fakemap[1].LengthLow = (memtmp << 10);
	  fakemap[2].LengthLow = cont;
	}

      mbi.mem_upper = memtmp;
    }

  saved_mem_upper = mbi.mem_upper;

  /*
   *  Initialize other Multiboot Info flags.
   */

  mbi.flags = MB_INFO_MEMORY | MB_INFO_CMDLINE | MB_INFO_BOOTDEV;

#endif /* STAGE1_5 */

  /*
   *  Set boot drive and partition.
   */

  saved_drive = boot_drive;
  saved_partition = install_partition;

  /*
   *  Start main routine here.
   */

  cmain ();
}
