/* partiton.c - Read macintosh partition tables.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/disk.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/machine/partition.h>

pupa_err_t
pupa_partition_iterate (pupa_disk_t disk,
			int (*hook) (const pupa_partition_t partition))
{
  struct pupa_partition part;
  struct pupa_apple_part apart;
  struct pupa_disk raw;
  int partno = 0;
  int pos = PUPA_DISK_SECTOR_SIZE * 2;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;

  for (;;)
    {
      if (pupa_disk_read (&raw, pos / PUPA_DISK_SECTOR_SIZE,
		      pos % PUPA_DISK_SECTOR_SIZE,
			  sizeof (struct pupa_apple_part),  (char *) &apart))
	return pupa_errno;

      if (apart.magic !=  PUPA_APPLE_PART_MAGIC)
	break;

      part.start = apart.first_phys_block;
      part.len = apart.blockcnt;
      part.offset = pos;
      part.index = partno;

      if (hook (&part))
	return pupa_errno;

      if (apart.first_phys_block == PUPA_DISK_SECTOR_SIZE * 2)
	return 0;

      pos += sizeof (struct pupa_apple_part);
      partno++;
    }

  return 0;
}

pupa_partition_t
pupa_partition_probe (pupa_disk_t disk, const char *str)
{
  pupa_partition_t p;
  int partnum = 0;
  char *s = (char *) str;

  int find_func (const pupa_partition_t partition)
    {
      if (partnum == partition->index)
	{
	  p = (pupa_partition_t) pupa_malloc (sizeof (*p));
	  if (! p)
	    return 1;
	  
	  pupa_memcpy (p, partition, sizeof (*p));
	  return 1;
	}
      
      return 0;
    }
  
  /* Get the partition number.  */
  partnum = pupa_strtoul (s, &s, 0);
  if (pupa_errno)
    {
      pupa_error (PUPA_ERR_BAD_FILENAME, "invalid partition");
      return 0;
    }

  if (pupa_partition_iterate (disk, find_func))
    goto fail;

  return p;

 fail:
  pupa_free (p);
  return 0;

}

char *
pupa_partition_get_name (const pupa_partition_t p)
{
  char *name;

  name = pupa_malloc (13);
  if (! name)
    return 0;

  pupa_sprintf (name, "%d", p->index);
  return name;
}
