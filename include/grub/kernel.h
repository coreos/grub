/*
 *  GRUB  --  GRand Unified Bootloader
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

#ifndef GRUB_KERNEL_HEADER
#define GRUB_KERNEL_HEADER	1

#include <grub/types.h>

/* The module header.  */
struct grub_module_header
{
  /* The offset of object code.  */
  grub_off_t offset;
  /* The size of object code plus this header.  */
  grub_size_t size;
};

/* The start address of the kernel.  */
extern grub_addr_t grub_start_addr;

/* The end address of the kernel.  */
extern grub_addr_t grub_end_addr;

/* The total size of modules including their headers.  */
extern grub_size_t grub_total_module_size;

/* The size of the kernel image.  */
extern grub_size_t grub_kernel_image_size;

/* The start point of the C code.  */
void grub_main (void);

/* The machine-specific initialization. This must initialize memory.  */
void grub_machine_init (void);

/* Return the end address of the core image.  */
grub_addr_t grub_get_end_addr (void);

/* Register all the exported symbols. This is automatically generated.  */
void grub_register_exported_symbols (void);

#endif /* ! GRUB_KERNEL_HEADER */
