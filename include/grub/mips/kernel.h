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

#ifndef GRUB_KERNEL_CPU_HEADER
#define GRUB_KERNEL_CPU_HEADER	1

#define GRUB_MOD_ALIGN 0x1
/* Non-zero value is only needed for PowerMacs.  */
#define GRUB_MOD_GAP   0x0

#define GRUB_KERNEL_MACHINE_LINK_ALIGN  32

#define GRUB_KERNEL_CPU_RAW_SIZE        0x200
#define GRUB_KERNEL_CPU_COMPRESSED_SIZE        0x8
#define GRUB_KERNEL_CPU_TOTAL_MODULE_SIZE      0xc
#define GRUB_KERNEL_CPU_KERNEL_IMAGE_SIZE      0x10

#define GRUB_KERNEL_CPU_PREFIX	        GRUB_KERNEL_CPU_RAW_SIZE
#define GRUB_KERNEL_CPU_DATA_END	GRUB_KERNEL_CPU_RAW_SIZE + 0x48

#define GRUB_KERNEL_MACHINE_RAW_SIZE GRUB_KERNEL_CPU_RAW_SIZE

#define GRUB_KERNEL_MACHINE_PREFIX	GRUB_KERNEL_CPU_PREFIX
#define GRUB_KERNEL_MACHINE_DATA_END	GRUB_KERNEL_CPU_DATA_END
#define GRUB_KERNEL_MACHINE_KERNEL_IMAGE_SIZE GRUB_KERNEL_CPU_KERNEL_IMAGE_SIZE
#define GRUB_KERNEL_MACHINE_TOTAL_MODULE_SIZE GRUB_KERNEL_CPU_TOTAL_MODULE_SIZE
#define GRUB_KERNEL_MACHINE_COMPRESSED_SIZE GRUB_KERNEL_CPU_COMPRESSED_SIZE

#define GRUB_PLATFORM_IMAGE_FORMATS     "raw, elf"
#define GRUB_PLATFORM_IMAGE_DEFAULT_FORMAT     "raw"

#define GRUB_PLATFORM_IMAGE_DEFAULT GRUB_PLATFORM_IMAGE_RAW

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

#endif
