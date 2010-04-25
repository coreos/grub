/* grub-mkimage.c - make a bootable image */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/types.h>
#include <grub/machine/boot.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/i18n.h>
#include <grub/kernel.h>
#include <grub/disk.h>
#include <grub/util/misc.h>
#include <grub/util/resolve.h>
#include <grub/misc.h>
#include <time.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <grub/efi/pe32.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#include "progname.h"

#define ALIGN_ADDR(x) (ALIGN_UP((x), GRUB_TARGET_SIZEOF_VOID_P))

#ifdef GRUB_MACHINE_EFI
#define SECTION_ALIGN GRUB_PE32_SECTION_ALIGNMENT
#define VADDR_OFFSET ALIGN_UP (sizeof (struct grub_pe32_header) + 5 * sizeof (struct grub_pe32_section_table), SECTION_ALIGN)
#else
#define SECTION_ALIGN 1
#define VADDR_OFFSET 0
#endif

#if GRUB_TARGET_WORDSIZE == 32
# define grub_target_to_host(val) grub_target_to_host32(val)
#elif GRUB_TARGET_WORDSIZE == 64
# define grub_target_to_host(val) grub_target_to_host64(val)
#endif

#ifdef ENABLE_LZMA
#include <grub/lib/LzmaEnc.h>

static void *SzAlloc(void *p, size_t size) { p = p; return xmalloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static void
compress_kernel (char *kernel_img, size_t kernel_size,
		 char **core_img, size_t *core_size)
{
  CLzmaEncProps props;
  unsigned char out_props[5];
  size_t out_props_size = 5;

  LzmaEncProps_Init(&props);
  props.dictSize = 1 << 16;
  props.lc = 3;
  props.lp = 0;
  props.pb = 2;
  props.numThreads = 1;

  if (kernel_size < GRUB_KERNEL_MACHINE_RAW_SIZE)
    grub_util_error (_("the core image is too small"));

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, GRUB_KERNEL_MACHINE_RAW_SIZE);

  *core_size = kernel_size - GRUB_KERNEL_MACHINE_RAW_SIZE;
  if (LzmaEncode((unsigned char *) *core_img + GRUB_KERNEL_MACHINE_RAW_SIZE,
                 core_size,
                 (unsigned char *) kernel_img + GRUB_KERNEL_MACHINE_RAW_SIZE,
                 kernel_size - GRUB_KERNEL_MACHINE_RAW_SIZE,
                 &props, out_props, &out_props_size,
                 0, NULL, &g_Alloc, &g_Alloc) != SZ_OK)
    grub_util_error (_("cannot compress the kernel image"));

  *core_size += GRUB_KERNEL_MACHINE_RAW_SIZE;
}

#else	/* No lzma compression */

static void
compress_kernel (char *kernel_img, size_t kernel_size,
               char **core_img, size_t *core_size)
{
  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, kernel_size);
  *core_size = kernel_size;
}

#endif	/* No lzma compression */

#ifdef GRUB_MACHINE_EFI

/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf_Addr
relocate_symbols (Elf_Ehdr *e, Elf_Shdr *sections,
		  Elf_Shdr *symtab_section, Elf_Addr *section_addresses,
		  Elf_Half section_entsize, Elf_Half num_sections)
{
  Elf_Word symtab_size, sym_size, num_syms;
  Elf_Off symtab_offset;
  Elf_Addr start_address = 0;
  Elf_Sym *sym;
  Elf_Word i;
  Elf_Shdr *strtab_section;
  const char *strtab;

  strtab_section
    = (Elf_Shdr *) ((char *) sections
		      + (grub_target_to_host32 (symtab_section->sh_link)
			 * section_entsize));
  strtab = (char *) e + grub_target_to_host32 (strtab_section->sh_offset);

  symtab_size = grub_target_to_host32 (symtab_section->sh_size);
  sym_size = grub_target_to_host32 (symtab_section->sh_entsize);
  symtab_offset = grub_target_to_host32 (symtab_section->sh_offset);
  num_syms = symtab_size / sym_size;

  for (i = 0, sym = (Elf_Sym *) ((char *) e + symtab_offset);
       i < num_syms;
       i++, sym = (Elf_Sym *) ((char *) sym + sym_size))
    {
      Elf_Section index;
      const char *name;

      name = strtab + grub_target_to_host32 (sym->st_name);

      index = grub_target_to_host16 (sym->st_shndx);
      if (index == STN_ABS)
        {
          continue;
        }
      else if ((index == STN_UNDEF))
	{
	  if (sym->st_name)
	    grub_util_error ("undefined symbol %s", name);
	  else
	    continue;
	}
      else if (index >= num_sections)
	grub_util_error ("section %d does not exist", index);

      sym->st_value = (grub_target_to_host32 (sym->st_value)
		       + section_addresses[index]);
      grub_util_info ("locating %s at 0x%x", name, sym->st_value);

      if (! start_address)
	if (strcmp (name, "_start") == 0 || strcmp (name, "start") == 0)
	  start_address = sym->st_value;
    }

  return start_address;
}

/* Return the address of a symbol at the index I in the section S.  */
static Elf_Addr
get_symbol_address (Elf_Ehdr *e, Elf_Shdr *s, Elf_Word i)
{
  Elf_Sym *sym;

  sym = (Elf_Sym *) ((char *) e
		       + grub_target_to_host32 (s->sh_offset)
		       + i * grub_target_to_host32 (s->sh_entsize));
  return sym->st_value;
}

/* Return the address of a modified value.  */
static Elf_Addr *
get_target_address (Elf_Ehdr *e, Elf_Shdr *s, Elf_Addr offset)
{
  return (Elf_Addr *) ((char *) e + grub_target_to_host32 (s->sh_offset) + offset);
}

/* Deal with relocation information. This function relocates addresses
   within the virtual address space starting from 0. So only relative
   addresses can be fully resolved. Absolute addresses must be relocated
   again by a PE32 relocator when loaded.  */
