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

#ifndef KERNEL_MACHINE_HEADER
#define KERNEL_MACHINE_HEADER	1

#include <pupa/types.h>

/* The offset of PUPA_TOTAL_MODULE_SIZE.  */
#define PUPA_KERNEL_MACHINE_TOTAL_MODULE_SIZE	0x8

/* The offset of PUPA_KERNEL_IMAGE_SIZE.  */
#define PUPA_KERNEL_MACHINE_KERNEL_IMAGE_SIZE	0xc

/* The offset of PUPA_INSTALL_DOS_PART.  */
#define PUPA_KERNEL_MACHINE_INSTALL_DOS_PART	0x10

/* The offset of PUPA_INSTALL_BSD_PART.  */
#define PUPA_KERNEL_MACHINE_INSTALL_BSD_PART	0x14

/* The offset of PUPA_PREFIX.  */
#define PUPA_KERNEL_MACHINE_PREFIX		0x18

/* The DOS partition number of the installed partition.  */
extern pupa_int32_t pupa_install_dos_part;

/* The BSD partition number of the installed partition.  */
extern pupa_int32_t pupa_install_bsd_part;

/* The prefix which points to the directory where PUPA modules and its
   configuration file are located.  */
extern char pupa_prefix[];

/* The boot BIOS drive number.  */
extern pupa_int32_t pupa_boot_drive;

#endif /* ! KERNEL_MACHINE_HEADER */
