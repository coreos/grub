/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999, 2000   Free Software Foundation, Inc.
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

#ifndef PUPA_BOOT_MACHINE_HEADER
#define PUPA_BOOT_MACHINE_HEADER	1

/* The signature for bootloader.  */
#define PUPA_BOOT_MACHINE_SIGNATURE	0xaa55

/* The offset of the end of BPB (BIOS Parameter Block).  */
#define PUPA_BOOT_MACHINE_BPBEND	0x3e

/* The offset of the major version.  */
#define PUPA_BOOT_MACHINE_VER_MAJ	0x3e

/* The offset of BOOT_DRIVE.  */
#define PUPA_BOOT_MACHINE_BOOT_DRIVE	0x40

/* The offset of FORCE_LBA.  */
#define PUPA_BOOT_MACHINE_FORCE_LBA	0x41

/* The offset of KERNEL_ADDRESS.  */
#define PUPA_BOOT_MACHINE_KERNEL_ADDRESS	0x42

/* The offset of KERNEL_SECTOR.  */
#define PUPA_BOOT_MACHINE_KERNEL_SECTOR	0x44

/* The offset of KERNEL_SEGMENT.  */
#define PUPA_BOOT_MACHINE_KERNEL_SEGMENT	0x48

/* The offset of a magic number used by Windows NT.  */
#define PUPA_BOOT_MACHINE_WINDOWS_NT_MAGIC	0x1b8

/* The offset of the start of the partition table.  */
#define PUPA_BOOT_MACHINE_PART_START	0x1be

/* The offset of the end of the partition table.  */
#define PUPA_BOOT_MACHINE_PART_END	0x1fe

/* The stack segment.  */
#define PUPA_BOOT_MACHINE_STACK_SEG	0x2000

/* The segment of disk buffer. The disk buffer MUST be 32K long and
   cannot straddle a 64K boundary.  */
#define PUPA_BOOT_MACHINE_BUFFER_SEG	0x7000

/* The address of drive parameters.  */
#define PUPA_BOOT_MACHINE_DRP_ADDR	0x7f00

/* The size of drive parameters.  */
#define PUPA_BOOT_MACHINE_DRP_SIZE	0x42

/* The flag for BIOS drive number to designate a hard disk vs. a
   floppy.  */
#define PUPA_BOOT_MACHINE_BIOS_HD_FLAG	0x80

/* The segment where the kernel is loaded.  */
#define PUPA_BOOT_MACHINE_KERNEL_SEG	0x800

/* The address where the kernel is loaded.  */
#define PUPA_BOOT_MACHINE_KERNEL_ADDR	(PUPA_BOOT_MACHINE_KERNEL_SEG << 4)

/* The size of a block list used in the kernel startup code.  */
#define PUPA_BOOT_MACHINE_LIST_SIZE	8

#endif /* ! BOOT_MACHINE_HEADER */