static void
relocate_addresses (Elf_Ehdr *e, Elf_Shdr *sections,
		    Elf_Addr *section_addresses,
		    Elf_Half section_entsize, Elf_Half num_sections,
		    const char *strtab)
{
  Elf_Half i;
  Elf_Shdr *s;

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((s->sh_type == grub_host_to_target32 (SHT_REL)) ||
        (s->sh_type == grub_host_to_target32 (SHT_RELA)))
      {
	Elf_Rela *r;
	Elf_Word rtab_size, r_size, num_rs;
	Elf_Off rtab_offset;
	Elf_Shdr *symtab_section;
	Elf_Word target_section_index;
	Elf_Addr target_section_addr;
	Elf_Shdr *target_section;
	Elf_Word j;

	symtab_section = (Elf_Shdr *) ((char *) sections
					 + (grub_target_to_host32 (s->sh_link)
					    * section_entsize));
	target_section_index = grub_target_to_host32 (s->sh_info);
	target_section_addr = section_addresses[target_section_index];
	target_section = (Elf_Shdr *) ((char *) sections
					 + (target_section_index
					    * section_entsize));

	grub_util_info ("dealing with the relocation section %s for %s",
			strtab + grub_target_to_host32 (s->sh_name),
			strtab + grub_target_to_host32 (target_section->sh_name));

	rtab_size = grub_target_to_host32 (s->sh_size);
	r_size = grub_target_to_host32 (s->sh_entsize);
	rtab_offset = grub_target_to_host32 (s->sh_offset);
	num_rs = rtab_size / r_size;

	for (j = 0, r = (Elf_Rela *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rela *) ((char *) r + r_size))
	  {
            Elf_Addr info;
	    Elf_Addr offset;
	    Elf_Addr sym_addr;
	    Elf_Addr *target;
	    Elf_Addr addend;

	    offset = grub_target_to_host (r->r_offset);
	    target = get_target_address (e, target_section, offset);
	    info = grub_target_to_host (r->r_info);
	    sym_addr = get_symbol_address (e, symtab_section,
					   ELF_R_SYM (info));

            addend = (s->sh_type == grub_target_to_host32 (SHT_RELA)) ?
	      r->r_addend : 0;

            switch (ELF_R_TYPE (info))
	      {
#if GRUB_TARGET_SIZEOF_VOID_P == 4
	      case R_386_NONE:
		break;

	      case R_386_32:
		/* This is absolute.  */
		*target = grub_host_to_target32 (grub_target_to_host32 (*target)
                                            + addend + sym_addr);
		grub_util_info ("relocating an R_386_32 entry to 0x%x at the offset 0x%x",
				*target, offset);
		break;

	      case R_386_PC32:
		/* This is relative.  */
		*target = grub_host_to_target32 (grub_target_to_host32 (*target)
						 + addend + sym_addr
						 - target_section_addr - offset
						 - VADDR_OFFSET);
		grub_util_info ("relocating an R_386_PC32 entry to 0x%x at the offset 0x%x",
				*target, offset);
		break;

#else

              case R_X86_64_NONE:
                break;

              case R_X86_64_64:
		*target = grub_host_to_target64 (grub_target_to_host64 (*target)
					    + addend + sym_addr);
		grub_util_info ("relocating an R_X86_64_64 entry to 0x%llx at the offset 0x%llx",
				*target, offset);
		break;

              case R_X86_64_PC32:
                {
                  grub_uint32_t *t32 = (grub_uint32_t *) target;
                  *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						+ addend + sym_addr
						- target_section_addr - offset
						- VADDR_OFFSET);
                  grub_util_info ("relocating an R_X86_64_PC32 entry to 0x%x at the offset 0x%llx",
                                  *t32, offset);
                  break;
                }

              case R_X86_64_32:
              case R_X86_64_32S:
                {
                  grub_uint32_t *t32 = (grub_uint32_t *) target;
                  *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
                                           + addend + sym_addr);
                  grub_util_info ("relocating an R_X86_64_32(S) entry to 0x%x at the offset 0x%llx",
                                  *t32, offset);
                  break;
                }

#endif
	      default:
		grub_util_error ("unknown relocation type %d",
				 ELF_R_TYPE (info));
		break;
	      }
	  }
      }
}

struct fixup_block_list
{
  struct fixup_block_list *next;
  int state;
  struct grub_pe32_fixup_block b;
};

/* Add a PE32's fixup entry for a relocation. Return the resulting address
   after having written to the file OUT.  */
Elf_Addr
add_fixup_entry (struct fixup_block_list **cblock, grub_uint16_t type,
		 Elf_Addr addr, int flush, Elf_Addr current_address)
{
  struct grub_pe32_fixup_block *b;

  b = &((*cblock)->b);

  /* First, check if it is necessary to write out the current block.  */
  if ((*cblock)->state)
    {
      if (flush || addr < b->page_rva || b->page_rva + 0x1000 <= addr)
	{
	  grub_uint32_t size;

	  if (flush)
	    {
	      /* Add as much padding as necessary to align the address
		 with a section boundary.  */
	      Elf_Addr next_address;
	      unsigned padding_size;
              size_t index;

	      next_address = current_address + b->block_size;
	      padding_size = ((ALIGN_UP (next_address, SECTION_ALIGN)
			       - next_address)
			      >> 1);
              index = ((b->block_size - sizeof (*b)) >> 1);
              grub_util_info ("adding %d padding fixup entries", padding_size);
	      while (padding_size--)
		{
		  b->entries[index++] = 0;
		  b->block_size += 2;
		}
	    }
          else if (b->block_size & (8 - 1))
            {
	      /* If not aligned with a 32-bit boundary, add
		 a padding entry.  */
              size_t index;

              grub_util_info ("adding a padding fixup entry");
              index = ((b->block_size - sizeof (*b)) >> 1);
              b->entries[index] = 0;
              b->block_size += 2;
            }

          /* Flush it.  */
          grub_util_info ("writing %d bytes of a fixup block starting at 0x%x",
                          b->block_size, b->page_rva);
          size = b->block_size;
	  current_address += size;
	  b->page_rva = grub_host_to_target32 (b->page_rva);
	  b->block_size = grub_host_to_target32 (b->block_size);
	  (*cblock)->next = xmalloc (sizeof (**cblock) + 2 * 0x1000);
	  memset ((*cblock)->next, 0, sizeof (**cblock) + 2 * 0x1000);
	  *cblock = (*cblock)->next;
	}
    }

  b = &((*cblock)->b);

  if (! flush)
    {
      grub_uint16_t entry;
      size_t index;

      /* If not allocated yet, allocate a block with enough entries.  */
      if (! (*cblock)->state)
	{
	  (*cblock)->state = 1;

	  /* The spec does not mention the requirement of a Page RVA.
	     Here, align the address with a 4K boundary for safety.  */
	  b->page_rva = (addr & ~(0x1000 - 1));
	  b->block_size = sizeof (*b);
	}

      /* Sanity check.  */
      if (b->block_size >= sizeof (*b) + 2 * 0x1000)
	grub_util_error ("too many fixup entries");

      /* Add a new entry.  */
      index = ((b->block_size - sizeof (*b)) >> 1);
      entry = GRUB_PE32_FIXUP_ENTRY (type, addr - b->page_rva);
      b->entries[index] = grub_host_to_target16 (entry);
      b->block_size += 2;
    }

  return current_address;
}

