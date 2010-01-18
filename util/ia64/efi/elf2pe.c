/* elf2pe.c - convert elf binary to PE/Coff.  */
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <elf.h>

#if defined(ELF2PE_I386)
#define USE_ELF32
#define USE_PE32
#define ELF_MACHINE EM_386
#define EFI_MACHINE PE32_MACHINE_I386
#elif defined(ELF2PE_IA64)
#define USE_ELF64
#define USE_PE32PLUS
#define ELF_MACHINE EM_IA_64
#define EFI_MACHINE PE32_MACHINE_IA64
#else
#error "unknown architecture"
#endif

#include "pe32.h"

const char *filename;

int
is_elf_header(uint8_t *buffer)
{
  return (buffer[EI_MAG0] == ELFMAG0
	  && buffer[EI_MAG1] == ELFMAG1
	  && buffer[EI_MAG2] == ELFMAG2
	  && buffer[EI_MAG3] == ELFMAG3);
}

#ifdef USE_ELF32
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Sym Elf_Sym;
#define ELFCLASS ELFCLASS32
#define ELF_R_TYPE(r) ELF32_R_TYPE(r)
#define ELF_R_SYM(r) ELF32_R_SYM(r)
#else
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Rel Elf_Rel;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Sym Elf_Sym;
#define ELFCLASS ELFCLASS64
#define ELF_R_TYPE(r) ELF64_R_TYPE(r)
#define ELF_R_SYM(r) ELF64_R_SYM(r)
#endif

#ifdef ELF2PE_IA64
#define ELF_ETYPE ET_DYN
#else
#define ELF_ETYPE ET_EXEC
#endif

/* Well known ELF structures.  */
Elf_Ehdr *ehdr;
Elf_Shdr *shdr_base;
Elf_Shdr *shdr_dynamic;
const uint8_t *shdr_str;

/* PE section alignment.  */
const uint32_t coff_alignment = 0x20;
const uint32_t coff_nbr_sections = 4;

/* Current offset in coff file.  */
uint32_t coff_offset;

/* Result Coff file in memory.  */
uint8_t *coff_file;

/* Offset in Coff file of headers and sections.  */
uint32_t nt_hdr_offset;
uint32_t table_offset;
uint32_t text_offset;
uint32_t data_offset;
uint32_t reloc_offset;

#ifdef ELF2PE_IA64
uint32_t coff_entry_descr_offset;
uint32_t coff_entry_descr_func;
uint64_t plt_base;
#endif

/* ELF sections to offset in Coff file.  */
uint32_t *coff_sections_offset;

struct pe32_fixup_block *coff_base_rel;
uint16_t *coff_entry_rel;

uint32_t
coff_align(uint32_t offset)
{
  return (offset + coff_alignment - 1) & ~(coff_alignment - 1);
}

Elf_Shdr *
get_shdr_by_index(uint32_t num)
{
  if (num >= ehdr->e_shnum)
    return NULL;
  return (Elf_Shdr*)((uint8_t*)shdr_base + num * ehdr->e_shentsize);
}

int
check_elf_header (void)
{
  /* Note: Magic has already been tested.  */
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS)
    return 0;
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return 0;
  if (ehdr->e_type != ELF_ETYPE)
    return 0;
  if (ehdr->e_machine != ELF_MACHINE)
    return 0;
  if (ehdr->e_version != EV_CURRENT)
    return 0;
  
  shdr_base = (Elf_Shdr *)((uint8_t *)ehdr + ehdr->e_shoff);

  coff_sections_offset =
    (uint32_t *)malloc (ehdr->e_shnum * sizeof (uint32_t));
  memset (coff_sections_offset, 0, ehdr->e_shnum * sizeof(uint32_t));

  if (ehdr->e_shstrndx != SHN_UNDEF)
    shdr_str = (uint8_t*)ehdr + shdr_base[ehdr->e_shstrndx].sh_offset;
  else
    shdr_str = NULL;

  return 1;
}

