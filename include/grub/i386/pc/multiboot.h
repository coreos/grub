/* multiboot.h - multiboot header file. */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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

#ifndef PUPA_MULTIBOOT_MACHINE_HEADER
#define PUPA_MULTIBOOT_MACHINE_HEADER 1

/* How many bytes from the start of the file we search for the header.  */
#define PUPA_MB_SEARCH                 8192

/* The magic field should contain this.  */
#define PUPA_MB_MAGIC                  0x1BADB002

/* This should be in %eax.  */
#define PUPA_MB_MAGIC2                 0x2BADB002

/* The bits in the required part of flags field we don't support.  */
#define PUPA_MB_UNSUPPORTED            0x0000fffc

/* Alignment of multiboot modules.  */
#define PUPA_MB_MOD_ALIGN              0x00001000

/* 
 * Flags set in the 'flags' member of the multiboot header.
 */

/* Align all boot modules on i386 page (4KB) boundaries.  */
#define PUPA_MB_PAGE_ALIGN		0x00000001

/* Must pass memory information to OS.  */
#define PUPA_MB_MEMORY_INFO		0x00000002

/* Must pass video information to OS.  */
#define PUPA_MB_VIDEO_MODE		0x00000004

/* This flag indicates the use of the address fields in the header.  */
#define PUPA_MB_AOUT_KLUDGE		0x00010000

/*
 *  Flags to be set in the 'flags' member of the multiboot info structure.
 */

/* is there basic lower/upper memory information? */
#define PUPA_MB_INFO_MEMORY		0x00000001
/* is there a boot device set? */
#define PUPA_MB_INFO_BOOTDEV		0x00000002
/* is the command-line defined? */
#define PUPA_MB_INFO_CMDLINE		0x00000004
/* are there modules to do something with? */
#define PUPA_MB_INFO_MODS		0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define PUPA_MB_INFO_AOUT_SYMS		0x00000010
/* is there an ELF section header table? */
#define PUPA_MB_INFO_ELF_SHDR		0x00000020

/* is there a full memory map? */
#define PUPA_MB_INFO_MEM_MAP		0x00000040

/* Is there drive info?  */
#define PUPA_MB_INFO_DRIVE_INFO		0x00000080

/* Is there a config table?  */
#define PUPA_MB_INFO_CONFIG_TABLE	0x00000100

/* Is there a boot loader name?  */
#define PUPA_MB_INFO_BOOT_LOADER_NAME	0x00000200

/* Is there a APM table?  */
#define PUPA_MB_INFO_APM_TABLE		0x00000400

/* Is there video information?  */
#define PUPA_MB_INFO_VIDEO_INFO		0x00000800

#ifndef ASM_FILE

#include <pupa/types.h>

struct pupa_multiboot_header
{ 
  /* Must be PUPA_MB_MAGIC - see above.  */
  pupa_uint32_t magic;

  /* Feature flags.  */
  pupa_uint32_t flags;

  /* The above fields plus this one must equal 0 mod 2^32. */
  pupa_uint32_t checksum;
  
  /* These are only valid if PUPA_MB_AOUT_KLUDGE is set.  */
  pupa_uint32_t header_addr;
  pupa_uint32_t load_addr;
  pupa_uint32_t load_end_addr;
  pupa_uint32_t bss_end_addr;
  pupa_uint32_t entry_addr;

  /* These are only valid if PUPA_MB_VIDEO_MODE is set.  */
  pupa_uint32_t mode_type;
  pupa_uint32_t width;
  pupa_uint32_t height;
  pupa_uint32_t depth;
};

struct pupa_multiboot_info
{
  /* MultiBoot info version number */
  pupa_uint32_t flags;
  
  /* Available memory from BIOS */
  pupa_uint32_t mem_lower;
  pupa_uint32_t mem_upper;
  
  /* "root" partition */
  pupa_uint32_t boot_device;
  
  /* Kernel command line */
  pupa_uint32_t cmdline;
  
  /* Boot-Module list */
  pupa_uint32_t mods_count;
  pupa_uint32_t mods_addr;
  
  pupa_uint32_t syms[4];
  
  /* Memory Mapping buffer */
  pupa_uint32_t mmap_length;
  pupa_uint32_t mmap_addr;
  
  /* Drive Info buffer */
  pupa_uint32_t drives_length;
  pupa_uint32_t drives_addr;
  
  /* ROM configuration table */
  pupa_uint32_t config_table;
  
  /* Boot Loader Name */
  pupa_uint32_t boot_loader_name;

  /* APM table */
  pupa_uint32_t apm_table;

  /* Video */
  pupa_uint32_t vbe_control_info;
  pupa_uint32_t vbe_mode_info;
  pupa_uint16_t vbe_mode;
  pupa_uint16_t vbe_interface_seg;
  pupa_uint16_t vbe_interface_off;
  pupa_uint16_t vbe_interface_len;
};

struct pupa_mod_list
{
  /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
  pupa_uint32_t mod_start;
  pupa_uint32_t mod_end;
  
  /* Module command line */
  pupa_uint32_t cmdline;
  
  /* padding to take it to 16 bytes (must be zero) */
  pupa_uint32_t pad;
};

#endif /* ! ASM_FILE */

#endif /* ! PUPA_MULTIBOOT_MACHINE_HEADER */
