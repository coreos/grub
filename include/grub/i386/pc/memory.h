/* memory.h - describe the memory map */
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

#ifndef PUPA_MEMORY_MACHINE_HEADER
#define PUPA_MEMORY_MACHINE_HEADER	1

/* The scratch buffer used in real mode code.  */
#define PUPA_MEMORY_MACHINE_SCRATCH_ADDR	0x68000
#define PUPA_MEMORY_MACHINE_SCRATCH_SEG	(PUPA_MEMORY_MACHINE_SCRATCH_ADDR >> 4)
#define PUPA_MEMORY_MACHINE_SCRATCH_SIZE	0x10000

/* The real mode stack.  */
#define PUPA_MEMORY_MACHINE_REAL_STACK	(0x2000 - 0x10)

/* The size of the protect mode stack.  */
#define PUPA_MEMORY_MACHINE_PROT_STACK_SIZE	0x8000

/* The protected mode stack.  */
#define PUPA_MEMORY_MACHINE_PROT_STACK	\
	(PUPA_MEMORY_MACHINE_SCRATCH_ADDR + PUPA_MEMORY_MACHINE_SCRATCH_SIZE \
	 + PUPA_MEMORY_MACHINE_PROT_STACK_SIZE - 0x10)

/* The memory area where PUPA uses its own purpose.  */
#define PUPA_MEMORY_MACHINE_RESERVED_START	\
	PUPA_MEMORY_MACHINE_SCRATCH_ADDR
#define PUPA_MEMORY_MACHINE_RESERVED_END	\
	(PUPA_MEMORY_MACHINE_PROT_STACK + 0x10)

/* The address of a partition table passed to another boot loader.  */
#define PUPA_MEMORY_MACHINE_PART_TABLE_ADDR	0x7be

/* The address where another boot loader is loaded.  */
#define PUPA_MEMORY_MACHINE_BOOT_LOADER_ADDR	0x7c00

/* The flag for protected mode.  */
#define PUPA_MEMORY_MACHINE_CR0_PE_ON		0x1

/* The code segment of the protected mode.  */
#define PUPA_MEMORY_MACHINE_PROT_MODE_CSEG	0x8

/* The data segment of the protected mode.  */
#define PUPA_MEMORY_MACHINE_PROT_MODE_DSEG	0x10

/* The code segment of the pseudo real mode.  */
#define PUPA_MEMORY_MACHINE_PSEUDO_REAL_CSEG	0x18

/* The data segment of the pseudo real mode.  */
#define PUPA_MEMORY_MACHINE_PSEUDO_REAL_DSEG	0x20

#endif /* ! PUPA_MEMORY_MACHINE_HEADER */