/* Make a .reloc section.  */
static Elf_Addr
make_reloc_section (Elf_Ehdr *e, void **out,
		    Elf_Addr *section_addresses, Elf_Shdr *sections,
		    Elf_Half section_entsize, Elf_Half num_sections,
		    const char *strtab)
{
  Elf_Half i;
  Elf_Shdr *s;
  struct fixup_block_list *lst, *lst0;
  Elf_Addr current_address = 0;

  lst = lst0 = xmalloc (sizeof (*lst) + 2 * 0x1000);
  memset (lst, 0, sizeof (*lst) + 2 * 0x1000);

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((s->sh_type == grub_cpu_to_le32 (SHT_REL)) ||
        (s->sh_type == grub_cpu_to_le32 (SHT_RELA)))
      {
	Elf_Rel *r;
	Elf_Word rtab_size, r_size, num_rs;
	Elf_Off rtab_offset;
	Elf_Addr section_address;
	Elf_Word j;

	grub_util_info ("translating the relocation section %s",
			strtab + grub_le_to_cpu32 (s->sh_name));

	rtab_size = grub_le_to_cpu32 (s->sh_size);
	r_size = grub_le_to_cpu32 (s->sh_entsize);
	rtab_offset = grub_le_to_cpu32 (s->sh_offset);
	num_rs = rtab_size / r_size;

	section_address = section_addresses[grub_le_to_cpu32 (s->sh_info)];

	for (j = 0, r = (Elf_Rel *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rel *) ((char *) r + r_size))
	  {
	    Elf_Addr info;
	    Elf_Addr offset;

	    offset = grub_le_to_cpu32 (r->r_offset);
	    info = grub_le_to_cpu32 (r->r_info);

	    /* Necessary to relocate only absolute addresses.  */
#if GRUB_TARGET_SIZEOF_VOID_P == 4
	    if (ELF_R_TYPE (info) == R_386_32)
	      {
		Elf_Addr addr;

		addr = section_address + offset;
		grub_util_info ("adding a relocation entry for 0x%x", addr);
		current_address = add_fixup_entry (&lst,
						   GRUB_PE32_REL_BASED_HIGHLOW,
						   addr, 0, current_address);
	      }
#else
	    if ((ELF_R_TYPE (info) == R_X86_64_32) ||
                (ELF_R_TYPE (info) == R_X86_64_32S))
	      {
		grub_util_error ("can\'t add fixup entry for R_X86_64_32(S)");
	      }
	    else if (ELF_R_TYPE (info) == R_X86_64_64)
	      {
		Elf_Addr addr;

		addr = section_address + offset;
		grub_util_info ("adding a relocation entry for 0x%llx", addr);
		current_address = add_fixup_entry (&lst,
						   GRUB_PE32_REL_BASED_DIR64,
						   addr,
						   0, current_address);
	      }
#endif
	  }
      }

  current_address = add_fixup_entry (&lst, 0, 0, 1, current_address);

  {
    grub_uint8_t *ptr;
    ptr = *out = xmalloc (current_address);
    for (lst = lst0; lst; lst = lst->next)
      if (lst->state)
	{
	  memcpy (ptr, &lst->b, grub_target_to_host32 (lst->b.block_size));
	  ptr += grub_target_to_host32 (lst->b.block_size);
	}
    if (current_address + *out != ptr)
      {
	grub_util_error ("Bug detected %d != %d\n", ptr - (grub_uint8_t *) *out,
			 current_address);
      }
  }

  return current_address;
}

#endif

/* Determine if this section is a text section. Return false if this
   section is not allocated.  */
