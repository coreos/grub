/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2003  Yoshinori K. Okuji <okuji@enbug.org>
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

#ifndef PUPA_LINUX_MACHINE_HEADER
#define PUPA_LINUX_MACHINE_HEADER	1

#define PUPA_LINUX_MAGIC_SIGNATURE	0x53726448      /* "HdrS" */
#define PUPA_LINUX_DEFAULT_SETUP_SECTS	4
#define PUPA_LINUX_FLAG_CAN_USE_HEAP	0x80
#define PUPA_LINUX_INITRD_MAX_ADDRESS	0x38000000
#define PUPA_LINUX_MAX_SETUP_SECTS	64
#define PUPA_LINUX_BOOT_LOADER_TYPE	0x72
#define PUPA_LINUX_HEAP_END_OFFSET	(0x9000 - 0x200)

#define PUPA_LINUX_BZIMAGE_ADDR		0x100000
#define PUPA_LINUX_ZIMAGE_ADDR		0x10000
#define PUPA_LINUX_OLD_REAL_MODE_ADDR	0x90000
#define PUPA_LINUX_SETUP_STACK		0x9000

#define PUPA_LINUX_FLAG_BIG_KERNEL	0x1

/* Linux's video mode selection support. Actually I hate it!  */
#define PUPA_LINUX_VID_MODE_NORMAL	0xFFFF
#define PUPA_LINUX_VID_MODE_EXTENDED	0xFFFE
#define PUPA_LINUX_VID_MODE_ASK		0xFFFD

#define PUPA_LINUX_CL_OFFSET		0x9000
#define PUPA_LINUX_CL_END_OFFSET	0x90FF
#define PUPA_LINUX_SETUP_MOVE_SIZE	0x9100
#define PUPA_LINUX_CL_MAGIC		0xA33F

#ifndef ASM_FILE

/* For the Linux/i386 boot protocol version 2.03.  */
struct linux_kernel_header
{ 
  pupa_uint8_t code1[0x0020];
  pupa_uint16_t cl_magic;		/* Magic number 0xA33F */
  pupa_uint16_t cl_offset;		/* The offset of command line */
  pupa_uint8_t code2[0x01F1 - 0x0020 - 2 - 2];
  pupa_uint8_t setup_sects;		/* The size of the setup in sectors */
  pupa_uint16_t root_flags;		/* If the root is mounted readonly */
  pupa_uint16_t syssize;		/* obsolete */
  pupa_uint16_t swap_dev;		/* obsolete */
  pupa_uint16_t ram_size;		/* obsolete */
  pupa_uint16_t vid_mode;		/* Video mode control */
  pupa_uint16_t root_dev;		/* Default root device number */
  pupa_uint16_t boot_flag;		/* 0xAA55 magic number */
  pupa_uint16_t jump;			/* Jump instruction */
  pupa_uint32_t header;			/* Magic signature "HdrS" */
  pupa_uint16_t version;		/* Boot protocol version supported */
  pupa_uint32_t realmode_swtch;		/* Boot loader hook */
  pupa_uint32_t start_sys;		/* Points to kernel version string */
  pupa_uint8_t type_of_loader;		/* Boot loader identifier */
  pupa_uint8_t loadflags;		/* Boot protocol option flags */
  pupa_uint16_t setup_move_size;	/* Move to high memory size */
  pupa_uint32_t code32_start;		/* Boot loader hook */
  pupa_uint32_t ramdisk_image;		/* initrd load address */
  pupa_uint32_t ramdisk_size;		/* initrd size */
  pupa_uint32_t bootsect_kludge;	/* obsolete */
  pupa_uint16_t heap_end_ptr;		/* Free memory after setup end */
  pupa_uint16_t pad1;			/* Unused */
  char *cmd_line_ptr;			/* Points to the kernel command line */
} __attribute__ ((packed));

#endif /* ! ASM_FILE */

#endif /* ! PUPA_LINUX_MACHINE_HEADER */
