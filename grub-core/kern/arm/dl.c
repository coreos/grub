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

#ifdef GRUB_UTIL
# include <grub/util/misc.h>
#else

#ifdef DL_DEBUG
static const char *symstrtab;

/*
 * This is a bit of a hack, setting the symstrtab pointer to the last STRTAB
 * section in the module (which is where symbol names are in the objects I've
 * inspected manually). 
 */
static void
set_symstrtab (Elf_Ehdr * e)
{
  int i;
  Elf_Shdr *s;

  symstrtab = NULL;

  for (i = 0, s = (Elf_Shdr *) ((grub_uint32_t) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((grub_uint32_t) s + e->e_shentsize))
    if (s->sh_type == SHT_STRTAB)
      symstrtab = (void *) ((grub_addr_t) e + s->sh_offset);
}

static const char *
get_symbolname (Elf_Sym * sym)
{
  const char *symbolname = symstrtab + sym->st_name;

  return (*symbolname ? symbolname : NULL);
}
#endif /* DL_DEBUG */

/*
 * R_ARM_ABS32
 *
 * Simple relocation of 32-bit value (in literal pool)
 */
static grub_err_t
reloc_abs32 (Elf_Word *target, Elf_Addr sym_addr)
{
  Elf_Addr tmp;

  tmp = *target;
  tmp += sym_addr;
  *target = tmp;
#if 0 //def GRUB_UTIL
  grub_util_info ("  %s:  reloc_abs32 0x%08x => 0x%08x", __FUNCTION__,
		  (unsigned int) sym_addr, (unsigned int) tmp);
#endif

  return GRUB_ERR_NONE;
}
#endif /* ndef GRUB_UTIL */


/********************************************************************
 * Thumb (T32) relocations:                                         *
 *                                                                  *
 * 32-bit Thumb instructions can be 16-bit aligned, and are fetched *
 * little-endian, requiring some additional fiddling.               *
 ********************************************************************/

/*
 * R_ARM_THM_CALL/THM_JUMP24
 *
 * Relocate Thumb (T32) instruction set relative branches:
 *   B.W, BL and BLX
 */
grub_err_t
grub_arm_reloc_thm_call (grub_uint16_t *target, Elf32_Addr sym_addr)
{
  grub_int32_t offset, offset_low, offset_high;
  grub_uint32_t sign, j1, j2, is_blx;
  grub_uint32_t insword, insmask;

  /* Extract instruction word in alignment-safe manner */
  insword = (*target << 16) | (*(target + 1));
  insmask = 0xf800d000;

  /* B.W/BL or BLX? Affects range and expected target state */
  if (((insword >> 12) & 0xd) == 0xc)
    is_blx = 1;
  else
    is_blx = 0;

  /* If BLX, target symbol must be ARM (target address LSB == 0) */
  if (is_blx && (sym_addr & 1))
    return grub_error (GRUB_ERR_BUG,
		       N_("Relocation targeting wrong execution state"));

  offset_low = -16777216;
  offset_high = is_blx ? 16777212 : 16777214;

  /* Extract bitfields from instruction words */
  sign = (insword >> 26) & 1;
  j1 = (insword >> 13) & 1;
  j2 = (insword >> 11) & 1;
  offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
    ((~(j2 ^ sign) & 1) << 22) |
    ((insword & 0x03ff0000) >> 4) | ((insword & 0x000007ff) << 1);

  /* Sign adjust and calculate offset */
  if (offset & (1 << 24))
    offset -= (1 << 25);
#ifdef GRUB_UTIL
  grub_util_info ("    sym_addr = 0x%08x", sym_addr);
#endif
#ifdef GRUB_UTIL
  offset += sym_addr;
#else
  offset += sym_addr - (grub_uint32_t) target;
#endif
#ifdef DEBUG
  grub_printf(" %s: target=0x%08x, sym_addr=0x%08x, offset=%d\n",
	      is_blx ? "BLX" : "BL", (unsigned int) target, sym_addr, offset);
#endif

  if ((offset < offset_low) || (offset > offset_high))
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       N_("THM_CALL Relocation out of range."));

#ifdef GRUB_UTIL
  grub_util_info ("    relative destination = 0x%08lx",
		  (unsigned long)target + offset);
#endif

  /* Reassemble instruction word */
  sign = (offset >> 24) & 1;
  j1 = sign ^ (~(offset >> 23) & 1);
  j2 = sign ^ (~(offset >> 22) & 1);
  insword = (insword & insmask) |
    (sign << 26) |
    (((offset >> 12) & 0x03ff) << 16) |
    (j1 << 13) | (j2 << 11) | ((offset >> 1) & 0x07ff);

  /* Write instruction word back in alignment-safe manner */
  *target = (insword >> 16) & 0xffff;
  *(target + 1) = insword & 0xffff;

#ifdef GRUB_UTIL
#pragma GCC diagnostic ignored "-Wcast-align"
  grub_util_info ("    *target = 0x%08x", *((unsigned int *)target));
#endif

  return GRUB_ERR_NONE;
}