static int
is_text_section (Elf_Shdr *s)
{
#ifndef GRUB_MACHINE_EFI
  if (grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
#endif
  return ((grub_target_to_host32 (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == (SHF_EXECINSTR | SHF_ALLOC));
}

/* Determine if this section is a data section. This assumes that
   BSS is also a data section, since the converter initializes BSS
   when producing PE32 to avoid a bug in EFI implementations.  */
static int
is_data_section (Elf_Shdr *s)
{
#ifndef GRUB_MACHINE_EFI
  if (grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
#endif
  return ((grub_target_to_host32 (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == SHF_ALLOC);
}

/* Locate section addresses by merging code sections and data sections
   into .text and .data, respectively. Return the array of section
   addresses.  */
static Elf_Addr *
locate_sections (Elf_Shdr *sections, Elf_Half section_entsize,
		 Elf_Half num_sections, const char *strtab,
		 grub_size_t *exec_size, grub_size_t *kernel_sz)
{
  int i;
  Elf_Addr current_address;
  Elf_Addr *section_addresses;
  Elf_Shdr *s;

  section_addresses = xmalloc (sizeof (*section_addresses) * num_sections);
  memset (section_addresses, 0, sizeof (*section_addresses) * num_sections);

  current_address = 0;

  /* .text */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (is_text_section (s))
      {
	Elf_Word align = grub_host_to_target32 (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);

	if (align)
	  current_address = ALIGN_UP (current_address + VADDR_OFFSET, align)
	    - VADDR_OFFSET;

	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_host_to_target32 (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + VADDR_OFFSET, SECTION_ALIGN)
    - VADDR_OFFSET;
  *exec_size = current_address;

  /* .data */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s))
      {
	Elf_Word align = grub_host_to_target32 (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);

	if (align)
	  current_address = ALIGN_UP (current_address + VADDR_OFFSET, align)
	    - VADDR_OFFSET;

	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_host_to_target32 (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + VADDR_OFFSET, SECTION_ALIGN)
    - VADDR_OFFSET;
  *kernel_sz = current_address;
  return section_addresses;
}

/* Return if the ELF header is valid.  */
static int
check_elf_header (Elf_Ehdr *e, size_t size)
{
  if (size < sizeof (*e)
      || e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != grub_host_to_target32 (EV_CURRENT))
    return 0;

  return 1;
}

static char *
load_image (const char *kernel_path, grub_size_t *exec_size, 
	    grub_size_t *kernel_sz, grub_size_t *bss_size,
	    grub_size_t total_module_size, Elf_Addr *start,
	    void **reloc_section, grub_size_t *reloc_size)
{
  char *kernel_img, *out_img;
  const char *strtab;
  Elf_Ehdr *e;
  Elf_Shdr *sections;
  Elf_Addr *section_addresses;
  Elf_Addr *section_vaddresses;
  int i;
  Elf_Shdr *s;
  Elf_Half num_sections;
  Elf_Off section_offset;
  Elf_Half section_entsize;
  grub_size_t kernel_size;
  Elf_Shdr *symtab_section;

  *start = 0;

  kernel_size = grub_util_get_image_size (kernel_path);
  kernel_img = xmalloc (kernel_size);
  grub_util_load_image (kernel_path, kernel_img);

  e = (Elf_Ehdr *) kernel_img;
  if (! check_elf_header (e, kernel_size))
    grub_util_error ("invalid ELF header");

  section_offset = grub_target_to_host32 (e->e_shoff);
  section_entsize = grub_target_to_host16 (e->e_shentsize);
  num_sections = grub_target_to_host16 (e->e_shnum);

  if (kernel_size < section_offset + section_entsize * num_sections)
    grub_util_error ("invalid ELF format");

  sections = (Elf_Shdr *) (kernel_img + section_offset);

  /* Relocate sections then symbols in the virtual address space.  */
  s = (Elf_Shdr *) ((char *) sections
		      + grub_host_to_target16 (e->e_shstrndx) * section_entsize);
  strtab = (char *) e + grub_host_to_target32 (s->sh_offset);

  section_addresses = locate_sections (sections, section_entsize,
				       num_sections, strtab,
				       exec_size, kernel_sz);

#ifdef GRUB_MACHINE_EFI
  {
    section_vaddresses = xmalloc (sizeof (*section_addresses) * num_sections);

    for (i = 0; i < num_sections; i++)
      section_vaddresses[i] = section_addresses[i] + VADDR_OFFSET;

#if 0
    {
      Elf_Addr current_address = *kernel_sz;

      for (i = 0, s = sections;
	   i < num_sections;
	   i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
	if (grub_target_to_host32 (s->sh_type) == SHT_NOBITS)
	  {
	    Elf_Word align = grub_host_to_target32 (s->sh_addralign);
	    const char *name = strtab + grub_host_to_target32 (s->sh_name);

	    if (align)
	      current_address = ALIGN_UP (current_address + VADDR_OFFSET, align)
		- VADDR_OFFSET;
	
	    grub_util_info ("locating the section %s at 0x%x",
			    name, current_address);
	    section_vaddresses[i] = current_address + VADDR_OFFSET;
	    current_address += grub_host_to_target32 (s->sh_size);
	  }
      current_address = ALIGN_UP (current_address + VADDR_OFFSET, SECTION_ALIGN)
	- VADDR_OFFSET;
      *bss_size = current_address - *kernel_sz;
    }
#else
    *bss_size = 0;
#endif

    symtab_section = NULL;
    for (i = 0, s = sections;
	 i < num_sections;
	 i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
      if (s->sh_type == grub_host_to_target32 (SHT_SYMTAB))
	{
	  symtab_section = s;
	  break;
	}

    if (! symtab_section)
      grub_util_error ("no symbol table");

    *start = relocate_symbols (e, sections, symtab_section,
			       section_vaddresses, section_entsize,
			       num_sections);
    if (*start == 0)
      grub_util_error ("start symbol is not defined");

    /* Resolve addresses in the virtual address space.  */
    relocate_addresses (e, sections, section_addresses, section_entsize,
			num_sections, strtab);

    *reloc_size = make_reloc_section (e, reloc_section,
				      section_vaddresses, sections,
				      section_entsize, num_sections,
				      strtab);

  }
#else
  *bss_size = 0;
  *reloc_size = 0;
  *reloc_section = NULL;
#endif

  out_img = xmalloc (*kernel_sz + total_module_size);

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s) || is_text_section (s))
      {
	if (grub_target_to_host32 (s->sh_type) == SHT_NOBITS)
	  memset (out_img + section_addresses[i], 0,
		  grub_host_to_target32 (s->sh_size));
	else
	  memcpy (out_img + section_addresses[i],
		  kernel_img + grub_host_to_target32 (s->sh_offset),
		  grub_host_to_target32 (s->sh_size));
      }
  free (kernel_img);

  return out_img;
}

static void
generate_image (const char *dir, char *prefix, FILE *out, char *mods[],
		char *memdisk_path, char *font_path, char *config_path,
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
		grub_platform_image_format_t format
#else
		int dummy __attribute__ ((unused))
#endif

)
{
  char *kernel_img, *core_img;
  size_t kernel_size, total_module_size, core_size, exec_size;
  size_t memdisk_size = 0, font_size = 0, config_size = 0, config_size_pure = 0;
  char *kernel_path;
  size_t offset;
  struct grub_util_path_list *path_list, *p, *next;
  struct grub_module_info *modinfo;
  grub_size_t bss_size;
  Elf_Addr start_address;
  void *rel_section;
  grub_size_t reloc_size;
  path_list = grub_util_resolve_dependencies (dir, "moddep.lst", mods);

  kernel_path = grub_util_get_path (dir, "kernel.img");

  total_module_size = sizeof (struct grub_module_info);

  if (memdisk_path)
    {
      memdisk_size = ALIGN_UP(grub_util_get_image_size (memdisk_path), 512);
      grub_util_info ("the size of memory disk is 0x%x", memdisk_size);
      total_module_size += memdisk_size + sizeof (struct grub_module_header);
    }

  if (font_path)
    {
      font_size = ALIGN_ADDR (grub_util_get_image_size (font_path));
      total_module_size += font_size + sizeof (struct grub_module_header);
    }

  if (config_path)
    {
      config_size_pure = grub_util_get_image_size (config_path) + 1;
      config_size = ALIGN_ADDR (config_size_pure);
      grub_util_info ("the size of config file is 0x%x", config_size);
      total_module_size += config_size + sizeof (struct grub_module_header);
    }

  for (p = path_list; p; p = p->next)
    total_module_size += (ALIGN_ADDR (grub_util_get_image_size (p->name))
			  + sizeof (struct grub_module_header));

  grub_util_info ("the total module size is 0x%x", total_module_size);

  kernel_img = load_image (kernel_path, &exec_size, &kernel_size, &bss_size,
			   total_module_size, &start_address, &rel_section,
			   &reloc_size);

  if (GRUB_KERNEL_MACHINE_PREFIX + strlen (prefix) + 1 > GRUB_KERNEL_MACHINE_DATA_END)
    grub_util_error (_("prefix is too long"));
  strcpy (kernel_img + GRUB_KERNEL_MACHINE_PREFIX, prefix);

  /* Fill in the grub_module_info structure.  */
  modinfo = (struct grub_module_info *) (kernel_img + kernel_size);
  memset (modinfo, 0, sizeof (struct grub_module_info));
  modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
  modinfo->offset = grub_host_to_target_addr (sizeof (struct grub_module_info));
  modinfo->size = grub_host_to_target_addr (total_module_size);

  offset = kernel_size + sizeof (struct grub_module_info);
  for (p = path_list; p; p = p->next)
    {
      struct grub_module_header *header;
      size_t mod_size, orig_size;

      orig_size = grub_util_get_image_size (p->name);
      mod_size = ALIGN_ADDR (orig_size);

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_ELF);
      header->size = grub_host_to_target32 (mod_size + sizeof (*header));
      offset += sizeof (*header);
      memset (kernel_img + offset + orig_size, 0, mod_size - orig_size);

      grub_util_load_image (p->name, kernel_img + offset);
      offset += mod_size;
    }

  if (memdisk_path)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_MEMDISK);
      header->size = grub_host_to_target32 (memdisk_size + sizeof (*header));
      offset += sizeof (*header);

      grub_util_load_image (memdisk_path, kernel_img + offset);
      offset += memdisk_size;
    }

  if (font_path)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_FONT);
      header->size = grub_host_to_target32 (font_size + sizeof (*header));
      offset += sizeof (*header);

      grub_util_load_image (font_path, kernel_img + offset);
      offset += font_size;
    }

  if (config_path)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_CONFIG);
      header->size = grub_host_to_target32 (config_size + sizeof (*header));
      offset += sizeof (*header);

      grub_util_load_image (config_path, kernel_img + offset);
      *(kernel_img + offset + config_size_pure - 1) = 0;
      offset += config_size;
    }

  grub_util_info ("kernel_img=%p, kernel_size=0x%x", kernel_img, kernel_size);
  compress_kernel (kernel_img, kernel_size + total_module_size,
		   &core_img, &core_size);

  grub_util_info ("the core size is 0x%x", core_size);

