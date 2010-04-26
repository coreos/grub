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
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/i18n.h>
#include <grub/kernel.h>
#include <grub/disk.h>
#include <grub/util/misc.h>
#include <grub/util/resolve.h>
#include <grub/misc.h>
#include <grub/offsets.h>
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

#define ALIGN_ADDR(x) (ALIGN_UP((x), image_target->voidp_sizeof))

#define TARGET_NO_FIELD 0xffffffff
struct image_target_desc
{
  const char *name;
  grub_size_t voidp_sizeof;
  int bigendian;
  enum {
    IMAGE_I386_PC, IMAGE_EFI, IMAGE_COREBOOT,
    IMAGE_SPARC64_AOUT, IMAGE_SPARC64_RAW, IMAGE_I386_IEEE1275,
    IMAGE_YEELOONG_ELF, IMAGE_QEMU, IMAGE_PPC
  } id;
  enum {FORMAT_RAW, FORMAT_AOUT, FORMAT_ELF, FORMAT_PE} format;
  enum
    {
      PLATFORM_FLAGS_NONE = 0,
      PLATFORM_FLAGS_LZMA = 1
    } flags;
  unsigned prefix;
  unsigned data_end;
  unsigned raw_size;
  unsigned total_module_size;
  unsigned kernel_image_size;
  unsigned compressed_size;
  unsigned link_align;
  grub_uint16_t elf_target;
  unsigned section_align;
  signed vaddr_offset;
  unsigned install_dos_part, install_bsd_part;
  grub_uint64_t link_addr;
};

