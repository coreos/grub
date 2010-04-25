/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_KERNEL_MACHINE_HEADER
#define GRUB_KERNEL_MACHINE_HEADER	1

#include <grub/symbol.h>

#define GRUB_PLATFORM_IMAGE_FORMATS     "raw, elf"
#define GRUB_PLATFORM_IMAGE_DEFAULT_FORMAT     "raw"

#define GRUB_PLATFORM_IMAGE_DEFAULT GRUB_PLATFORM_IMAGE_RAW
#define GRUB_KERNEL_MACHINE_PREFIX		0x4
#define GRUB_KERNEL_MACHINE_DATA_END	0x44
#define GRUB_KERNEL_MACHINE_LINK_ALIGN 4

#define EM_TARGET EM_PPC
#define GRUB_KERNEL_MACHINE_LINK_ADDR 0x200000

#ifndef ASM_FILE

typedef enum {
  GRUB_PLATFORM_IMAGE_RAW,
  GRUB_PLATFORM_IMAGE_ELF
}
  grub_platform_image_format_t;
#define GRUB_PLATFORM_IMAGE_RAW GRUB_PLATFORM_IMAGE_RAW
#define GRUB_PLATFORM_IMAGE_ELF GRUB_PLATFORM_IMAGE_ELF

/* The prefix which points to the directory where GRUB modules and its
   configuration file are located.  */
extern char grub_prefix[];

#endif

#endif /* ! GRUB_KERNEL_MACHINE_HEADER */