#ifdef GRUB_KERNEL_MACHINE_TOTAL_MODULE_SIZE
  *((grub_uint32_t *) (core_img + GRUB_KERNEL_MACHINE_TOTAL_MODULE_SIZE))
    = grub_host_to_target32 (total_module_size);
#endif
#ifdef GRUB_KERNEL_MACHINE_KERNEL_IMAGE_SIZE
  *((grub_uint32_t *) (core_img + GRUB_KERNEL_MACHINE_KERNEL_IMAGE_SIZE))
    = grub_host_to_target32 (kernel_size);
#endif
#ifdef GRUB_KERNEL_MACHINE_COMPRESSED_SIZE
  *((grub_uint32_t *) (core_img + GRUB_KERNEL_MACHINE_COMPRESSED_SIZE))
    = grub_host_to_target32 (core_size - GRUB_KERNEL_MACHINE_RAW_SIZE);
#endif

#if defined(GRUB_KERNEL_MACHINE_INSTALL_DOS_PART) && defined(GRUB_KERNEL_MACHINE_INSTALL_BSD_PART)
  /* If we included a drive in our prefix, let GRUB know it doesn't have to
     prepend the drive told by BIOS.  */
  if (prefix[0] == '(')
    {
      *((grub_int32_t *) (core_img + GRUB_KERNEL_MACHINE_INSTALL_DOS_PART))
	= grub_host_to_target32 (-2);
      *((grub_int32_t *) (core_img + GRUB_KERNEL_MACHINE_INSTALL_BSD_PART))
	= grub_host_to_target32 (-2);
    }
#endif

#ifdef GRUB_MACHINE_PCBIOS
  if (GRUB_KERNEL_MACHINE_LINK_ADDR + core_size > GRUB_MEMORY_MACHINE_UPPER)
    grub_util_error (_("core image is too big (%p > %p)"),
 		     GRUB_KERNEL_MACHINE_LINK_ADDR + core_size,
		     GRUB_MEMORY_MACHINE_UPPER);
#endif

