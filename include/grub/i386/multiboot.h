/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_MULTIBOOT_CPU_HEADER
#define GRUB_MULTIBOOT_CPU_HEADER	1

/* The asm part of the multiboot loader.  */
void grub_multiboot_real_boot (grub_addr_t entry,
			       struct multiboot_info *mbi)
     __attribute__ ((noreturn));
void grub_multiboot2_real_boot (grub_addr_t entry,
				struct multiboot_info *mbi)
     __attribute__ ((noreturn));

extern grub_uint32_t grub_multiboot_payload_eip;
extern char *grub_multiboot_payload_orig;
extern grub_addr_t grub_multiboot_payload_dest;
extern grub_size_t grub_multiboot_payload_size;

#endif /* ! GRUB_MULTIBOOT_CPU_HEADER */
