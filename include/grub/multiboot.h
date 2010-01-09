/* multiboot.h - multiboot header file with grub definitions. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2008,2010  Free Software Foundation, Inc.
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

#ifndef GRUB_MULTIBOOT_HEADER
#define GRUB_MULTIBOOT_HEADER 1

#ifdef GRUB_USE_MULTIBOOT2
#include <multiboot2.h>
/* Same thing as far as our loader is concerned.  */
#define MULTIBOOT_BOOTLOADER_MAGIC	MULTIBOOT2_BOOTLOADER_MAGIC
#define MULTIBOOT_HEADER_MAGIC		MULTIBOOT2_HEADER_MAGIC
#else
#include <multiboot.h>
#endif

void grub_multiboot (int argc, char *argv[]);
void grub_module (int argc, char *argv[]);

#endif /* ! GRUB_MULTIBOOT_HEADER */
