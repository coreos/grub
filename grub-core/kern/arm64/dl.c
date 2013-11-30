/* dl.c - arch-dependent part of loadable module support */
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

#include <grub/dl.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/i18n.h>
#include <grub/cpu/reloc.h>

/*
 * Check if EHDR is a valid ELF header.
 */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB || e->e_machine != EM_AARCH64)
    return grub_error (GRUB_ERR_BAD_OS,
		       N_("invalid arch-dependent ELF magic"));

  return GRUB_ERR_NONE;
}

/*
 * Unified function for both REL and RELA 
 */
static grub_err_t
do_relX (Elf_Shdr * relhdr, Elf_Ehdr * e, grub_dl_t mod)
{
  grub_err_t retval;
  grub_dl_segment_t segment;
  Elf_Rel *rel;
  Elf_Rela *rela;
  Elf_Sym *symbol;
  int i, entnum;
  unsigned long long  entsize;

  /* Find the target segment for this relocation section. */
  for (segment = mod->segment ; segment != 0 ; segment = segment->next)
    if (segment->section == relhdr->sh_info)
      break;
  if (!segment)
    return grub_error (GRUB_ERR_EOF, N_("relocation segment not found"));

  rel = (Elf_Rel *) ((grub_addr_t) e + relhdr->sh_offset);
  rela = (Elf_Rela *) rel;
  if (relhdr->sh_type == SHT_RELA)
    entsize = sizeof (Elf_Rela);
  else
    entsize = sizeof (Elf_Rel);

  entnum = relhdr->sh_size / entsize;
  retval = GRUB_ERR_NONE;

  grub_dprintf("dl", "Processing %d relocation entries.\n", entnum);

  /* Step through all relocations */
  for (i = 0, symbol = mod->symtab; i < entnum; i++)
    {
      void *place;
      grub_uint64_t sym_addr, symidx, reltype;

      if (rel->r_offset >= segment->size)
	return grub_error (GRUB_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      symidx = ELF_R_SYM (rel->r_info);
      reltype = ELF_R_TYPE (rel->r_info);

      sym_addr = symbol[symidx].st_value;
      if (relhdr->sh_type == SHT_RELA)
	sym_addr += rela->r_addend;

      place = (void *) ((grub_addr_t) segment->addr + rel->r_offset);

      switch (reltype)
	{
	case R_AARCH64_ABS64:
	  {
	    grub_uint64_t *abs_place = place;

	    grub_dprintf ("dl", "  reloc_abs64 %p => 0x%016llx\n",
			  place, (unsigned long long) sym_addr);

	    *abs_place = (grub_uint64_t) sym_addr;
	  }
	  break;
	case R_AARCH64_CALL26:
	case R_AARCH64_JUMP26:
	  retval = grub_arm64_reloc_xxxx26 (place, sym_addr);
	  break;
	default:
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     reltype);
	}

      if (retval != GRUB_ERR_NONE)
	break;

      rel = (Elf_Rel *) ((grub_addr_t) rel + entsize);
      rela++;
    }

  return retval;
}

/*
 * Verify that provided ELF header contains reference to a symbol table
 */
static int
has_symtab (Elf_Ehdr * e)
{
  int i;
  Elf_Shdr *s;

  for (i = 0, s = (Elf_Shdr *) ((grub_addr_t) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((grub_addr_t) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      return 1;

  return 0;
}

/*
 * grub_arch_dl_relocate_symbols():
 *   Locates the relocations section of the ELF object, and calls
 *   do_relX() to deal with it.
 */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf_Ehdr *e = ehdr;
  Elf_Shdr *s;
  unsigned i;

  if (!has_symtab (e))
    return grub_error (GRUB_ERR_BAD_MODULE, N_("no symbol table"));

#define FIRST_SHDR(x) ((Elf_Shdr *) ((grub_addr_t)(x) + (x)->e_shoff))
#define NEXT_SHDR(x, y) ((Elf_Shdr *) ((grub_addr_t)(y) + (x)->e_shentsize))

  for (i = 0, s = FIRST_SHDR (e); i < e->e_shnum; i++, s = NEXT_SHDR (e, s))
    {
      grub_err_t ret;

      switch (s->sh_type)
	{
	case SHT_REL:
	case SHT_RELA:
	  {
	    ret = do_relX (s, e, mod);
	    if (ret != GRUB_ERR_NONE)
	      return ret;
	  }
	  break;
	case SHT_ARM_ATTRIBUTES:
	case SHT_NOBITS:
	case SHT_NULL:
	case SHT_PROGBITS:
	case SHT_SYMTAB:
	case SHT_STRTAB:
	  break;
	default:
	  {
	    grub_dprintf ("dl", "unhandled section_type: %d (0x%08x)\n",
			  s->sh_type, s->sh_type);
	    return GRUB_ERR_NOT_IMPLEMENTED_YET;
	  };
	}
    }

#undef FIRST_SHDR
#undef NEXT_SHDR

  return GRUB_ERR_NONE;
}
