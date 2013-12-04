/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#ifndef GRUB_LINUX_CPU_HEADER
#define GRUB_LINUX_CPU_HEADER 1

#include <grub/efi/efi.h>

#define GRUB_EFI_KERNEL_STUB_ENTRY_OFFSET 0
#define GRUB_LINUX_MAX_LOAD_ADDR 0xffffffffffffULL

#define GRUB_LINUX_MAGIC 0x644d5241 /* 'ARM\x64' */

/* From linux/Documentation/arm64/booting.txt */
struct linux_kernel_header
{
  grub_uint32_t code0;		/* Executable code */
  grub_uint32_t code1;		/* Executable code */
  grub_uint64_t text_offset;    /* Image load offset */
  grub_uint64_t res0;		/* reserved */
  grub_uint64_t res1;		/* reserved */
  grub_uint64_t res2;		/* reserved */
  grub_uint64_t res3;		/* reserved */
  grub_uint64_t res4;		/* reserved */
  grub_uint32_t magic;		/* Magic number, little endian, "ARM\x64" */
  grub_uint32_t hdr_offset;	/* Offset of PE/COFF header */
};

#define grub_linux_get_params() NULL
extern grub_err_t grub_linux_init_params (void);
extern grub_err_t grub_linux_finalize_params (void);
extern grub_err_t grub_linux_register_kernel (struct linux_kernel_header *lh);
extern grub_err_t grub_linux_register_cmdline (void * addr);
extern grub_err_t grub_linux_register_initrd (void * addr, grub_size_t size);

extern void grub_efi_linux_arch_register_commands (void);
extern void grub_efi_linux_arch_unregister_commands (void);

#endif /* ! GRUB_LINUX_CPU_HEADER */
