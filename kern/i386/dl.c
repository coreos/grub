/* dl-386.c - arch-dependent part of loadable module support */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/dl.h>
#include <pupa/elf.h>
#include <pupa/misc.h>
#include <pupa/err.h>

/* Check if EHDR is a valid ELF header.  */
int
pupa_arch_dl_check_header (void *ehdr, unsigned size)
{
  Elf32_Ehdr *e = ehdr;

  /* Check the header size.  */
  if (size < sizeof (Elf32_Ehdr))
    return 0;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_version != EV_CURRENT
      || e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_386
      || e->e_type != ET_REL)
    return 0;

  /* Make sure that every section is within the core.  */
  if (size < e->e_shoff + e->e_shentsize * e->e_shnum)
    return 0;

  return 1;
}

/* Relocate symbols.  */
pupa_err_t
pupa_arch_dl_relocate_symbols (pupa_dl_t mod, void *ehdr)
{
  Elf32_Ehdr *e = ehdr;
  Elf32_Shdr *s;
  Elf32_Sym *symtab;
  Elf32_Word entsize;
  unsigned i;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf32_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf32_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return pupa_error (PUPA_ERR_BAD_MODULE, "no symtab found");
  
  symtab = (Elf32_Sym *) ((char *) e + s->sh_offset);
  entsize = s->sh_entsize;

  for (i = 0, s = (Elf32_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf32_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	pupa_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf32_Rel *rel, *max;
	    
	    for (rel = (Elf32_Rel *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		Elf32_Word *addr;
		Elf32_Sym *sym;
		
		if (seg->size < rel->r_offset)
		  return pupa_error (PUPA_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");
		
		addr = (Elf32_Word *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf32_Sym *) ((char *) symtab
				     + entsize * ELF32_R_SYM (rel->r_info));
		
		switch (ELF32_R_TYPE (rel->r_info))
		  {
		  case R_386_32:
		    *addr += sym->st_value;
		    break;

		  case R_386_PC32:
		    *addr += (sym->st_value - (Elf32_Word) seg->addr
			      - rel->r_offset);
		    break;
		  }
	      }
	  }
      }

  return PUPA_ERR_NONE;
}
