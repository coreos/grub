/* dl.c - arch-dependent part of loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2005,2007,2009  Free Software Foundation, Inc.
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

void
grub_ia64_dl_get_tramp_got_size (const void *ehdr, grub_size_t *tramp,
				 grub_size_t *got)
{
  const Elf_Ehdr *e = ehdr;
  grub_size_t cntt = 0, cntg = 0;;
  const Elf_Shdr *s;
  Elf_Word entsize;
  unsigned i;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return;

  entsize = s->sh_entsize;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_RELA)
      {
	Elf_Rela *rel, *max;

	for (rel = (Elf_Rela *) ((char *) e + s->sh_offset),
	       max = rel + s->sh_size / s->sh_entsize;
	     rel < max; rel++)
	  switch (ELF_R_TYPE (rel->r_info))
	    {
	    case R_IA64_PCREL21B:
	      cntt++;
	      break;
	    case R_IA64_LTOFF_FPTR22:
	    case R_IA64_LTOFF22X:
	    case R_IA64_LTOFF22:
	      cntg++;
	      break;
	    }
      }
  *tramp = cntt;
  *got = cntg;
}