int
is_text_shdr (Elf_Shdr *shdr)
{
  if (shdr->sh_type != SHT_PROGBITS) {
    return 0;
  }
#ifdef ELF2PE_IA64
  return (shdr->sh_flags & (SHF_EXECINSTR | SHF_ALLOC))
    == (SHF_ALLOC | SHF_EXECINSTR);
#else
  return (shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC;
#endif
}

int
is_data_shdr (Elf_Shdr *shdr)
{
  if (shdr->sh_type != SHT_PROGBITS && shdr->sh_type != SHT_NOBITS) {
    return 0;
  }
  return (shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}

void
create_section_header (const char *name, uint32_t offset, uint32_t size,
		       uint32_t flags)
{
  struct pe32_section_header *hdr;
  hdr = (struct pe32_section_header*)(coff_file + table_offset);

  strcpy (hdr->name, name);
  hdr->virtual_size = size;
  hdr->virtual_address = offset;
  hdr->raw_data_size = size;
  hdr->raw_data_offset = offset;
  hdr->relocations_offset = 0;
  hdr->line_numbers_offset = 0;
  hdr->num_relocations = 0;
  hdr->num_line_numbers = 0;
  hdr->characteristics = flags;

  table_offset += sizeof (struct pe32_section_header);
}

int
scan_sections (void)
{
  uint32_t i;
  struct pe32_dos_header *doshdr;
  struct pe32_nt_header *nt_hdr;
  uint32_t coff_entry = 0;
  int status = 0;

  coff_offset = 0;

  /* Coff file start with a DOS header.  */
  coff_offset = sizeof(struct pe32_dos_header);
  nt_hdr_offset = coff_offset;
  coff_offset += sizeof(struct pe32_nt_header);
  table_offset = coff_offset;
  coff_offset += coff_nbr_sections * sizeof(struct pe32_section_header);

  /* First text sections.  */
  coff_offset = coff_align(coff_offset);
  text_offset = coff_offset;
  for (i = 0; i < ehdr->e_shnum; i++) {
    Elf_Shdr *shdr = get_shdr_by_index (i);
    if (is_text_shdr (shdr)) {
      /* Relocate entry.  */
      if (ehdr->e_entry >= shdr->sh_addr
	  && ehdr->e_entry < shdr->sh_addr + shdr->sh_size) {
	coff_entry = coff_offset + ehdr->e_entry - shdr->sh_addr;
      }
      coff_sections_offset[i] = coff_offset;
      coff_offset += shdr->sh_size;
#ifdef ELF2PE_IA64
      if (coff_sections_offset[i] != shdr->sh_addr) {
	fprintf (stderr, 
		 "Section %s: Coff offset (%x) != Elf offset (%lx)",
		 shdr_str + shdr->sh_name,
		 coff_sections_offset[i],
		 shdr->sh_addr);
	status = -1;
      }
#endif
    }
    if (shdr->sh_type == SHT_DYNAMIC) {
      shdr_dynamic = shdr;
    }
  }
#ifdef ELF2PE_IA64
  /* 16 bytes are reserved (by the ld script) for the entry point descriptor.
   */
  coff_entry_descr_offset = coff_offset - 16;
#endif

  coff_offset = coff_align (coff_offset);
		       
  /* Then data sections.  */
  data_offset = coff_offset;
  for (i = 0; i < ehdr->e_shnum; i++) {
    Elf_Shdr *shdr = get_shdr_by_index (i);
    if (is_data_shdr (shdr)) {
      coff_sections_offset[i] = coff_offset;
      coff_offset += shdr->sh_size;
#ifdef ELF2PE_IA64
      if (coff_sections_offset[i] != shdr->sh_addr) {
	fprintf (stderr,
		 "Section %s: Coff offset (%x) != Elf offset (%lx)",
		 shdr_str + shdr->sh_name,
		 coff_sections_offset[i],
		 shdr->sh_addr);
	status = -1;
      }
#endif
    }
  }
  coff_offset = coff_align (coff_offset);

  reloc_offset = coff_offset;  

  /* Allocate base Coff file.  Will be expanded later for relocations.  */
  coff_file = (uint8_t *)malloc (coff_offset);
  memset (coff_file, 0, coff_offset);

  /* Fill headers.  */
  doshdr = (struct pe32_dos_header *)coff_file;
  doshdr->magic = 0x5A4D;
  doshdr->new_hdr_offset = nt_hdr_offset;

  nt_hdr = (struct pe32_nt_header*)(coff_file + nt_hdr_offset);

  memcpy (nt_hdr->signature, "PE\0", 4);

  nt_hdr->coff_header.machine = EFI_MACHINE;
  nt_hdr->coff_header.num_sections = coff_nbr_sections;
  nt_hdr->coff_header.time = time (NULL);
  nt_hdr->coff_header.symtab_offset = 0;
  nt_hdr->coff_header.num_symbols = 0;
  nt_hdr->coff_header.optional_header_size = sizeof(nt_hdr->optional_header);
  nt_hdr->coff_header.characteristics = PE32_EXECUTABLE_IMAGE
    | PE32_LINE_NUMS_STRIPPED
    | PE32_LOCAL_SYMS_STRIPPED
    | PE32_32BIT_MACHINE;

#ifdef USE_PE32
  nt_hdr->optional_header.magic = PE32_PE32_MAGIC;
#else
  nt_hdr->optional_header.magic = PE32_PE64_MAGIC;
#endif
  nt_hdr->optional_header.code_size = data_offset - text_offset;
  nt_hdr->optional_header.data_size = reloc_offset - data_offset;
  nt_hdr->optional_header.bss_size = 0;
#ifdef ELF2PE_IA64
  nt_hdr->optional_header.entry_addr = coff_entry_descr_offset;
  coff_entry_descr_func = coff_entry;
#else
  nt_hdr->optional_header.entry_addr = coff_entry;
#endif
  nt_hdr->optional_header.code_base = text_offset;

#ifdef USE_PE32
  nt_hdr->optional_header.data_base = data_offset;
#endif
  nt_hdr->optional_header.image_base = 0;
  nt_hdr->optional_header.section_alignment = coff_alignment;
  nt_hdr->optional_header.file_alignment = coff_alignment;
  nt_hdr->optional_header.image_size = 0;

  nt_hdr->optional_header.header_size = text_offset;
  nt_hdr->optional_header.num_data_directories = PE32_NUM_DATA_DIRECTORIES;

  /* Section headers.  */
  create_section_header (".text", text_offset, data_offset - text_offset,
			 PE32_SCN_CNT_CODE
			 | PE32_SCN_MEM_EXECUTE
			 | PE32_SCN_MEM_READ);
  create_section_header (".data", data_offset, reloc_offset - data_offset,
			 PE32_SCN_CNT_INITIALIZED_DATA
			 | PE32_SCN_MEM_WRITE
			 | PE32_SCN_MEM_READ);
#ifdef ELF2PE_IA64
  if (shdr_dynamic != NULL)
    {
      Elf64_Dyn *dyn = (Elf64_Dyn*)((uint8_t*)ehdr + shdr_dynamic->sh_offset);
      while (dyn->d_tag != DT_NULL)
	{
	  if (dyn->d_tag == DT_PLTGOT)
	    plt_base = dyn->d_un.d_ptr;
	  dyn++;
	}
    }
#endif
  return status;
}

int
write_sections (int (*filter)(Elf_Shdr *))
{
  uint32_t idx;
  int status = 0;

  /* First: copy sections.  */
  for (idx = 0; idx < ehdr->e_shnum; idx++)
    {
      Elf_Shdr *shdr = get_shdr_by_index (idx);
      if ((*filter)(shdr))
	{
	  switch (shdr->sh_type) {
	  case SHT_PROGBITS:
	    /* Copy.  */
	    memcpy (coff_file + coff_sections_offset[idx],
		    (uint8_t*)ehdr + shdr->sh_offset,
		    shdr->sh_size);
	    break;
	  case SHT_NOBITS:
	    memset (coff_file + coff_sections_offset[idx], 0, shdr->sh_size);
	    break;
	  case SHT_DYNAMIC:
	    break;
	  default:
	    fprintf (stderr, "unhandled section type %x",
		     (unsigned int)shdr->sh_type);
	    status = -1;
	  }
	}
    }

  /* Second: apply relocations.  */
  for (idx = 0; idx < ehdr->e_shnum; idx++)
    {
      Elf_Shdr *rel_shdr = get_shdr_by_index (idx);
      if (rel_shdr->sh_type != SHT_REL && rel_shdr->sh_type != SHT_RELA)
	continue;
      Elf_Shdr *sec_shdr = get_shdr_by_index (rel_shdr->sh_info);
      uint32_t sec_offset = coff_sections_offset[rel_shdr->sh_info];

      if (rel_shdr->sh_info == 0 || (*filter)(sec_shdr))
	{
	  uint32_t rel_idx;
	  Elf_Shdr *symtab_shdr = get_shdr_by_index(rel_shdr->sh_link);
	  uint8_t *symtab = (uint8_t*)ehdr + symtab_shdr->sh_offset;

	  if (rel_shdr->sh_type == SHT_REL)
	    {
	      for (rel_idx = 0;
		   rel_idx < rel_shdr->sh_size;
		   rel_idx += rel_shdr->sh_entsize)
		{
		  Elf_Rel *rel = (Elf_Rel *)
		    ((uint8_t*)ehdr + rel_shdr->sh_offset + rel_idx);
		  Elf_Sym *sym = (Elf_Sym *)
		    (symtab
		     + ELF_R_SYM(rel->r_info) * symtab_shdr->sh_entsize);
		  Elf_Shdr *sym_shdr;
		  uint8_t *targ;

		  if (sym->st_shndx == SHN_UNDEF
		      || sym->st_shndx == SHN_ABS
		      || sym->st_shndx > ehdr->e_shnum)
		    {
		      fprintf (stderr, "bad symbol definition");
		      status = -1;
		    }
		  sym_shdr = get_shdr_by_index(sym->st_shndx);

		  /* Note: r_offset in a memory address.
		     Convert it to a pointer in the coff file.  */
		  targ = coff_file + sec_offset
		    + (rel->r_offset - sec_shdr->sh_addr);

		  switch (ELF_R_TYPE(rel->r_info)) {
		  case R_386_NONE:
		    break;
		  case R_386_32:
		    /* Absolute relocation.  */
		    *(uint32_t *)targ = *(uint32_t *)targ - sym_shdr->sh_addr
		      + coff_sections_offset[sym->st_shndx];
		    break;
		  case R_386_PC32:
		    /* Relative relocation: Symbol - Ip + Addend  */
		    *(uint32_t *)targ = *(uint32_t *)targ
		      + (coff_sections_offset[sym->st_shndx]
			 - sym_shdr->sh_addr)
		      - (sec_offset - sec_shdr->sh_addr);
		    break;
		  default:
		    fprintf (stderr, "unhandled relocation type %lx",
			     ELF_R_TYPE(rel->r_info));
		    status = -1;
		  }
		}
	    }
	  else if (rel_shdr->sh_type == SHT_RELA)
	    {
	      for (rel_idx = 0;
		   rel_idx < rel_shdr->sh_size;
		   rel_idx += rel_shdr->sh_entsize) {
		Elf_Rela *rela = (Elf_Rela *)
		  ((uint8_t*)ehdr + rel_shdr->sh_offset + rel_idx);
		Elf_Sym *sym = (Elf_Sym *)
		  (symtab + ELF_R_SYM(rela->r_info) * symtab_shdr->sh_entsize);
		Elf_Shdr *sym_shdr;
		uint8_t *targ;
		
		if (ELF_R_TYPE(rela->r_info) == R_IA64_NONE)
		  continue;
		
#if 0
		if (sym->st_shndx == SHN_UNDEF
		    || sym->st_shndx == SHN_ABS
		    || sym->st_shndx > ehdr->e_shnum) {
		  fprintf (stderr, "bad symbol definition %d",
			   ELF_R_SYM(rela->r_info));
		}
#endif
		sym_shdr = get_shdr_by_index (sym->st_shndx);
		
		/* Note: r_offset in a memory address.
		   Convert it to a pointer in the coff file.  */
		targ = coff_file + sec_offset
		  + (rela->r_offset - sec_shdr->sh_addr);
		
		switch (ELF_R_TYPE(rela->r_info)) {
		case R_IA64_IPLTLSB:
		  /* If there is a descriptor with the same function
		     pointer as the ELF entry point, use that
		     descriptor for the PE/Coff entry.  */
		  if (*(uint64_t*)targ == ehdr->e_entry) {
		    struct pe32_nt_header *nt_hdr;
		    
		    nt_hdr = 
		      (struct pe32_nt_header*)(coff_file + nt_hdr_offset);
		    nt_hdr->optional_header.entry_addr = targ - coff_file;
		  }
		  break;
		case R_IA64_REL64LSB:
		case R_IA64_NONE:
		  break;
		default:
		  fprintf (stderr,
			   "unhandled relocation type %lx in section %d",
			   ELF_R_TYPE(rela->r_info), rel_shdr->sh_info);
		  status = -1;
		}
	      }
	    }
	}
    }
  return status;
}

void
coff_add_fixup_entry (uint16_t val)
{
  *coff_entry_rel = val;
  coff_entry_rel++;
  coff_base_rel->block_size += 2;
  coff_offset += 2;
}

void
coff_add_fixup (uint32_t offset, uint8_t type)
{
  if (coff_base_rel == NULL
      || coff_base_rel->page_rva != (offset & ~0xfff)) {
    if (coff_base_rel != NULL) {
      /* Add a null entry (is it required ?)  */
      coff_add_fixup_entry (0);
      /* Pad for alignment.  */
      if (coff_offset % 4 != 0)
	coff_add_fixup_entry (0);
    }
      
    coff_file = realloc
      (coff_file,
       coff_offset + sizeof(struct pe32_fixup_block) + 2*0x1000);
    memset(coff_file + coff_offset, 0,
	   sizeof(struct pe32_fixup_block) + 2*0x1000);

    coff_base_rel = (struct pe32_fixup_block*)(coff_file + coff_offset);
    coff_base_rel->page_rva = offset & ~0xfff;
    coff_base_rel->block_size = sizeof(struct pe32_fixup_block);

    coff_entry_rel = (uint16_t *)(coff_base_rel + 1);
    coff_offset += sizeof(struct pe32_fixup_block);
  }

  /* Fill the entry.  */
  coff_add_fixup_entry ((type << 12) | (offset & 0xfff));
}

int
write_relocations(void)
{
  uint32_t idx;
  struct pe32_nt_header *nt_hdr;
  struct pe32_data_directory *dir;
  int status = 0;

  for (idx = 0; idx < ehdr->e_shnum; idx++)
    {
      Elf_Shdr *rel_shdr = get_shdr_by_index (idx);
      if (rel_shdr->sh_type == SHT_REL)
	{
	  Elf_Shdr *sec_shdr = get_shdr_by_index (rel_shdr->sh_info);
	  if (is_text_shdr(sec_shdr) || is_data_shdr(sec_shdr))
	    {
	      uint32_t rel_idx;
	      for (rel_idx = 0;
		   rel_idx < rel_shdr->sh_size;
		   rel_idx += rel_shdr->sh_entsize)
		{
		  Elf_Rel *rel = (Elf_Rel *)
		    ((uint8_t*)ehdr + rel_shdr->sh_offset + rel_idx);

		  switch (ELF_R_TYPE(rel->r_info))
		    {
		    case R_386_NONE:
		    case R_386_PC32:
		      break;
		    case R_386_32:
		      coff_add_fixup(coff_sections_offset[rel_shdr->sh_info]
				     + (rel->r_offset - sec_shdr->sh_addr),
				     PE32_REL_BASED_HIGHLOW);
		      break;
		    default:
		      fprintf (stderr, "unhandled relocation type %lx",
			       ELF_R_TYPE(rel->r_info));
		      status = -1;
		    }
		}
	    }
	}
      else if (rel_shdr->sh_type == SHT_RELA)
	{
	  Elf_Shdr *sec_shdr = get_shdr_by_index(rel_shdr->sh_info);
	  if (rel_shdr->sh_info == 0
	      || is_text_shdr(sec_shdr) || is_data_shdr(sec_shdr))
	    {
	      uint32_t rel_idx;
	      for (rel_idx = 0;
		   rel_idx < rel_shdr->sh_size;
		   rel_idx += rel_shdr->sh_entsize) {
		Elf_Rela *rela = (Elf_Rela *)
		  ((uint8_t*)ehdr + rel_shdr->sh_offset + rel_idx);
		uint32_t Loc = coff_sections_offset[rel_shdr->sh_info]
		  + (rela->r_offset - sec_shdr->sh_addr);

		switch (ELF_R_TYPE(rela->r_info))
		  {
		  case R_IA64_IPLTLSB:
		    coff_add_fixup(Loc, PE32_REL_BASED_DIR64);
		    coff_add_fixup(Loc + 8, PE32_REL_BASED_DIR64);
		    break;
		  case R_IA64_REL64LSB:
		    coff_add_fixup(Loc, PE32_REL_BASED_DIR64);
		    break;
		  case R_IA64_DIR64LSB:
		    coff_add_fixup(Loc, PE32_REL_BASED_DIR64);
		    break;
		  case R_IA64_IMM64:
		    coff_add_fixup(Loc, PE32_REL_BASED_IA64_IMM64);
		    break;
		  case R_IA64_PCREL21B:
		  case R_IA64_PCREL64LSB:
		  case R_IA64_SECREL32LSB:
		  case R_IA64_SEGREL64LSB:
		    break;
		  case R_IA64_GPREL22:
		  case R_IA64_LTOFF22X:
		  case R_IA64_LDXMOV:
		  case R_IA64_LTOFF_FPTR22:
		  case R_IA64_NONE:
		    break;
		  default:
		    fprintf (stderr, "unhandled relocation type %lx",
			     ELF_R_TYPE(rela->r_info));
		    status = -1;
		  }
	      }
	    }
	}
    }
  
#ifdef ELF2PE_IA64
  coff_add_fixup (coff_entry_descr_offset, PE32_REL_BASED_DIR64);
  coff_add_fixup (coff_entry_descr_offset + 8, PE32_REL_BASED_DIR64);
#endif

  /* Pad by adding empty entries.   */
  while (coff_offset & (coff_alignment - 1))
    coff_add_fixup_entry (0);

  create_section_header (".reloc", reloc_offset, coff_offset - reloc_offset,
			 PE32_SCN_CNT_INITIALIZED_DATA
			 | PE32_SCN_MEM_DISCARDABLE
			 | PE32_SCN_MEM_READ);
  
  nt_hdr = (struct pe32_nt_header *)(coff_file + nt_hdr_offset);
  dir = &nt_hdr->optional_header.base_relocation_table;
  dir->rva = reloc_offset;
  dir->size = coff_offset - reloc_offset;
  
  return status;
}

void
write_debug(void)
{
  uint32_t len = strlen(filename) + 1;
  uint32_t debug_offset = coff_offset;
  struct pe32_nt_header *nt_hdr;
  struct pe32_data_directory *data_dir;
  struct pe32_debug_directory_entry *dir;
  struct pe32_debug_codeview_nb10_entry *nb10;

  coff_offset += sizeof (struct pe32_debug_directory_entry)
    + sizeof(struct pe32_debug_codeview_nb10_entry)
    + len;
  coff_offset = coff_align(coff_offset);

  coff_file = realloc
    (coff_file, coff_offset);
  memset(coff_file + debug_offset, 0, coff_offset - debug_offset);
  
  dir = (struct pe32_debug_directory_entry*)(coff_file + debug_offset);
  dir->type = PE32_DEBUG_TYPE_CODEVIEW;
  dir->data_size = sizeof(struct pe32_debug_directory_entry) + len;
  dir->rva = debug_offset + sizeof(struct pe32_debug_directory_entry);
  dir->file_offset = debug_offset + sizeof(struct pe32_debug_directory_entry);
  
  nb10 = (struct pe32_debug_codeview_nb10_entry*)(dir + 1);
  nb10->signature = PE32_CODEVIEW_SIGNATURE_NB10;
  strcpy (nb10->filename, filename);

  create_section_header (".debug", debug_offset, coff_offset - debug_offset,
		       PE32_SCN_CNT_INITIALIZED_DATA
		       | PE32_SCN_MEM_DISCARDABLE
		       | PE32_SCN_MEM_READ);

  nt_hdr = (struct pe32_nt_header *)(coff_file + nt_hdr_offset);
  data_dir = &nt_hdr->optional_header.debug;
  data_dir->rva = debug_offset;
  data_dir->size = coff_offset - debug_offset;
}

int
convert_elf (uint8_t **file_buffer, unsigned int *file_length)
{
  struct pe32_nt_header *nt_hdr;

  /* Check header, read section table.  */
  ehdr = (Elf_Ehdr*)*file_buffer;
  if (!check_elf_header ())
    return -1;

  /* Compute sections new address.  */
  if (scan_sections () != 0)
    return -2;

  /* Write and relocate sections.  */
  if (write_sections (is_text_shdr) != 0)
    return -3;

#ifdef ELF2PE_IA64
  *(uint64_t*)(coff_file + coff_entry_descr_offset) = coff_entry_descr_func;
  *(uint64_t*)(coff_file + coff_entry_descr_offset + 8) = plt_base;
#endif

  if (write_sections (is_data_shdr) != 0)
    return -4;

  /* Translate and write relocations.  */
  if (write_relocations () != 0)
    return -5;

  /* Write debug info.  */
  write_debug ();

  nt_hdr = (struct pe32_nt_header *)(coff_file + nt_hdr_offset);
  nt_hdr->optional_header.image_size = coff_offset;

  nt_hdr->optional_header.subsystem = PE32_SUBSYSTEM_EFI_APPLICATION;

  /* Replace.  */
  free (*file_buffer);
  *file_buffer = coff_file;
  *file_length = coff_offset;

  return 0;
}

int
main (int argc, char **argv)
{
  FILE *f;
  unsigned int size;
  uint8_t *buffer;
  const char *outfile;
  int status;

  if (argc != 3)
    {
      fprintf (stderr, "usage: %s elf-file pe-file\n", argv[0]);
      exit (1);
    }

  filename = argv[1];
  outfile = argv[2];
  f = fopen (filename, "rb");
  fseek (f, 0, SEEK_END);
  size = ftell (f);
  fseek (f, 0, SEEK_SET);

  buffer = malloc (size);
  if (buffer == NULL)
    {
      fprintf (stderr, "cannot allocate %u bytes of memory\n", size);
      exit (2);
    }
  if (fread (buffer, size, 1, f) != 1)
    {
      fprintf (stderr, "cannot read %s\n", filename);
      exit (2);
    }
  fclose (f);

  if (!is_elf_header (buffer))
    {
      fprintf (stderr, "%s is not an elf file\n", filename);
      exit (2);
    }
  
  status = convert_elf (&buffer, &size);
  if (status != 0)
    {
      fprintf (stderr, "cannot convert %s to pe (err=%d)\n", filename, status);
      exit (2);
    }
  
  f = fopen (outfile, "wb");
  if (f == NULL)
    {
      fprintf (stderr, "cannot open %s\n", outfile);
      exit (2);
    }
  if (fwrite (buffer, size, 1, f) != 1)
    {
      fprintf (stderr, "cannot write to %s\n", outfile);
      exit (2);
    }
  fclose (f);

  return 0;
}
