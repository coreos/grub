/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002 Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/machine/partition.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/misc.h>

/* Parse the partition representation in STR and return a partition.  */
static grub_partition_t
grub_partition_parse (const char *str)
{
  grub_partition_t p;
  char *s = (char *) str;
  
  p = (grub_partition_t) grub_malloc (sizeof (*p));
  if (! p)
    return 0;

  /* Initialize some of the fields with invalid values.  */
  p->bsd_part = p->dos_type = p->bsd_type = p->index = -1;

  /* Get the DOS partition number.  */
  p->dos_part = grub_strtoul (s, &s, 0);
  
  if (grub_errno)
    {
      /* Not found. Maybe only a BSD label is specified.  */
      p->dos_part = -1;
      grub_errno = GRUB_ERR_NONE;
    }
  else if (*s == ',')
    s++;

  if (*s)
    {
      if (*s >= 'a' && *s <= 'h')
	{
	  p->bsd_part = *s - 'a';
	  s++;
	}

      if (*s)
	goto fail;
    }

  if (p->dos_part == -1 && p->bsd_part == -1)
    goto fail;

  return p;
  
 fail:
  grub_free (p);
  grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
  return 0;
}

grub_err_t
grub_partition_iterate (grub_disk_t disk,
			int (*hook) (const grub_partition_t partition))
{
  struct grub_partition p;
  struct grub_partition_mbr mbr;
  struct grub_partition_disk_label label;
  struct grub_disk raw;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;
  
  p.offset = 0;
  p.ext_offset = 0;
  p.dos_part = -1;
  
  while (1)
    {
      int i;
      struct grub_partition_entry *e;
      
      /* Read the MBR.  */
      if (grub_disk_read (&raw, p.offset, 0, sizeof (mbr), (char *) &mbr))
	goto finish;

      /* Check if it is valid.  */
      if (mbr.signature != grub_cpu_to_le16 (GRUB_PARTITION_SIGNATURE))
	return grub_error (GRUB_ERR_BAD_PART_TABLE, "no signature");

      /* Analyze DOS partitions.  */
      for (p.index = 0; p.index < 4; p.index++)
	{
	  e = mbr.entries + p.index;
	  
	  p.start = p.offset + grub_le_to_cpu32 (e->start);
	  p.len = grub_le_to_cpu32 (e->length);
	  p.bsd_part = -1;
	  p.dos_type = e->type;
	  p.bsd_type = -1;

	  /* If this partition is a normal one, call the hook.  */
	  if (! grub_partition_is_empty (e->type)
	      && ! grub_partition_is_extended (e->type))
	    {
	      p.dos_part++;
	      
	      if (hook (&p))
		goto finish;

	      /* Check if this is a BSD partition.  */
	      if (grub_partition_is_bsd (e->type))
		{
		  /* Check if the BSD label is within the DOS partition.  */
		  if (p.len <= GRUB_PARTITION_BSD_LABEL_SECTOR)
		    return grub_error (GRUB_ERR_BAD_PART_TABLE,
				       "no space for disk label");

		  /* Read the BSD label.  */
		  if (grub_disk_read (&raw,
				      (p.start
				       + GRUB_PARTITION_BSD_LABEL_SECTOR),
				      0,
				      sizeof (label),
				      (char *) &label))
		    goto finish;

		  /* Check if it is valid.  */
		  if (label.magic
		      != grub_cpu_to_le32 (GRUB_PARTITION_BSD_LABEL_MAGIC))
		    return grub_error (GRUB_ERR_BAD_PART_TABLE,
				       "invalid disk label magic");

		  for (p.bsd_part = 0;
		       p.bsd_part < grub_cpu_to_le16 (label.num_partitions);
		       p.bsd_part++)
		    {
		      struct grub_partition_bsd_entry *be
			= label.entries + p.bsd_part;

		      p.start = grub_le_to_cpu32 (be->offset);
		      p.len = grub_le_to_cpu32 (be->size);
		      p.bsd_type = be->fs_type;
		      
		      if (be->fs_type != GRUB_PARTITION_BSD_TYPE_UNUSED)
			if (hook (&p))
			  goto finish;
		    }
		}
	    }
	  else if (p.dos_part < 4)
	    /* If this partition is a logical one, shouldn't increase the
	       partition number.  */
	    p.dos_part++;
	}

      /* Find an extended partition.  */
      for (i = 0; i < 4; i++)
	{
	  e = mbr.entries + i;
	  
	  if (grub_partition_is_extended (e->type))
	    {
	      p.offset = p.ext_offset + grub_le_to_cpu32 (e->start);
	      if (! p.ext_offset)
		p.ext_offset = p.offset;

	      break;
	    }
	}

      /* If no extended partition, the end.  */
      if (i == 4)
	break;
    }

 finish:
  return grub_errno;
}

grub_partition_t
grub_partition_probe (grub_disk_t disk, const char *str)
{
  grub_partition_t p;
  auto int find_func (const grub_partition_t partition);

  int find_func (const grub_partition_t partition)
    {
      if ((p->dos_part == partition->dos_part || p->dos_part == -1)
	  && p->bsd_part == partition->bsd_part)
	{
	  grub_memcpy (p, partition, sizeof (*p));
	  return 1;
	}
      
      return 0;
    }
  
  p = grub_partition_parse (str);
  if (! p)
    return 0;


  if (grub_partition_iterate (disk, find_func))
    goto fail;

  if (p->index < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "no such partition");
      goto fail;
    }

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

  if (p->bsd_part < 0)
    grub_sprintf (name, "%d", p->dos_part);
  else
    grub_sprintf (name, "%d,%c", p->dos_part, p->bsd_part + 'a');

  return name;
}