#if defined(GRUB_MACHINE_PCBIOS)
  {
    unsigned num;
    char *boot_path, *boot_img;
    size_t boot_size;
    num = ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);
    if (num > 0xffff)
      grub_util_error (_("the core image is too big"));

    boot_path = grub_util_get_path (dir, "diskboot.img");
    boot_size = grub_util_get_image_size (boot_path);
    if (boot_size != GRUB_DISK_SECTOR_SIZE)
      grub_util_error (_("diskboot.img size must be %u bytes"),
		       GRUB_DISK_SECTOR_SIZE);

    boot_img = grub_util_read_image (boot_path);

    {
      struct grub_boot_blocklist *block;
      block = (struct grub_boot_blocklist *) (boot_img
					      + GRUB_DISK_SECTOR_SIZE
					      - sizeof (*block));
      block->len = grub_host_to_target16 (num);

      /* This is filled elsewhere.  Verify it just in case.  */
      assert (block->segment
	      == grub_host_to_target16 (GRUB_BOOT_MACHINE_KERNEL_SEG
					+ (GRUB_DISK_SECTOR_SIZE >> 4)));
    }

    grub_util_write_image (boot_img, boot_size, out);
    free (boot_img);
    free (boot_path);
  }
#elif defined(GRUB_MACHINE_EFI)
  {
    void *pe_img;
    size_t pe_size;
    struct grub_pe32_header *header;
    struct grub_pe32_coff_header *c;
    struct grub_pe32_optional_header *o;
    struct grub_pe32_section_table *text_section, *data_section;
    struct grub_pe32_section_table *mods_section, *reloc_section;
    static const grub_uint8_t stub[] = GRUB_PE32_MSDOS_STUB;
    int header_size = ALIGN_UP (sizeof (struct grub_pe32_header)
				+ 5 * sizeof (struct grub_pe32_section_table),
				SECTION_ALIGN);
    int reloc_addr = ALIGN_UP (header_size + core_size, SECTION_ALIGN);

    pe_size = ALIGN_UP (reloc_addr + reloc_size, SECTION_ALIGN);
    pe_img = xmalloc (reloc_addr + reloc_size);
    memset (pe_img, 0, header_size);
    memcpy (pe_img + header_size, core_img, core_size);
    memcpy (pe_img + reloc_addr, rel_section, reloc_size);
    header = pe_img;

    /* The magic.  */
    memcpy (header->msdos_stub, stub, sizeof (header->msdos_stub));
    memcpy (header->signature, "PE\0\0", sizeof (header->signature));

    /* The COFF file header.  */
    c = &header->coff_header;
#if GRUB_TARGET_SIZEOF_VOID_P == 4
    c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_I386);
#else
    c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_X86_64);
#endif

    c->num_sections = grub_host_to_target16 (4);
    c->time = grub_host_to_target32 (time (0));
    c->optional_header_size = grub_host_to_target16 (sizeof (header->optional_header));
    c->characteristics = grub_host_to_target16 (GRUB_PE32_EXECUTABLE_IMAGE
						| GRUB_PE32_LINE_NUMS_STRIPPED
#if GRUB_TARGET_SIZEOF_VOID_P == 4
						| GRUB_PE32_32BIT_MACHINE
#endif
						| GRUB_PE32_LOCAL_SYMS_STRIPPED
						| GRUB_PE32_DEBUG_STRIPPED);

    /* The PE Optional header.  */
    o = &header->optional_header;
    o->magic = grub_host_to_target16 (GRUB_PE32_PE32_MAGIC);
    o->code_size = grub_host_to_target32 (exec_size);
    o->data_size = grub_cpu_to_le32 (reloc_addr - exec_size);
    o->bss_size = grub_cpu_to_le32 (bss_size);
    o->entry_addr = grub_cpu_to_le32 (start_address);
    o->code_base = grub_cpu_to_le32 (header_size);
#if GRUB_TARGET_SIZEOF_VOID_P == 4
    o->data_base = grub_host_to_target32 (header_size + exec_size);
#endif
    o->image_base = 0;
    o->section_alignment = grub_host_to_target32 (SECTION_ALIGN);
    o->file_alignment = grub_host_to_target32 (SECTION_ALIGN);
    o->image_size = grub_host_to_target32 (pe_size);
    o->header_size = grub_host_to_target32 (header_size);
    o->subsystem = grub_host_to_target16 (GRUB_PE32_SUBSYSTEM_EFI_APPLICATION);

    /* Do these really matter? */
    o->stack_reserve_size = grub_host_to_target32 (0x10000);
    o->stack_commit_size = grub_host_to_target32 (0x10000);
    o->heap_reserve_size = grub_host_to_target32 (0x10000);
    o->heap_commit_size = grub_host_to_target32 (0x10000);
    
    o->num_data_directories = grub_host_to_target32 (GRUB_PE32_NUM_DATA_DIRECTORIES);

    o->base_relocation_table.rva = grub_host_to_target32 (reloc_addr);
    o->base_relocation_table.size = grub_host_to_target32 (reloc_size);

    /* The sections.  */
    text_section = (struct grub_pe32_section_table *) (header + 1);
    strcpy (text_section->name, ".text");
    text_section->virtual_size = grub_cpu_to_le32 (exec_size);
    text_section->virtual_address = grub_cpu_to_le32 (header_size);
    text_section->raw_data_size = grub_cpu_to_le32 (exec_size);
    text_section->raw_data_offset = grub_cpu_to_le32 (header_size);
    text_section->characteristics = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_CODE
						      | GRUB_PE32_SCN_MEM_EXECUTE
						      | GRUB_PE32_SCN_MEM_READ
						      | GRUB_PE32_SCN_ALIGN_64BYTES);

    data_section = text_section + 1;
    strcpy (data_section->name, ".data");
    data_section->virtual_size = grub_cpu_to_le32 (kernel_size - exec_size);
    data_section->virtual_address = grub_cpu_to_le32 (header_size + exec_size);
    data_section->raw_data_size = grub_cpu_to_le32 (kernel_size - exec_size);
    data_section->raw_data_offset = grub_cpu_to_le32 (header_size + exec_size);
    data_section->characteristics
      = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			  | GRUB_PE32_SCN_MEM_READ
			  | GRUB_PE32_SCN_MEM_WRITE
			  | GRUB_PE32_SCN_ALIGN_64BYTES);

