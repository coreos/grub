/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999   Free Software Foundation, Inc.
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

#ifndef STAGE1_HEADER
#define STAGE1_HEADER	1


/* Define the version numbers here, so that Stage 1 can know them.  */
#define COMPAT_VERSION_MAJOR	2
#define COMPAT_VERSION_MINOR	2
#define COMPAT_VERSION		((COMPAT_VERSION_MINOR << 8) \
					| COMPAT_VERSION_MAJOR)

/* The signature for bootloader.  */
#define STAGE1_SIGNATURE	0xaa55

/* The offset of the end of BPB (BIOS Parameter Block).  */
#define STAGE1_BPBEND		0x3e

/* The offset of FIRSTLIST.  */
#define STAGE1_FIRSTLIST	0x1b5

/* The offset of the major version.  */
#define STAGE1_VER_MAJ_OFFS	0x1bc

/* The offset of the install segment.  */
#define STAGE1_INSTALLSEG	0x1ba

/* The offset of the install address.  */
#define STAGE1_INSTALLADDR	0x1b8

/* The offset of the start of the partition table.  */
#define STAGE1_PARTSTART	0x1be

/* The offset of the end of the partition table.  */
#define STAGE1_PARTEND		0x1fe

/* The size of extra data parameters.  */
#define STAGE1_MINPARAMSIZE	13

/* The size of sector list.  */
#define STAGE1_LISTSIZE		8

/* The stack segment.  */
#define STAGE1_STACKSEG		0x2000

/* The segment of disk buffer. The disk buffer MUST be 32K long and
   cannot straddle a 64K boundary.  */
#define STAGE1_BUFFERSEG	0x7000

/* The address of drive parameters.  */
#define STAGE1_DRP_ADDR		0x7f00

/* The size of drive parameters.  */
#define STAGE1_DRP_SIZE		0x42

/* The flag for BIOS drive number to designate a hard disk vs. a
   floppy.  */
#define STAGE1_BIOS_HD_FLAG	0x80

/* Stage 1 identifiers */
/* The offset of the id.  */
#define STAGE1_ID_OFFSET	STAGE1_BPBEND

/* The id for the ``normal'' stage1.  */
#define STAGE1_ID_CHS		0x10

/* The id for the LBA version of stage1.  */
#define STAGE1_ID_LBA		0x20


#endif /* ! STAGE1_HEADER */
