/* pe32.h  - PE/Coff definitions.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#ifdef USE_PE32PLUS
typedef uint64_t pe32_uintptr_t;
#else
typedef uint32_t pe32_uintptr_t;
#endif

struct pe32_coff_header
{
  uint16_t machine;
  uint16_t num_sections;
  uint32_t time;
  uint32_t symtab_offset;
  uint32_t num_symbols;
  uint16_t optional_header_size;
  uint16_t characteristics;
};

#define PE32_MACHINE_I386     0x014c
#define PE32_MACHINE_IA64     0x0200
#define PE32_MACHINE_EBC      0x0EBC
#define PE32_MACHINE_X64      0x8664

#define PE32_RELOCS_STRIPPED		0x0001
#define PE32_EXECUTABLE_IMAGE		0x0002
#define PE32_LINE_NUMS_STRIPPED		0x0004
#define PE32_LOCAL_SYMS_STRIPPED	0x0008
#define PE32_AGGRESSIVE_WS_TRIM		0x0010
#define PE32_LARGE_ADDRESS_AWARE	0x0020
#define PE32_16BIT_MACHINE		0x0040
#define PE32_BYTES_REVERSED_LO		0x0080
#define PE32_32BIT_MACHINE		0x0100
#define PE32_DEBUG_STRIPPED		0x0200
#define PE32_REMOVABLE_RUN_FROM_SWAP	0x0400
#define PE32_SYSTEM			0x1000
#define PE32_DLL			0x2000
#define PE32_UP_SYSTEM_ONLY		0x4000
#define PE32_BYTES_REVERSED_HI		0x8000

struct pe32_data_directory
{
  uint32_t rva;
  uint32_t size;
};

struct pe32_optional_header
{
  uint16_t magic;
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t code_size;
  uint32_t data_size;
  uint32_t bss_size;
  uint32_t entry_addr;
  uint32_t code_base;
  
#ifndef USE_PE32PLUS
  uint32_t data_base;
#endif

  pe32_uintptr_t image_base;
  uint32_t section_alignment;
  uint32_t file_alignment;
  uint16_t major_os_version;
  uint16_t minor_os_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version;
  uint16_t minor_subsystem_version;
  uint32_t reserved;
  uint32_t image_size;
  uint32_t header_size;
  uint32_t checksum;
  uint16_t subsystem;
  uint16_t dll_characteristics;
  pe32_uintptr_t stack_reserve_size;
  pe32_uintptr_t stack_commit_size;
  pe32_uintptr_t heap_reserve_size;
  pe32_uintptr_t heap_commit_size;
  uint32_t loader_flags;
  uint32_t num_data_directories;
  
  /* Data directories.  */
  struct pe32_data_directory export_table;
  struct pe32_data_directory import_table;
  struct pe32_data_directory resource_table;
  struct pe32_data_directory exception_table;
  struct pe32_data_directory certificate_table;
  struct pe32_data_directory base_relocation_table;
  struct pe32_data_directory debug;
  struct pe32_data_directory architecture;
  struct pe32_data_directory global_ptr;
  struct pe32_data_directory tls_table;
  struct pe32_data_directory load_config_table;
  struct pe32_data_directory bound_import;
  struct pe32_data_directory iat;
  struct pe32_data_directory delay_import_descriptor;
  struct pe32_data_directory com_runtime_header;
  struct pe32_data_directory reserved_entry;
};

#define PE32_PE32_MAGIC	0x10b
#define PE32_PE64_MAGIC	0x20b

#define PE32_SUBSYSTEM_EFI_APPLICATION	       10
#define PE32_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define PE32_SUBSYSTEM_EFI_RUNTIME_DRIVER      12
#define PE32_SUBSYSTEM_EFI_EFI_ROM             13

#define PE32_NUM_DATA_DIRECTORIES	16

struct pe32_section_header
{
  char name[8];
  uint32_t virtual_size;
  uint32_t virtual_address;
  uint32_t raw_data_size;
  uint32_t raw_data_offset;
  uint32_t relocations_offset;
  uint32_t line_numbers_offset;
  uint16_t num_relocations;
  uint16_t num_line_numbers;
  uint32_t characteristics;
};

#define PE32_SCN_CNT_CODE		0x00000020
#define PE32_SCN_CNT_INITIALIZED_DATA	0x00000040
#define PE32_SCN_MEM_DISCARDABLE	0x02000000
#define PE32_SCN_MEM_EXECUTE		0x20000000
#define PE32_SCN_MEM_READ		0x40000000
#define PE32_SCN_MEM_WRITE		0x80000000

struct pe32_dos_header
{
  uint16_t  magic;    // Magic number
  uint16_t  cblp;     // Bytes on last page of file
  uint16_t  cp;       // Pages in file
  uint16_t  crlc;     // Relocations
  uint16_t  cparhdr;  // Size of header in paragraphs
  uint16_t  minalloc; // Minimum extra paragraphs needed
  uint16_t  maxalloc; // Maximum extra paragraphs needed
  uint16_t  ss;       // Initial (relative) SS value
  uint16_t  sp;       // Initial SP value
  uint16_t  csum;     // Checksum
  uint16_t  ip;       // Initial IP value
  uint16_t  cs;       // Initial (relative) CS value
  uint16_t  lfa_rlc;   // File address of relocation table
  uint16_t  ov_no;     // Overlay number
  uint16_t  res[4];   // Reserved words
  uint16_t  oem_id;    // OEM identifier (for e_oeminfo)
  uint16_t  oem_info;  // OEM information; e_oemid specific
  uint16_t  res2[10]; // Reserved words
  uint32_t  new_hdr_offset;

  uint16_t  stub[0x20];
};

struct pe32_nt_header
{
  /* This is always PE\0\0.  */
  char signature[4];

  /* The COFF file header.  */
  struct pe32_coff_header coff_header;

  /* The Optional header.  */
  struct pe32_optional_header optional_header;
};

struct pe32_base_relocation
{
  uint32_t page_rva;
  uint32_t block_size;
};

struct pe32_fixup_block
{
  uint32_t page_rva;
  uint32_t block_size;
  uint16_t entries[0];
};

#define PE32_FIXUP_ENTRY(type, offset)	(((type) << 12) | (offset))

#define PE32_REL_BASED_ABSOLUTE	    0
#define PE32_REL_BASED_HIGHLOW	    3
#define PE32_REL_BASED_IA64_IMM64   9
#define PE32_REL_BASED_DIR64        10

#define PE32_DEBUG_TYPE_CODEVIEW 2
struct pe32_debug_directory_entry {
  uint32_t  characteristics;
  uint32_t  time;
  uint16_t  major_version;
  uint16_t  minor_version;
  uint32_t  type;
  uint32_t  data_size;
  uint32_t  rva;
  uint32_t  file_offset;
};

#define PE32_CODEVIEW_SIGNATURE_NB10 0x3031424E  // "NB10"
struct pe32_debug_codeview_nb10_entry {
  uint32_t  signature;                        // "NB10"
  uint32_t  unknown[3];
  char filename[0];  /* Filename of .PDB */
};


#if 1
#define pe32_check(name, x) extern char pe32_check_##name [x ? 1 : -1]
#ifdef USE_PE32PLUS
#define PE32_HEADER_SIZE 112
#else
#define PE32_HEADER_SIZE 96
#endif

pe32_check(optional_header, sizeof (struct pe32_optional_header) == PE32_HEADER_SIZE + PE32_NUM_DATA_DIRECTORIES * 8);
#endif

