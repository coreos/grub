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

#include <grub/i386/relocator.h>

extern grub_uint8_t grub_relocator32_forward_start;
extern grub_uint8_t grub_relocator32_forward_end;
extern grub_uint8_t grub_relocator32_backward_start;
extern grub_uint8_t grub_relocator32_backward_end;

extern grub_uint32_t grub_relocator32_backward_dest;
extern grub_uint32_t grub_relocator32_backward_size;
#ifdef __x86_64__
extern grub_uint64_t grub_relocator32_backward_src;
#else
extern grub_uint32_t grub_relocator32_backward_src;
#endif

extern grub_uint32_t grub_relocator32_forward_dest;
extern grub_uint32_t grub_relocator32_forward_size;
#ifdef __x86_64__
extern grub_uint64_t grub_relocator32_forward_src;
#else
extern grub_uint32_t grub_relocator32_forward_src;
#endif

extern grub_uint32_t grub_relocator32_forward_eax;
extern grub_uint32_t grub_relocator32_forward_ebx;
extern grub_uint32_t grub_relocator32_forward_ecx;
extern grub_uint32_t grub_relocator32_forward_edx;
extern grub_uint32_t grub_relocator32_forward_eip;
extern grub_uint32_t grub_relocator32_forward_esp;

extern grub_uint32_t grub_relocator32_backward_eax;
extern grub_uint32_t grub_relocator32_backward_ebx;
extern grub_uint32_t grub_relocator32_backward_ecx;
extern grub_uint32_t grub_relocator32_backward_edx;
extern grub_uint32_t grub_relocator32_backward_eip;
extern grub_uint32_t grub_relocator32_backward_esp;

#define RELOCATOR_SIZEOF(x)	(&grub_relocator32_##x##_end - &grub_relocator32_##x##_start)
#define RELOCATOR_ALIGN 16

void *
grub_relocator32_alloc (grub_size_t size)
{
  char *playground;

  playground = grub_malloc ((RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)
			    + size
			    + (RELOCATOR_SIZEOF (backward) +
			       RELOCATOR_ALIGN));
  if (!playground)
    return 0;

  *(grub_size_t *) playground = size;

  return playground + RELOCATOR_SIZEOF (forward);
}

void
grub_relocator32_free (void *relocator)
{
  if (relocator)
    grub_free ((char *) relocator - RELOCATOR_SIZEOF (forward));
}


grub_err_t
grub_relocator32_boot (void *relocator, grub_uint32_t dest,
		       struct grub_relocator32_state state)
{
  grub_size_t size;
  char *playground;
  void (*entry) ();

  playground = (char *) relocator - RELOCATOR_SIZEOF (forward);
  size = *(grub_size_t *) playground;

  if (UINT_TO_PTR (dest) >= relocator)
    {
      int overhead;

      overhead =
	ALIGN_UP (dest - RELOCATOR_SIZEOF (backward) - RELOCATOR_ALIGN,
		  RELOCATOR_ALIGN);
      grub_relocator32_backward_dest = dest - overhead;
      grub_relocator32_backward_src = PTR_TO_UINT64 (relocator - overhead);
      grub_relocator32_backward_size = size + overhead;

      grub_relocator32_backward_eax = state.eax;
      grub_relocator32_backward_ebx = state.ebx;
      grub_relocator32_backward_ecx = state.ecx;
      grub_relocator32_backward_edx = state.edx;
      grub_relocator32_backward_eip = state.eip;
      grub_relocator32_backward_esp = state.esp;

      grub_memmove (relocator - overhead,
		    &grub_relocator32_backward_start,
		    RELOCATOR_SIZEOF (backward));
      entry = (void (*)()) (relocator - overhead);
    }
  else
    {
      int overhead;

      overhead = ALIGN_UP (dest + size, RELOCATOR_ALIGN)
	+ RELOCATOR_SIZEOF (forward) - (dest + size);

      grub_relocator32_forward_dest = dest;
      grub_relocator32_forward_src = PTR_TO_UINT64 (relocator);
      grub_relocator32_forward_size = size + overhead;

      grub_relocator32_forward_eax = state.eax;
      grub_relocator32_forward_ebx = state.ebx;
      grub_relocator32_forward_ecx = state.ecx;
      grub_relocator32_forward_edx = state.edx;
      grub_relocator32_forward_eip = state.eip;
      grub_relocator32_forward_esp = state.esp;

      grub_memmove (relocator + size + overhead - RELOCATOR_SIZEOF (forward),
		    &grub_relocator32_forward_start,
		    RELOCATOR_SIZEOF (forward));
      entry =
	(void (*)()) (relocator + size + overhead -
		      RELOCATOR_SIZEOF (forward));
    }
  entry ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}
