/* partiton.c - Read macintosh partition tables.  */
/*
 *  GRUB  --  GRand Unified Bootloader
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

#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/partition.h>

grub_err_t
grub_partition_iterate (grub_disk_t disk,
			int (*hook) (const grub_partition_t partition))
{
  struct grub_partition part;
  struct grub_apple_part apart;
  struct grub_disk raw;
  int partno = 0;
  int pos = GRUB_DISK_SECTOR_SIZE;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;

  for (;;)
    {
      if (grub_disk_read (&raw, pos / GRUB_DISK_SECTOR_SIZE,
		      pos % GRUB_DISK_SECTOR_SIZE,
			  sizeof (struct grub_apple_part),  (char *) &apart))
	return grub_errno;

      if (apart.magic !=  GRUB_APPLE_PART_MAGIC)
	break;

      part.start = apart.first_phys_block;
      part.len = apart.blockcnt;
      part.offset = pos;
      part.index = partno;

      if (hook (&part))
	return grub_errno;

      if (apart.first_phys_block == GRUB_DISK_SECTOR_SIZE * 2)
	return 0;

      pos += sizeof (struct grub_apple_part);
      partno++;
    }

  return 0;
}

grub_partition_t
grub_partition_probe (grub_disk_t disk, const char *str)
{
  grub_partition_t p = 0;
  int partnum = 0;
  char *s = (char *) str;

  int find_func (const grub_partition_t partition)
    {
      if (partnum == partition->index)
	{
	  p = (grub_partition_t) grub_malloc (sizeof (*p));
	  if (! p)
	    return 1;
	  
	  grub_memcpy (p, partition, sizeof (*p));
	  return 1;
	}
      
      return 0;
    }
  
  /* Get the partition number.  */
  partnum = grub_strtoul (s, 0, 10);
  if (grub_errno)
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
      return 0;
    }

  if (grub_partition_iterate (disk, find_func))
    goto fail;

  return p;

 fail:
  grub_free (p);
  return 0;

}

char *
grub_partition_get_name (const grub_partition_t p)
{
  char *name;

  name = grub_malloc (13);
  if (! name)
    return 0;

  grub_sprintf (name, "%d", p->index);
  return name;
}
