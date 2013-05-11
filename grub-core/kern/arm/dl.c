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
#include <grub/arm/reloc.h>

/*************************************************
 * Runtime dynamic linker with helper functions. *
 *************************************************/
static grub_err_t
do_relocations (Elf_Shdr * relhdr, Elf_Ehdr * e, grub_dl_t mod)
{
  grub_dl_segment_t seg;
  Elf_Rel *rel;
  Elf_Sym *sym;
  int i, entnum;

  entnum = relhdr->sh_size / sizeof (Elf_Rel);

  /* Find the target segment for this relocation section. */
  for (seg = mod->segment ; seg ; seg = seg->next)
    if (seg->section == relhdr->sh_info)
      break;
  if (!seg)
    return grub_error (GRUB_ERR_EOF, N_("relocation segment not found"));

  rel = (Elf_Rel *) ((grub_addr_t) e + relhdr->sh_offset);

  /* Step through all relocations */
  for (i = 0, sym = mod->symtab; i < entnum; i++)
    {
      Elf_Addr *target, sym_addr;
      int relsym, reltype;
      grub_err_t retval;

      if (seg->size < rel[i].r_offset)
	return grub_error (GRUB_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");
      relsym = ELF_R_SYM (rel[i].r_info);
      reltype = ELF_R_TYPE (rel[i].r_info);
      target = (void *) ((grub_addr_t) seg->addr + rel[i].r_offset);

      sym_addr = sym[relsym].st_value;

      switch (reltype)
	{
	case R_ARM_ABS32:
	  {
	    /* Data will be naturally aligned */
	    retval = grub_arm_reloc_abs32 (target, sym_addr);
	    if (retval != GRUB_ERR_NONE)
	      return retval;
	  }
	  break;
	case R_ARM_CALL:
	case R_ARM_JUMP24:
	  {
	    retval = grub_arm_reloc_jump24 (target, sym_addr);
	    if (retval != GRUB_ERR_NONE)
	      return retval;
	  }
	  break;
	case R_ARM_THM_CALL:
	case R_ARM_THM_JUMP24:
	  {
	    /* Thumb instructions can be 16-bit aligned */
	    retval = grub_arm_reloc_thm_call ((grub_uint16_t *) target, sym_addr);
	    if (retval != GRUB_ERR_NONE)
	      return retval;
	  }
	  break;
	case R_ARM_THM_JUMP19:
	  {
	    /* Thumb instructions can be 16-bit aligned */
	    retval = grub_arm_reloc_thm_jump19 ((grub_uint16_t *) target, sym_addr);
	    if (retval != GRUB_ERR_NONE)
	      return retval;
	  }
	  break;
	default:
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     reltype);
	}
    }

  return GRUB_ERR_NONE;
}


/*
 * Check if EHDR is a valid ELF header.
 */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB || e->e_machine != EM_ARM)
    return grub_error (GRUB_ERR_BAD_OS,
		       N_("invalid arch-dependent ELF magic"));

  return GRUB_ERR_NONE;
}

/*
 * Verify that provided ELF header contains reference to a symbol table
 */
static int
has_symtab (Elf_Ehdr * e)
{
  int i;
  Elf_Shdr *s;

  for (i = 0, s = (Elf_Shdr *) ((grub_uint32_t) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((grub_uint32_t) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      return 1;

  return 0;
}

/*
 * grub_arch_dl_relocate_symbols():
 *   Only externally visible function in this file.
 *   Locates the relocations section of the ELF object, and calls
 *   do_relocations() to deal with it.
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
	  {
	    /* Relocations, no addends */
	    ret = do_relocations (s, e, mod);
	    if (ret != GRUB_ERR_NONE)
	      return ret;
	  }
	  break;
	case SHT_NULL:
	case SHT_PROGBITS:
	case SHT_SYMTAB:
	case SHT_STRTAB:
	case SHT_NOBITS:
	case SHT_ARM_ATTRIBUTES:
	  break;
	case SHT_RELA:
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