#if 0
    bss_section = data_section + 1;
    strcpy (bss_section->name, ".bss");
    bss_section->virtual_size = grub_cpu_to_le32 (bss_size);
    bss_section->virtual_address = grub_cpu_to_le32 (header_size + kernel_size);
    bss_section->raw_data_size = 0;
    bss_section->raw_data_offset = 0;
    bss_section->characteristics
      = grub_cpu_to_le32 (GRUB_PE32_SCN_MEM_READ
			  | GRUB_PE32_SCN_MEM_WRITE
			  | GRUB_PE32_SCN_ALIGN_64BYTES
			  | GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			  | 0x80);
#endif
    
    mods_section = data_section + 1;
    strcpy (mods_section->name, "mods");
    mods_section->virtual_size = grub_cpu_to_le32 (reloc_addr - kernel_size - header_size);
    mods_section->virtual_address = grub_cpu_to_le32 (header_size + kernel_size + bss_size);
    mods_section->raw_data_size = grub_cpu_to_le32 (reloc_addr - kernel_size - header_size);
    mods_section->raw_data_offset = grub_cpu_to_le32 (header_size + kernel_size);
    mods_section->characteristics
      = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			  | GRUB_PE32_SCN_MEM_READ
			  | GRUB_PE32_SCN_MEM_WRITE
			  | GRUB_PE32_SCN_ALIGN_64BYTES);

    reloc_section = mods_section + 1;
    strcpy (reloc_section->name, ".reloc");
    reloc_section->virtual_size = grub_cpu_to_le32 (reloc_size);
    reloc_section->virtual_address = grub_cpu_to_le32 (reloc_addr + bss_size);
    reloc_section->raw_data_size = grub_cpu_to_le32 (reloc_size);
    reloc_section->raw_data_offset = grub_cpu_to_le32 (reloc_addr);
    reloc_section->characteristics
      = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			  | GRUB_PE32_SCN_MEM_DISCARDABLE
			  | GRUB_PE32_SCN_MEM_READ);
    free (core_img);
    core_img = pe_img;
    core_size = pe_size;
  }
#elif defined(GRUB_MACHINE_QEMU)
  {
    char *rom_img;
    size_t rom_size;
    char *boot_path, *boot_img;
    size_t boot_size;

    boot_path = grub_util_get_path (dir, "boot.img");
    boot_size = grub_util_get_image_size (boot_path);
    boot_img = grub_util_read_image (boot_path);

    /* Rom sizes must be 64k-aligned.  */
    rom_size = ALIGN_UP (core_size + boot_size, 64 * 1024);

    rom_img = xmalloc (rom_size);
    memset (rom_img, 0, rom_size);

    *((grub_int32_t *) (core_img + GRUB_KERNEL_MACHINE_CORE_ENTRY_ADDR))
      = grub_host_to_target32 ((grub_uint32_t) -rom_size);

    memcpy (rom_img, core_img, core_size);

    *((grub_int32_t *) (boot_img + GRUB_BOOT_MACHINE_CORE_ENTRY_ADDR))
      = grub_host_to_target32 ((grub_uint32_t) -rom_size);

    memcpy (rom_img + rom_size - boot_size, boot_img, boot_size);

    free (core_img);
    core_img = rom_img;
    core_size = rom_size;

    free (boot_img);
    free (boot_path);
  }
#elif defined (GRUB_MACHINE_SPARC64)
  if (format == GRUB_PLATFORM_IMAGE_AOUT)
    {
      void *aout_img;
      size_t aout_size;
      struct grub_aout32_header *aout_head;

      aout_size = core_size + sizeof (*aout_head);
      aout_img = xmalloc (aout_size);
      aout_head = aout_img;
      aout_head->a_midmag = grub_host_to_target32 ((AOUT_MID_SUN << 16)
						   | AOUT32_OMAGIC);
      aout_head->a_text = grub_host_to_target32 (core_size);
      aout_head->a_entry
	= grub_host_to_target32 (GRUB_BOOT_MACHINE_IMAGE_ADDRESS);
      memcpy (aout_img + sizeof (*aout_head), core_img, core_size);

      free (core_img);
      core_img = aout_img;
      core_size = aout_size;
    }
  else
    {
      unsigned int num;
      char *boot_path, *boot_img;
      size_t boot_size;

      num = ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);
      num <<= GRUB_DISK_SECTOR_BITS;

      boot_path = grub_util_get_path (dir, "diskboot.img");
      boot_size = grub_util_get_image_size (boot_path);
      if (boot_size != GRUB_DISK_SECTOR_SIZE)
	grub_util_error ("diskboot.img is not one sector size");

      boot_img = grub_util_read_image (boot_path);

      *((grub_uint32_t *) (boot_img + GRUB_DISK_SECTOR_SIZE
			   - GRUB_BOOT_MACHINE_LIST_SIZE + 8))
	= grub_host_to_target32 (num);

      grub_util_write_image (boot_img, boot_size, out);
      free (boot_img);
      free (boot_path);
    }