struct image_target_desc image_targets[] =
  {
    {"i386-coreboot", 4, 0, IMAGE_COREBOOT, FORMAT_ELF, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0
    },
    {"i386-pc", 4, 0, IMAGE_I386_PC, FORMAT_RAW, PLATFORM_FLAGS_LZMA,
     GRUB_KERNEL_I386_PC_PREFIX, GRUB_KERNEL_I386_PC_DATA_END,
     GRUB_KERNEL_I386_PC_RAW_SIZE, GRUB_KERNEL_I386_PC_TOTAL_MODULE_SIZE,
     GRUB_KERNEL_I386_PC_KERNEL_IMAGE_SIZE, GRUB_KERNEL_I386_PC_COMPRESSED_SIZE,
     .section_align = 1,
     .vaddr_offset = 0,
     GRUB_KERNEL_I386_PC_INSTALL_DOS_PART, GRUB_KERNEL_I386_PC_INSTALL_BSD_PART
    },
    {"i386-efi", 4, 0, IMAGE_EFI, FORMAT_PE, PLATFORM_FLAGS_NONE,
     .section_align = GRUB_PE32_SECTION_ALIGNMENT,
     .vaddr_offset = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
			       + GRUB_PE32_SIGNATURE_SIZE
			       + sizeof (struct grub_pe32_coff_header)
			       + sizeof (struct grub_pe32_optional_header)
			       + 4 * sizeof (struct grub_pe32_section_table),
			       GRUB_PE32_SECTION_ALIGNMENT)
    },
    {"i386-ieee1275", 4, 0, IMAGE_I386_IEEE1275, FORMAT_ELF, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
    {"i386-qemu", 4, 0, IMAGE_QEMU, FORMAT_RAW, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
    {"x86_64-efi", 8, 0, IMAGE_EFI, FORMAT_PE, PLATFORM_FLAGS_NONE,
     .section_align = GRUB_PE32_SECTION_ALIGNMENT,
     .vaddr_offset = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
			       + GRUB_PE32_SIGNATURE_SIZE
			       + sizeof (struct grub_pe32_coff_header)
			       + sizeof (struct grub_pe64_optional_header)
			       + 4 * sizeof (struct grub_pe32_section_table),
			       GRUB_PE32_SECTION_ALIGNMENT)
    },
    {"mipsel-yeeloong-elf", 4, 0, IMAGE_YEELOONG_ELF, FORMAT_ELF, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
    {"powerpc-ieee1275", 4, 1, IMAGE_PPC, FORMAT_ELF, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
    {"sparc64-ieee1275-raw", 8, 1, IMAGE_SPARC64_RAW,
     FORMAT_RAW, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
    {"sparc64-ieee1275-aout", 8, 1, IMAGE_SPARC64_AOUT,
     FORMAT_AOUT, PLATFORM_FLAGS_NONE,
     .section_align = 1,
     .vaddr_offset = 0},
  };

#define grub_target_to_host32(x) (grub_target_to_host32_real (image_target, (x)))
#define grub_host_to_target32(x) (grub_host_to_target32_real (image_target, (x)))
#define grub_target_to_host16(x) (grub_target_to_host16_real (image_target, (x)))
#define grub_host_to_target16(x) (grub_host_to_target16_real (image_target, (x)))

static inline grub_uint32_t
grub_target_to_host32_real (struct image_target_desc *image_target, grub_uint32_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu32 (in);
  else
    return grub_le_to_cpu32 (in);
}

static inline grub_uint64_t
grub_target_to_host64_real (struct image_target_desc *image_target, grub_uint64_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu64 (in);
  else
    return grub_le_to_cpu64 (in);
}

static inline grub_uint32_t
grub_target_to_host_real (struct image_target_desc *image_target, grub_uint32_t in)
{
  if (image_target->voidp_sizeof == 8)
    return grub_target_to_host64_real (image_target, in);
  else
    return grub_target_to_host32_real (image_target, in);
}

static inline grub_uint32_t
grub_host_to_target32_real (struct image_target_desc *image_target, grub_uint32_t in)
{
  if (image_target->bigendian)
    return grub_cpu_to_be32 (in);
  else
    return grub_cpu_to_le32 (in);
}

static inline grub_uint16_t
grub_target_to_host16_real (struct image_target_desc *image_target, grub_uint16_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu16 (in);
  else
    return grub_le_to_cpu16 (in);
}

static inline grub_uint16_t
grub_host_to_target16_real (struct image_target_desc *image_target, grub_uint16_t in)
{
  if (image_target->bigendian)
    return grub_cpu_to_be16 (in);
  else
    return grub_cpu_to_le16 (in);
}

#define GRUB_IEEE1275_NOTE_NAME "PowerPC"
#define GRUB_IEEE1275_NOTE_TYPE 0x1275

/* These structures are defined according to the CHRP binding to IEEE1275,
   "Client Program Format" section.  */

struct grub_ieee1275_note_hdr
{
  grub_uint32_t namesz;
  grub_uint32_t descsz;
  grub_uint32_t type;
  char name[sizeof (GRUB_IEEE1275_NOTE_NAME)];
};

struct grub_ieee1275_note_desc
{
  grub_uint32_t real_mode;
  grub_uint32_t real_base;
  grub_uint32_t real_size;
  grub_uint32_t virt_base;
  grub_uint32_t virt_size;
  grub_uint32_t load_base;
};

struct grub_ieee1275_note
{
  struct grub_ieee1275_note_hdr header;
  struct grub_ieee1275_note_desc descriptor;
};

#define grub_target_to_host(val) grub_target_to_host_real(image_target, (val))

#include <grub/lib/LzmaEnc.h>

static void *SzAlloc(void *p, size_t size) { p = p; return xmalloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static void
compress_kernel_lzma (char *kernel_img, size_t kernel_size,
		      char **core_img, size_t *core_size, size_t raw_size)
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

  if (kernel_size < raw_size)
    grub_util_error (_("the core image is too small"));

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, raw_size);

  *core_size = kernel_size - raw_size;
  if (LzmaEncode ((unsigned char *) *core_img + raw_size, core_size,
		  (unsigned char *) kernel_img + raw_size,
		  kernel_size - raw_size,
		  &props, out_props, &out_props_size,
		  0, NULL, &g_Alloc, &g_Alloc) != SZ_OK)
    grub_util_error (_("cannot compress the kernel image"));

  *core_size += raw_size;
}

static void
compress_kernel (struct image_target_desc *image_target, char *kernel_img,
		 size_t kernel_size, char **core_img, size_t *core_size)
{
 if (image_target->flags & PLATFORM_FLAGS_LZMA)
   {
     compress_kernel_lzma (kernel_img, kernel_size, core_img,
			   core_size, image_target->raw_size);
     return;
   }

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, kernel_size);
  *core_size = kernel_size;
}


/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf_Addr
relocate_symbols (Elf_Ehdr *e, Elf_Shdr *sections,
		  Elf_Shdr *symtab_section, Elf_Addr *section_addresses,
		  Elf_Half section_entsize, Elf_Half num_sections,
		  struct image_target_desc *image_target)
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
get_symbol_address (Elf_Ehdr *e, Elf_Shdr *s, Elf_Word i,
		    struct image_target_desc *image_target)
{
  Elf_Sym *sym;

  sym = (Elf_Sym *) ((char *) e
		       + grub_target_to_host32 (s->sh_offset)
		       + i * grub_target_to_host32 (s->sh_entsize));
  return sym->st_value;
}

/* Return the address of a modified value.  */
static Elf_Addr *
get_target_address (Elf_Ehdr *e, Elf_Shdr *s, Elf_Addr offset,
		    struct image_target_desc *image_target)
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
		    const char *strtab, struct image_target_desc *image_target)
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
	    target = get_target_address (e, target_section, offset, image_target);
	    info = grub_target_to_host (r->r_info);
	    sym_addr = get_symbol_address (e, symtab_section,
					   ELF_R_SYM (info), image_target);

            addend = (s->sh_type == grub_target_to_host32 (SHT_RELA)) ?
	      r->r_addend : 0;

	    if (image_target->voidp_sizeof == 4)
	      switch (ELF_R_TYPE (info))
		{
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
						   - image_target->vaddr_offset);
		  grub_util_info ("relocating an R_386_PC32 entry to 0x%x at the offset 0x%x",
				  *target, offset);
		  break;
		default:
		  grub_util_error ("unknown relocation type %d",
				   ELF_R_TYPE (info));
		  break;
		}
	    else
	      switch (ELF_R_TYPE (info))
		{

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
						  - image_target->vaddr_offset);
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
		 Elf_Addr addr, int flush, Elf_Addr current_address,
		 struct image_target_desc *image_target)
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
	      padding_size = ((ALIGN_UP (next_address, image_target->section_align)
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
		    const char *strtab, struct image_target_desc *image_target)
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
	    if (image_target->voidp_sizeof == 4)
	      {
		if (ELF_R_TYPE (info) == R_386_32)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%x", addr);
		    current_address = add_fixup_entry (&lst,
						       GRUB_PE32_REL_BASED_HIGHLOW,
						       addr, 0, current_address,
						       image_target);
		  }
	      }
	    else
	      {
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
						       0, current_address,
						       image_target);
		  }
	      }
	  }
      }

  current_address = add_fixup_entry (&lst, 0, 0, 1, current_address, image_target);

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

