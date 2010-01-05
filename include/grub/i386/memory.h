/* memory.h - describe the memory map */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_MEMORY_CPU_HEADER
#define GRUB_MEMORY_CPU_HEADER	1

/* The flag for protected mode.  */
#define GRUB_MEMORY_CPU_CR0_PE_ON		0x1
#define GRUB_MEMORY_CPU_CR4_PAE_ON		0x00000040
#define GRUB_MEMORY_CPU_CR0_PAGING_ON		0x80000000
#define GRUB_MEMORY_CPU_AMD64_MSR		0xc0000080
#define GRUB_MEMORY_CPU_AMD64_MSR_ON		0x00000100

#endif /* ! GRUB_MEMORY_CPU_HEADER */