#elif defined(GRUB_MACHINE_MIPS)
  if (format == GRUB_PLATFORM_IMAGE_ELF)
    {
      char *elf_img;
      size_t program_size;
      Elf32_Ehdr *ehdr;
      Elf32_Phdr *phdr;
      grub_uint32_t target_addr;

      program_size = ALIGN_ADDR (core_size);

      elf_img = xmalloc (program_size + sizeof (*ehdr) + sizeof (*phdr));
      memset (elf_img, 0, program_size + sizeof (*ehdr) + sizeof (*phdr));
      memcpy (elf_img  + sizeof (*ehdr) + sizeof (*phdr), core_img, core_size);
      ehdr = (void *) elf_img;
      phdr = (void *) (elf_img + sizeof (*ehdr));
      memcpy (ehdr->e_ident, ELFMAG, SELFMAG);
      ehdr->e_ident[EI_CLASS] = ELFCLASS32;
#ifdef GRUB_CPU_MIPSEL
      ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
#else
      ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
#endif
      ehdr->e_ident[EI_VERSION] = EV_CURRENT;
      ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
      ehdr->e_type = grub_host_to_target16 (ET_EXEC);
      ehdr->e_machine = grub_host_to_target16 (EM_MIPS);
      ehdr->e_version = grub_host_to_target32 (EV_CURRENT);

      ehdr->e_phoff = grub_host_to_target32 ((char *) phdr - (char *) ehdr);
      ehdr->e_phentsize = grub_host_to_target16 (sizeof (*phdr));
      ehdr->e_phnum = grub_host_to_target16 (1);

      /* No section headers.  */
      ehdr->e_shoff = grub_host_to_target32 (0);
      ehdr->e_shentsize = grub_host_to_target16 (0);
      ehdr->e_shnum = grub_host_to_target16 (0);
      ehdr->e_shstrndx = grub_host_to_target16 (0);

      ehdr->e_ehsize = grub_host_to_target16 (sizeof (*ehdr));

      phdr->p_type = grub_host_to_target32 (PT_LOAD);
      phdr->p_offset = grub_host_to_target32 (sizeof (*ehdr) + sizeof (*phdr));
      phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);

      target_addr = ALIGN_UP (GRUB_KERNEL_MACHINE_LINK_ADDR 
			      + kernel_size + total_module_size, 32);
      ehdr->e_entry = grub_host_to_target32 (target_addr);
      phdr->p_vaddr = grub_host_to_target32 (target_addr);
      phdr->p_paddr = grub_host_to_target32 (target_addr);
      phdr->p_align = grub_host_to_target32 (GRUB_KERNEL_MACHINE_LINK_ALIGN);
      ehdr->e_flags = grub_host_to_target32 (0x1000 | EF_MIPS_NOREORDER 
					     | EF_MIPS_PIC | EF_MIPS_CPIC);
      phdr->p_filesz = grub_host_to_target32 (core_size);
      phdr->p_memsz = grub_host_to_target32 (core_size);

      free (core_img);
      core_img = elf_img;
      core_size = program_size  + sizeof (*ehdr) + sizeof (*phdr);
  }
#endif

  grub_util_write_image (core_img, core_size, out);
  free (kernel_img);
  free (core_img);
  free (kernel_path);

  while (path_list)
    {
      next = path_list->next;
      free ((void *) path_list->name);
      free (path_list);
      path_list = next;
    }
}



static struct option options[] =
  {
    {"directory", required_argument, 0, 'd'},
    {"prefix", required_argument, 0, 'p'},
    {"memdisk", required_argument, 0, 'm'},
    {"font", required_argument, 0, 'f'},
    {"config", required_argument, 0, 'c'},
    {"output", required_argument, 0, 'o'},
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
    {"format", required_argument, 0, 'O'},
#endif
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, _("Try `%s --help' for more information.\n"), program_name);
  else
    printf (_("\
Usage: %s [OPTION]... [MODULES]\n\
\n\
Make a bootable image of GRUB.\n\
\n\
  -d, --directory=DIR     use images and modules under DIR [default=%s]\n\
  -p, --prefix=DIR        set grub_prefix directory [default=%s]\n\
  -m, --memdisk=FILE      embed FILE as a memdisk image\n\
  -f, --font=FILE         embed FILE as a boot font\n\
  -c, --config=FILE       embed FILE as boot config\n\
  -o, --output=FILE       output a generated image to FILE [default=stdout]\n"
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
	    "\
  -O, --format=FORMAT     generate an image in format [default=%s]\n\
                          available formats: %s\n"
#endif
	    "\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
"), 
program_name, GRUB_LIBDIR, DEFAULT_DIRECTORY,
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
  GRUB_PLATFORM_IMAGE_DEFAULT_FORMAT, GRUB_PLATFORM_IMAGE_FORMATS,
#endif
PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  char *output = NULL;
  char *dir = NULL;
  char *prefix = NULL;
  char *memdisk = NULL;
  char *font = NULL;
  char *config = NULL;
  FILE *fp = stdout;
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
  grub_platform_image_format_t format = GRUB_PLATFORM_IMAGE_DEFAULT;
#endif

  set_program_name (argv[0]);

  grub_util_init_nls ();

  while (1)
    {
      int c = getopt_long (argc, argv, "d:p:m:c:o:O:f:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'o':
	    if (output)
	      free (output);

	    output = xstrdup (optarg);
	    break;

#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
	  case 'O':
#ifdef GRUB_PLATFORM_IMAGE_RAW
	    if (strcmp (optarg, "raw") == 0)
	      format = GRUB_PLATFORM_IMAGE_RAW;
	    else 
#endif
#ifdef GRUB_PLATFORM_IMAGE_ELF
	    if (strcmp (optarg, "elf") == 0)
	      format = GRUB_PLATFORM_IMAGE_ELF;
	    else 
#endif
#ifdef GRUB_PLATFORM_IMAGE_AOUT
	    if (strcmp (optarg, "aout") == 0)
	      format = GRUB_PLATFORM_IMAGE_AOUT;
	    else 
#endif
	      usage (1);
	    break;
#endif

	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;

	  case 'm':
	    if (memdisk)
	      free (memdisk);

	    memdisk = xstrdup (optarg);

	    if (prefix)
	      free (prefix);

	    prefix = xstrdup ("(memdisk)/boot/grub");
	    break;

	  case 'f':
	    if (font)
	      free (font);

	    font = xstrdup (optarg);
	    break;

	  case 'c':
	    if (config)
	      free (config);

	    config = xstrdup (optarg);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'p':
	    if (prefix)
	      free (prefix);

	    prefix = xstrdup (optarg);
	    break;

	  case 'V':
	    printf ("grub-mkimage (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (output)
    {
      fp = fopen (output, "wb");
      if (! fp)
	grub_util_error (_("cannot open %s"), output);
      free (output);
    }

  generate_image (dir ? : GRUB_LIBDIR, prefix ? : DEFAULT_DIRECTORY, fp,
		  argv + optind, memdisk, font, config,
#ifdef GRUB_PLATFORM_IMAGE_DEFAULT
		  format
#else
		  0
#endif
		  );

  fclose (fp);

  if (dir)
    free (dir);

  return 0;
}