/* Determine if this section is a text section. Return false if this
   section is not allocated.  */
static int
is_text_section (Elf_Shdr *s, struct image_target_desc *image_target)
{
  if (image_target->id != IMAGE_EFI 
      && grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
  return ((grub_target_to_host32 (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == (SHF_EXECINSTR | SHF_ALLOC));
}

/* Determine if this section is a data section. This assumes that
   BSS is also a data section, since the converter initializes BSS
   when producing PE32 to avoid a bug in EFI implementations.  */
static int
is_data_section (Elf_Shdr *s, struct image_target_desc *image_target)
{
  if (image_target->id != IMAGE_EFI 
      && grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
  return ((grub_target_to_host32 (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == SHF_ALLOC);
}

/* Locate section addresses by merging code sections and data sections
   into .text and .data, respectively. Return the array of section
   addresses.  */
static Elf_Addr *
locate_sections (Elf_Shdr *sections, Elf_Half section_entsize,
		 Elf_Half num_sections, const char *strtab,
		 grub_size_t *exec_size, grub_size_t *kernel_sz,
		 struct image_target_desc *image_target)
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
    if (is_text_section (s, image_target))
      {
	Elf_Word align = grub_host_to_target32 (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);

	if (align)
	  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
				      align) - image_target->vaddr_offset;

	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_host_to_target32 (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
			      image_target->section_align)
    - image_target->vaddr_offset;
  *exec_size = current_address;

  /* .data */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s, image_target))
      {
	Elf_Word align = grub_host_to_target32 (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);

	if (align)
	  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
				      align)
	    - image_target->vaddr_offset;

	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_host_to_target32 (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
			      image_target->section_align) - image_target->vaddr_offset;
  *kernel_sz = current_address;
  return section_addresses;
}

/* Return if the ELF header is valid.  */
static int
check_elf_header (Elf_Ehdr *e, size_t size, struct image_target_desc *image_target)
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
	    void **reloc_section, grub_size_t *reloc_size,
	    struct image_target_desc *image_target)
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
  if (! check_elf_header (e, kernel_size, image_target))
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
				       exec_size, kernel_sz, image_target);

  if (image_target->id == IMAGE_EFI)
    {
      section_vaddresses = xmalloc (sizeof (*section_addresses) * num_sections);

      for (i = 0; i < num_sections; i++)
	section_vaddresses[i] = section_addresses[i] + image_target->vaddr_offset;

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
				 num_sections, image_target);
      if (*start == 0)
	grub_util_error ("start symbol is not defined");

      /* Resolve addresses in the virtual address space.  */
      relocate_addresses (e, sections, section_addresses, section_entsize,
			  num_sections, strtab, image_target);

      *reloc_size = make_reloc_section (e, reloc_section,
					section_vaddresses, sections,
					section_entsize, num_sections,
					strtab, image_target);

    }
  else
    {
      *bss_size = 0;
      *reloc_size = 0;
      *reloc_section = NULL;
    }

  out_img = xmalloc (*kernel_sz + total_module_size);

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s, image_target) || is_text_section (s, image_target))
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
		struct image_target_desc *image_target, int note)
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
			   &reloc_size, image_target);

  if (image_target->prefix + strlen (prefix) + 1 > image_target->data_end)
    grub_util_error (_("prefix is too long"));
  strcpy (kernel_img + image_target->prefix, prefix);

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
  compress_kernel (image_target, kernel_img, kernel_size + total_module_size,
		   &core_img, &core_size);

  grub_util_info ("the core size is 0x%x", core_size);

  if (image_target->total_module_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->total_module_size))
      = grub_host_to_target32 (total_module_size);
  if (image_target->kernel_image_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->kernel_image_size))
      = grub_host_to_target32 (kernel_size);
  if (image_target->compressed_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->compressed_size))
      = grub_host_to_target32 (core_size - image_target->raw_size);

  /* If we included a drive in our prefix, let GRUB know it doesn't have to
     prepend the drive told by BIOS.  */
  if (image_target->install_dos_part != TARGET_NO_FIELD
      && image_target->install_bsd_part != TARGET_NO_FIELD && prefix[0] == '(')
    {
      *((grub_int32_t *) (core_img + image_target->install_dos_part))
	= grub_host_to_target32 (-2);
      *((grub_int32_t *) (core_img + image_target->install_bsd_part))
	= grub_host_to_target32 (-2);
    }

  switch (image_target->id)
    {
    case IMAGE_I386_PC:
      {
	unsigned num;
	char *boot_path, *boot_img;
	size_t boot_size;

	if (GRUB_KERNEL_MACHINE_LINK_ADDR + core_size > GRUB_MEMORY_I386_PC_UPPER)
	  grub_util_error (_("core image is too big (%p > %p)"),
			   GRUB_KERNEL_MACHINE_LINK_ADDR + core_size,
			   GRUB_MEMORY_I386_PC_UPPER);

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
	  struct grub_pc_bios_boot_blocklist *block;
	  block = (struct grub_pc_bios_boot_blocklist *) (boot_img
							  + GRUB_DISK_SECTOR_SIZE
							  - sizeof (*block));
	  block->len = grub_host_to_target16 (num);

	  /* This is filled elsewhere.  Verify it just in case.  */
	  assert (block->segment
		  == grub_host_to_target16 (GRUB_BOOT_I386_PC_KERNEL_SEG
					    + (GRUB_DISK_SECTOR_SIZE >> 4)));
	}

	grub_util_write_image (boot_img, boot_size, out);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_EFI:
      {
	void *pe_img;
	grub_uint8_t *header;
	size_t pe_size;
	struct grub_pe32_coff_header *c;
	struct grub_pe32_section_table *text_section, *data_section;
	struct grub_pe32_section_table *mods_section, *reloc_section;
	static const grub_uint8_t stub[] = GRUB_PE32_MSDOS_STUB;
	int header_size;
	int reloc_addr;

	if (image_target->voidp_sizeof == 4)
	  header_size = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				  + GRUB_PE32_SIGNATURE_SIZE
				  + sizeof (struct grub_pe32_coff_header)
				  + sizeof (struct grub_pe32_optional_header)
				  + 4 * sizeof (struct grub_pe32_section_table),
				  GRUB_PE32_SECTION_ALIGNMENT);
	else
	  header_size = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				  + GRUB_PE32_SIGNATURE_SIZE
				  + sizeof (struct grub_pe32_coff_header)
				  + sizeof (struct grub_pe64_optional_header)
				  + 4 * sizeof (struct grub_pe32_section_table),
				  GRUB_PE32_SECTION_ALIGNMENT);

	reloc_addr = ALIGN_UP (header_size + core_size,
			       image_target->section_align);

	pe_size = ALIGN_UP (reloc_addr + reloc_size,
			    image_target->section_align);
	pe_img = xmalloc (reloc_addr + reloc_size);
	memset (pe_img, 0, header_size);
	memcpy (pe_img + header_size, core_img, core_size);
	memcpy (pe_img + reloc_addr, rel_section, reloc_size);
	header = pe_img;

	/* The magic.  */
	memcpy (header, stub, GRUB_PE32_MSDOS_STUB_SIZE);
	memcpy (header + GRUB_PE32_MSDOS_STUB_SIZE, "PE\0\0",
		GRUB_PE32_SIGNATURE_SIZE);

	/* The COFF file header.  */
	c = header + GRUB_PE32_MSDOS_STUB_SIZE + GRUB_PE32_SIGNATURE_SIZE;
	if (image_target->voidp_sizeof == 4)
	  c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_I386);
	else
	  c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_X86_64);

	c->num_sections = grub_host_to_target16 (4);
	c->time = grub_host_to_target32 (time (0));
	c->characteristics = grub_host_to_target16 (GRUB_PE32_EXECUTABLE_IMAGE
						    | GRUB_PE32_LINE_NUMS_STRIPPED
						    | ((image_target->voidp_sizeof == 4)
						       ? GRUB_PE32_32BIT_MACHINE
						       : 0)
						    | GRUB_PE32_LOCAL_SYMS_STRIPPED
						    | GRUB_PE32_DEBUG_STRIPPED);

	/* The PE Optional header.  */
	if (image_target->voidp_sizeof == 4)
	  {
	    struct grub_pe32_optional_header *o;

	    c->optional_header_size = grub_host_to_target16 (sizeof (struct grub_pe32_optional_header));

	    o = header + GRUB_PE32_MSDOS_STUB_SIZE + GRUB_PE32_SIGNATURE_SIZE
	      + sizeof (struct grub_pe32_coff_header);
	    o->magic = grub_host_to_target16 (GRUB_PE32_PE32_MAGIC);
	    o->code_size = grub_host_to_target32 (exec_size);
	    o->data_size = grub_cpu_to_le32 (reloc_addr - exec_size);
	    o->bss_size = grub_cpu_to_le32 (bss_size);
	    o->entry_addr = grub_cpu_to_le32 (start_address);
	    o->code_base = grub_cpu_to_le32 (header_size);

	    o->data_base = grub_host_to_target32 (header_size + exec_size);

	    o->image_base = 0;
	    o->section_alignment = grub_host_to_target32 (image_target->section_align);
	    o->file_alignment = grub_host_to_target32 (image_target->section_align);
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
	  }
	else
	  {
	    struct grub_pe64_optional_header *o;

	    c->optional_header_size = grub_host_to_target16 (sizeof (struct grub_pe32_optional_header));

	    o = header + GRUB_PE32_MSDOS_STUB_SIZE + GRUB_PE32_SIGNATURE_SIZE
	      + sizeof (struct grub_pe32_coff_header);
	    o->magic = grub_host_to_target16 (GRUB_PE32_PE32_MAGIC);
	    o->code_size = grub_host_to_target32 (exec_size);
	    o->data_size = grub_cpu_to_le32 (reloc_addr - exec_size);
	    o->bss_size = grub_cpu_to_le32 (bss_size);
	    o->entry_addr = grub_cpu_to_le32 (start_address);
	    o->code_base = grub_cpu_to_le32 (header_size);
	    o->image_base = 0;
	    o->section_alignment = grub_host_to_target32 (image_target->section_align);
	    o->file_alignment = grub_host_to_target32 (image_target->section_align);
	    o->image_size = grub_host_to_target32 (pe_size);
	    o->header_size = grub_host_to_target32 (header_size);
	    o->subsystem = grub_host_to_target16 (GRUB_PE32_SUBSYSTEM_EFI_APPLICATION);

	    /* Do these really matter? */
	    o->stack_reserve_size = grub_host_to_target32 (0x10000);
	    o->stack_commit_size = grub_host_to_target32 (0x10000);
	    o->heap_reserve_size = grub_host_to_target32 (0x10000);
	    o->heap_commit_size = grub_host_to_target32 (0x10000);
    
	    o->num_data_directories
	      = grub_host_to_target32 (GRUB_PE32_NUM_DATA_DIRECTORIES);

	    o->base_relocation_table.rva = grub_host_to_target32 (reloc_addr);
	    o->base_relocation_table.size = grub_host_to_target32 (reloc_size);
	  }
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
      break;
    case IMAGE_QEMU:
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

	*((grub_int32_t *) (core_img + GRUB_KERNEL_I386_QEMU_CORE_ENTRY_ADDR))
	  = grub_host_to_target32 ((grub_uint32_t) -rom_size);

	memcpy (rom_img, core_img, core_size);

	*((grub_int32_t *) (boot_img + GRUB_BOOT_I386_QEMU_CORE_ENTRY_ADDR))
	  = grub_host_to_target32 ((grub_uint32_t) -rom_size);

	memcpy (rom_img + rom_size - boot_size, boot_img, boot_size);

	free (core_img);
	core_img = rom_img;
	core_size = rom_size;

	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_SPARC64_AOUT:
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
	  = grub_host_to_target32 (GRUB_BOOT_SPARC64_IEEE1275_IMAGE_ADDRESS);
	memcpy (aout_img + sizeof (*aout_head), core_img, core_size);

	free (core_img);
	core_img = aout_img;
	core_size = aout_size;
      }
      break;
    case IMAGE_SPARC64_RAW:
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
			     - GRUB_BOOT_SPARC64_IEEE1275_LIST_SIZE + 8))
	  = grub_host_to_target32 (num);

	grub_util_write_image (boot_img, boot_size, out);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_YEELOONG_ELF:
    case IMAGE_PPC:
    case IMAGE_COREBOOT:
    case IMAGE_I386_IEEE1275:
      {
	char *elf_img;
	size_t program_size;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	grub_uint32_t target_addr;
	int header_size, footer_size = 0;
	int phnum = 1;

	if (note)
	  {
	    phnum++;
	    footer_size += sizeof (struct grub_ieee1275_note);
	  }
	header_size = ALIGN_ADDR (sizeof (*ehdr) + phnum * sizeof (*phdr));

	program_size = ALIGN_ADDR (core_size);

	elf_img = xmalloc (program_size + header_size + footer_size);
	memset (elf_img, 0, program_size + header_size);
	memcpy (elf_img  + header_size, core_img, core_size);
	ehdr = (void *) elf_img;
	phdr = (void *) (elf_img + sizeof (*ehdr));
	memcpy (ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	if (!image_target->bigendian)
	  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	else
	  ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
	ehdr->e_type = grub_host_to_target16 (ET_EXEC);
	ehdr->e_machine = grub_host_to_target16 (image_target->elf_target);
	ehdr->e_version = grub_host_to_target32 (EV_CURRENT);

	ehdr->e_phoff = grub_host_to_target32 ((char *) phdr - (char *) ehdr);
	ehdr->e_phentsize = grub_host_to_target16 (sizeof (*phdr));
	ehdr->e_phnum = grub_host_to_target16 (phnum);

	/* No section headers.  */
	ehdr->e_shoff = grub_host_to_target32 (0);
	ehdr->e_shentsize = grub_host_to_target16 (0);
	ehdr->e_shnum = grub_host_to_target16 (0);
	ehdr->e_shstrndx = grub_host_to_target16 (0);

	ehdr->e_ehsize = grub_host_to_target16 (sizeof (*ehdr));

	phdr->p_type = grub_host_to_target32 (PT_LOAD);
	phdr->p_offset = grub_host_to_target32 (header_size);
	phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);

	if (image_target->id == IMAGE_YEELOONG_ELF)
	  target_addr = ALIGN_UP (image_target->link_addr
				  + kernel_size + total_module_size, 32);
	else
	  target_addr = image_target->link_addr;
	ehdr->e_entry = grub_host_to_target32 (target_addr);
	phdr->p_vaddr = grub_host_to_target32 (target_addr);
	phdr->p_paddr = grub_host_to_target32 (target_addr);
	phdr->p_align = grub_host_to_target32 (image_target->link_align);
	if (image_target->id == IMAGE_YEELOONG_ELF)
	  ehdr->e_flags = grub_host_to_target32 (0x1000 | EF_MIPS_NOREORDER 
						 | EF_MIPS_PIC | EF_MIPS_CPIC);
	else
	  ehdr->e_flags = 0;
	phdr->p_filesz = grub_host_to_target32 (core_size);
	phdr->p_memsz = grub_host_to_target32 (core_size);

	if (note)
	  {
	    int note_size = sizeof (struct grub_ieee1275_note);
	    struct grub_ieee1275_note *note = (struct grub_ieee1275_note *) 
	      (elf_img + program_size + header_size);

	    grub_util_info ("adding CHRP NOTE segment");

	    note->header.namesz = grub_host_to_target32 (sizeof (GRUB_IEEE1275_NOTE_NAME));
	    note->header.descsz = grub_host_to_target32 (note_size);
	    note->header.type = grub_host_to_target32 (GRUB_IEEE1275_NOTE_TYPE);
	    strcpy (note->header.name, GRUB_IEEE1275_NOTE_NAME);
	    note->descriptor.real_mode = grub_host_to_target32 (0xffffffff);
	    note->descriptor.real_base = grub_host_to_target32 (0x00c00000);
	    note->descriptor.real_size = grub_host_to_target32 (0xffffffff);
	    note->descriptor.virt_base = grub_host_to_target32 (0xffffffff);
	    note->descriptor.virt_size = grub_host_to_target32 (0xffffffff);
	    note->descriptor.load_base = grub_host_to_target32 (0x00004000);

	    phdr[1].p_type = grub_host_to_target32 (PT_NOTE);
	    phdr[1].p_flags = grub_host_to_target32 (PF_R);
	    phdr[1].p_align = grub_host_to_target32 (GRUB_TARGET_SIZEOF_LONG);
	    phdr[1].p_vaddr = 0;
	    phdr[1].p_paddr = 0;
	    phdr[1].p_filesz = grub_host_to_target32 (note_size);
	    phdr[1].p_memsz = 0;
	    phdr[1].p_offset = grub_host_to_target32 (header_size + program_size);
	  }

	free (core_img);
	core_img = elf_img;
	core_size = program_size + header_size + footer_size;
      }
      break;
    }

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
    {"note", no_argument, 0, 'n'},
    {"format", required_argument, 0, 'O'},
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
    {
      int format_len = 0;
      char *formats;
      char *ptr;
      unsigned i;
      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	format_len += strlen (image_targets[i].name) + 2;
      ptr = formats = xmalloc (format_len);
      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	{
	  strcpy (ptr, image_targets[i].name);
	  ptr += strlen (image_targets[i].name);
	  *ptr++ = ',';
	  *ptr++ = ' ';
	}
      ptr[-2] = 0;

      printf (_("\
Usage: %s [OPTION]... [MODULES]\n\
\n\
Make a bootable image of GRUB.\n\
\n\
  -d, --directory=DIR     use images and modules under DIR [default=%s/@platform@]\n\
  -p, --prefix=DIR        set grub_prefix directory [default=%s]\n\
  -m, --memdisk=FILE      embed FILE as a memdisk image\n\
  -f, --font=FILE         embed FILE as a boot font\n\
  -c, --config=FILE       embed FILE as boot config\n\
  -n, --note              add NOTE segment for CHRP Open Firmware\n\
  -o, --output=FILE       output a generated image to FILE [default=stdout]\n\
  -O, --format=FORMAT     generate an image in format\n\
                          available formats: %s\n\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
"), 
	      program_name, GRUB_LIBDIR, DEFAULT_DIRECTORY,
	      formats,
	      PACKAGE_BUGREPORT);
      free (formats);
    }
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
  int note = 0;
  struct image_target_desc *image_target = NULL;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  while (1)
    {
      int c = getopt_long (argc, argv, "d:p:m:c:o:O:f:hVvn", options, 0);

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

	  case 'O':
	    {
	      unsigned i;
	      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
		if (strcmp (optarg, image_targets[i].name) == 0)
		  image_target = &image_targets[i];
	      if (!image_target)
		{
		  printf ("unknown target %s\n", optarg);
		  usage (1);
		}
	      break;
	    }
	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;

	  case 'n':
	    note = 1;
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

  if (!image_target)
    {
      printf ("Target not specified.\n");
      usage (1);
    }

  if (output)
    {
      fp = fopen (output, "wb");
      if (! fp)
	grub_util_error (_("cannot open %s"), output);
      free (output);
    }

  if (!dir)
    {
      const char *last;
      last = strchr (image_target->name, '-');
      if (last)
	last = strchr (last + 1, '-');
      if (!last)
	last = image_target->name + strlen (image_target->name);
      dir = xmalloc (sizeof (GRUB_LIBDIR) + (last - image_target->name));
      memcpy (dir, GRUB_LIBDIR, sizeof (GRUB_LIBDIR) - 1);
      memcpy (dir + sizeof (GRUB_LIBDIR) - 1, image_target->name,
	      last - image_target->name);
      *(dir + sizeof (GRUB_LIBDIR) - 1 +  (last - image_target->name)) = 0;
    }

  generate_image (dir ? : GRUB_LIBDIR, prefix ? : DEFAULT_DIRECTORY, fp,
		  argv + optind, memdisk, font, config,
		  image_target, note);

  fclose (fp);

  if (dir)
    free (dir);

  return 0;
}