/*
 * R_ARM_THM_JUMP19
 *
 * Relocate conditional Thumb (T32) B<c>.W
 */
grub_err_t
grub_arm_reloc_thm_jump19 (grub_uint16_t *addr, Elf32_Addr sym_addr)
{
  grub_int32_t offset;
  grub_uint32_t insword, insmask;

  /* Extract instruction word in alignment-safe manner */
  insword = (*addr) << 16 | *(addr + 1);
  insmask = 0xfbc0d800;

  /* Extract and sign extend offset */
  offset = ((insword >> 26) & 1) << 18
    | ((insword >> 11) & 1) << 17
    | ((insword >> 13) & 1) << 16
    | ((insword >> 16) & 0x3f) << 11
    | (insword & 0x7ff);
  offset <<= 1;
  if (offset & (1 << 19))
    offset -= (1 << 20);

  /* Adjust and re-truncate offset */
#ifdef GRUB_UTIL
  offset += sym_addr;
#else
  offset += sym_addr - (grub_uint32_t) addr;
#endif
  if ((offset > 1048574) || (offset < -1048576))
    {
      return grub_error
        (GRUB_ERR_OUT_OF_RANGE, N_("THM_JUMP19 Relocation out of range."));
    }

  offset >>= 1;
  offset &= 0x7ffff;

  /* Reassemble instruction word and write back */
  insword &= insmask;
  insword |= ((offset >> 18) & 1) << 26
    | ((offset >> 17) & 1) << 11
    | ((offset >> 16) & 1) << 13
    | ((offset >> 11) & 0x3f) << 16
    | (offset & 0x7ff);
  *addr = insword >> 16;
  *(addr + 1) = insword & 0xffff;
  return GRUB_ERR_NONE;
}



/***********************************************************
 * ARM (A32) relocations:                                  *
 *                                                         *
 * ARM instructions are 32-bit in size and 32-bit aligned. *
 ***********************************************************/

/*
 * R_ARM_JUMP24
 *
 * Relocate ARM (A32) B
 */
grub_err_t
grub_arm_reloc_jump24 (grub_uint32_t *addr, Elf32_Addr sym_addr)
{
  grub_uint32_t insword;
  grub_int32_t offset;

  insword = *addr;

  offset = (insword & 0x00ffffff) << 2;
  if (offset & 0x02000000)
    offset -= 0x04000000;
#ifdef GRUB_UTIL
  offset += sym_addr;
#else
  offset += sym_addr - (grub_uint32_t) addr;
#endif

  insword &= 0xff000000;
  insword |= (offset >> 2) & 0x00ffffff;

  *addr = insword;

  return GRUB_ERR_NONE;
}



/*************************************************
 * Runtime dynamic linker with helper functions. *
 *************************************************/
#ifndef GRUB_UTIL
/*
 * find_segment(): finds a module segment matching sh_info
 */
static grub_dl_segment_t
find_segment (grub_dl_segment_t seg, Elf32_Word sh_info)
{
  for (; seg; seg = seg->next)
    if (seg->section == sh_info)
      return seg;

  return NULL;
}


/*
 * do_relocations():
 *   Iterate over all relocations in section, calling appropriate functions
 *   for patching.
 */
static grub_err_t
do_relocations (Elf_Shdr * relhdr, Elf_Ehdr * e, grub_dl_t mod)
{
  grub_dl_segment_t seg;
  Elf_Rel *rel;
  Elf_Sym *sym;
  int i, entnum;

  entnum = relhdr->sh_size / sizeof (Elf_Rel);

  /* Find the target segment for this relocation section. */
  seg = find_segment (mod->segment, relhdr->sh_info);
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
      target = (Elf_Word *) ((grub_addr_t) seg->addr + rel[i].r_offset);

      sym_addr = sym[relsym].st_value;

#ifdef DL_DEBUG

      grub_printf ("%s: 0x%08x -> %s @ 0x%08x\n", __FUNCTION__,
		   (grub_addr_t) sym_addr, get_symbolname (sym), sym->st_value);
#endif

      switch (reltype)
	{
	case R_ARM_ABS32:
	  {
	    /* Data will be naturally aligned */
	    retval = reloc_abs32 (target, sym_addr);
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

#ifdef DL_DEBUG
  set_symstrtab (e);
#endif

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
	    grub_printf ("unhandled section_type: %d (0x%08x)\n",
			 s->sh_type, s->sh_type);
	    return GRUB_ERR_NOT_IMPLEMENTED_YET;
	  };
	}
    }

#undef FIRST_SHDR
#undef NEXT_SHDR

  return GRUB_ERR_NONE;
}
#endif /* ndef GRUB_UTIL */
