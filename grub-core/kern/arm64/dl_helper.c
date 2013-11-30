/* dl_helper.c - relocation helper functions for modules and grub-mkimage */
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
#include <grub/arm64/reloc.h>

static grub_ssize_t
sign_compress_offset (grub_ssize_t offset, int bitpos)
{
  return offset & ((1LL << (bitpos + 1)) - 1);
}

/*
 * grub_arm64_reloc_xxxx26():
 *
 * JUMP26/CALL26 relocations for B and BL instructions.
 */

grub_err_t
grub_arm64_reloc_xxxx26 (grub_uint32_t *place, Elf64_Addr adjust)
{
  grub_uint32_t insword, insmask;
  grub_ssize_t offset;
  const grub_ssize_t offset_low = -(1 << 27), offset_high = (1 << 27) - 1;

  insword = grub_le_to_cpu32 (*place);
  insmask = 0xfc000000;

  offset = adjust;
#ifndef GRUB_UTIL
  offset -= (grub_addr_t) place;
#endif

  if ((offset < offset_low) || (offset > offset_high))
    {
      return grub_error (GRUB_ERR_BAD_MODULE,
			 N_("CALL26 Relocation out of range"));
    }

  grub_dprintf ("dl", "  reloc_xxxx64 %p %c= 0x%llx\n",
		place, offset > 0 ? '+' : '-',
		offset < 0 ? (long long) -(unsigned long long) offset : offset);

  offset = sign_compress_offset (offset, 27) >> 2;

  *place = grub_cpu_to_le32 ((insword & insmask) | offset);

  return GRUB_ERR_NONE;
}
