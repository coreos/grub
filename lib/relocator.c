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

void *
PREFIX (alloc) (grub_size_t size)
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

void *
PREFIX (realloc) (void *relocator, grub_size_t size)
{
  char *playground;

  playground = (char *) relocator - RELOCATOR_SIZEOF (forward);

  playground = grub_realloc (playground,
			     (RELOCATOR_SIZEOF (forward) + RELOCATOR_ALIGN)
			     + size
			     + (RELOCATOR_SIZEOF (backward) + RELOCATOR_ALIGN));
  if (!playground)
    return 0;

  *(grub_size_t *) playground = size;

  return playground + RELOCATOR_SIZEOF (forward);
}

void
PREFIX(free) (void *relocator)
{
  if (relocator)
    grub_free ((char *) relocator - RELOCATOR_SIZEOF (forward));
}

grub_err_t
PREFIX (boot) (void *relocator, grub_uint32_t dest,
	       struct grub_relocator32_state state)
{
  grub_size_t size;
  char *playground;

  playground = (char *) relocator - RELOCATOR_SIZEOF (forward);
  size = *(grub_size_t *) playground;

  grub_dprintf ("relocator",
		"Relocator: source: %p, destination: 0x%x, size: 0x%x\n",
		relocator, dest, size);

  if (UINT_TO_PTR (dest) >= relocator)
    {
      int overhead;
      overhead =
	ALIGN_UP (dest - RELOCATOR_SIZEOF (backward) - RELOCATOR_ALIGN,
		  RELOCATOR_ALIGN);
      grub_dprintf ("relocator",
		    "Backward relocator: code %p, source: %p, "
		    "destination: 0x%x, size: 0x%x\n",
		    (char *) relocator - overhead,
		    (char *) relocator - overhead, 
		    dest - overhead, size + overhead);

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
		    "destination: 0x%x, size: 0x%x\n",
		    (char *) relocator + size + overhead
		    - RELOCATOR_SIZEOF (forward),
		    relocator, dest, size + overhead);

      write_call_relocator_fw ((char *) relocator + size + overhead
			       - RELOCATOR_SIZEOF (forward),
			       relocator, dest, size + overhead, state);
    }

  /* Not reached.  */
  return GRUB_ERR_NONE;
}
