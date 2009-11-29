/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/misc.h>

#include <grub/types.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/cache.h>

#include <grub/mips/relocator.h>

/* Remark: doesn't work with source outside of 4G.
   Use relocator64 in this case. 
 */

extern grub_uint8_t grub_relocator32_forward_start;
extern grub_uint8_t grub_relocator32_forward_end;
extern grub_uint8_t grub_relocator32_backward_start;
extern grub_uint8_t grub_relocator32_backward_end;

#define REGW_SIZEOF (2 * sizeof (grub_uint32_t))
#define JUMP_SIZEOF (2 * sizeof (grub_uint32_t))

#define RELOCATOR_SRC_SIZEOF(x) (&grub_relocator32_##x##_end \
				 - &grub_relocator32_##x##_start)
#define RELOCATOR_SIZEOF(x)	(RELOCATOR_SRC_SIZEOF(x) \
				 + REGW_SIZEOF * (31 + 3) + JUMP_SIZEOF)
#define RELOCATOR_ALIGN 16

#define PREFIX(x) grub_relocator32_ ## x

static void
write_reg (int regn, grub_uint32_t val, void **target)
{
  /* lui $r, (val+0x8000).  */
  *(grub_uint32_t *) *target = ((0x3c00 | regn) << 16) | ((val + 0x8000) >> 16);
  *target = ((grub_uint32_t *) *target) + 1;
  /* addiu $r, $r, val. */
  *(grub_uint32_t *) *target = (((0x2400 | regn << 5 | regn) << 16)
				| (val & 0xffff));
  *target = ((grub_uint32_t *) *target) + 1;
}

static void
write_jump (int regn, void **target)
{
  /* j $r.  */
  *(grub_uint32_t *) *target = (regn<<21) | 0x8;
  *target = ((grub_uint32_t *) *target) + 1;
  /* nop.  */
  *(grub_uint32_t *) *target = 0;
  *target = ((grub_uint32_t *) *target) + 1;
}

static void
write_call_relocator_bw (void *ptr0, void *src, grub_uint32_t dest,
			 grub_size_t size, struct grub_relocator32_state state)
{
  void *ptr = ptr0;
  int i;
  write_reg (8, (grub_uint32_t) src, &ptr);
  write_reg (9, dest, &ptr);
  write_reg (10, size, &ptr);
  grub_memcpy (ptr, &grub_relocator32_backward_start,
	       RELOCATOR_SRC_SIZEOF (backward));
  ptr = (grub_uint8_t *) ptr + RELOCATOR_SRC_SIZEOF (backward);
  for (i = 1; i < 32; i++)
    write_reg (i, state.gpr[i], &ptr);
  write_jump (state.jumpreg, &ptr);
  grub_arch_sync_caches (ptr0, (grub_uint8_t *) ptr -  (grub_uint8_t *) ptr0);
  grub_dprintf ("relocator", "Backward relocator: about to jump to %p\n", ptr0);
  ((void (*) (void)) ptr0) ();
}

static void
write_call_relocator_fw (void *ptr0, void *src, grub_uint32_t dest,
			 grub_size_t size, struct grub_relocator32_state state)
{
  void *ptr = ptr0;
  int i;
  write_reg (8, (grub_uint32_t) src, &ptr);
  write_reg (9, dest, &ptr);
  write_reg (10, size, &ptr);
  grub_memcpy (ptr, &grub_relocator32_forward_start,
	       RELOCATOR_SRC_SIZEOF (forward));
  ptr = (grub_uint8_t *) ptr + RELOCATOR_SRC_SIZEOF (forward);
  for (i = 1; i < 32; i++)
    write_reg (i, state.gpr[i], &ptr);
  write_jump (state.jumpreg, &ptr);
  grub_arch_sync_caches (ptr0, (grub_uint8_t *) ptr -  (grub_uint8_t *) ptr0);
  grub_dprintf ("relocator", "Forward relocator: about to jump to %p\n", ptr0);
  ((void (*) (void)) ptr0) ();
}

#include "../relocator.c"
