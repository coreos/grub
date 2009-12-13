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

#define MAX_OVERHEAD ((RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN) \
		      + (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN) \
		      + (RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)	\
		      + (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN))
#define PRE_REGION_SIZE (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN)

void *
PREFIX (alloc) (grub_size_t size)
{
  char *playground;

  playground = grub_malloc (size + MAX_OVERHEAD);
  if (!playground)
    return 0;

  *(grub_size_t *) playground = size;

  return playground + PRE_REGION_SIZE;
}

void *
PREFIX (realloc) (void *relocator, grub_size_t size)
{
  char *playground;

  if (!relocator)
    return PREFIX (alloc) (size);

  playground = (char *) relocator - PRE_REGION_SIZE;

  playground = grub_realloc (playground, size + MAX_OVERHEAD);
  if (!playground)
    return 0;

  *(grub_size_t *) playground = size;

  return playground + PRE_REGION_SIZE;
}

void
PREFIX(free) (void *relocator)
{
  if (relocator)
    grub_free ((char *) relocator - PRE_REGION_SIZE);
}

grub_err_t
PREFIX (boot) (void *relocator, grub_uint32_t dest,
	       struct grub_relocator32_state state)
{
  grub_size_t size;
  char *playground;

  playground = (char *) relocator - PRE_REGION_SIZE;
  size = *(grub_size_t *) playground;

  grub_dprintf ("relocator",
		"Relocator: source: %p, destination: 0x%x, size: 0x%lx\n",
		relocator, (unsigned) dest, (unsigned long) size);

  /* Very unlikely condition: Relocator may risk overwrite itself.
     Just move it a bit up.  */
  if ((grub_addr_t) dest < (grub_addr_t) relocator
      + (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN) 
      && (grub_addr_t) dest + (RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)
      > (grub_addr_t) relocator)
    {
      void *relocator_new = ((grub_uint8_t *) relocator)
	+ (RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)
	+ (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN);
      grub_dprintf ("relocator", "Overwrite condition detected moving "
		    "relocator from %p to %p\n", relocator, relocator_new);
      grub_memmove (relocator_new, relocator,
		    (RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)
		    + size
		    + (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN));
      relocator = relocator_new;
    }

  if ((grub_addr_t) dest >= (grub_addr_t) relocator)
    {
      int overhead;
      overhead = dest -
	ALIGN_UP (dest - RELOCATOR_SIZEOF (backward) - RELOCATOR_ALIGN,
		  RELOCATOR_ALIGN);
      grub_dprintf ("relocator",
		    "Backward relocator: code %p, source: %p, "
		    "destination: 0x%x, size: 0x%lx\n",
		    (char *) relocator - overhead,
		    (char *) relocator - overhead, 
		    (unsigned) dest - overhead,
		    (unsigned long) size + overhead);

      write_call_relocator_bw ((char *) relocator - overhead,
			       (char *) relocator - overhead,
			       dest - overhead, size + overhead, state);
    }
  else
    {
      int overhead;

      overhead = ALIGN_UP (dest + size, RELOCATOR_ALIGN)
	+ RELOCATOR_SIZEOF (forward) - (dest + size);
      grub_dprintf ("relocator",
		    "Forward relocator: code %p, source: %p, "
		    "destination: 0x%x, size: 0x%lx\n",
		    (char *) relocator + size + overhead
		    - RELOCATOR_SIZEOF (forward),
		    relocator, (unsigned) dest,
		    (unsigned long) size + overhead);

      write_call_relocator_fw ((char *) relocator + size + overhead
			       - RELOCATOR_SIZEOF (forward),
			       relocator, dest, size + overhead, state);
    }

  /* Not reached.  */
  return GRUB_ERR_NONE;
}
