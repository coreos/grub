/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2007  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/disk.h>

static grub_partition_map_t grub_partition_map_list;

void
grub_partition_map_register (grub_partition_map_t partmap)
{
  partmap->next = grub_partition_map_list;
  grub_partition_map_list = partmap;
}

void
grub_partition_map_unregister (grub_partition_map_t partmap)
{
  grub_partition_map_t *p, q;

  for (p = &grub_partition_map_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == partmap)
      {
        *p = q->next;
	break;
      }
}

int
grub_partition_map_iterate (int (*hook) (const grub_partition_map_t partmap))
{
  grub_partition_map_t p;

  for (p = grub_partition_map_list; p; p = p->next)
    if (hook (p))
      return 1;

  return 0;
}

static grub_partition_t
grub_partition_map_probe (const grub_partition_map_t partmap,
			  grub_disk_t disk, int partnum)
{
  grub_partition_t p = 0;

  auto int find_func (grub_disk_t d, const grub_partition_t partition);

  int find_func (grub_disk_t d __attribute__ ((unused)),
		 const grub_partition_t partition)
    {
      if (partnum == partition->number)
	{
	  p = (grub_partition_t) grub_malloc (sizeof (*p));
	  if (! p)
	    return 1;

	  grub_memcpy (p, partition, sizeof (*p));
	  return 1;
	}

      return 0;
    }

  partmap->iterate (disk, find_func);
  if (grub_errno)
    goto fail;

  return p;

 fail:
  grub_free (p);
  return 0;
}

grub_partition_t
grub_partition_probe (struct grub_disk *disk, const char *str)
{
  grub_partition_t part = 0;
  grub_partition_t curpart = 0;
  grub_partition_t tail;
  const char *ptr;
  int num;

  auto int part_map_probe (const grub_partition_map_t partmap);

  int part_map_probe (const grub_partition_map_t partmap)
    {
      disk->partition = part;
      curpart = grub_partition_map_probe (partmap, disk, num);
      disk->partition = tail;
      if (curpart)
	return 1;

      if (grub_errno == GRUB_ERR_BAD_PART_TABLE)
	{
	  /* Continue to next partition map type.  */
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}

      return 1;
    }

  part = tail = disk->partition;

  for (ptr = str; *ptr;)
    {
      /* BSD-like partition specification.  */
      if (*ptr >= 'a' && *ptr <= 'z')
	num = *(ptr++) - 'a';
      else
	num = grub_strtoul (ptr, (char **) &ptr, 0) - 1;

      curpart = 0;
      /* Use the first partition map type found.  */
      grub_partition_map_iterate (part_map_probe);

      if (! curpart)
	{
	  while (part)
	    {
	      curpart = part->parent;
	      grub_free (part);
	      part = curpart;
	    }
	  return 0;
	}
      curpart->parent = part;
      part = curpart;
      if (! ptr || *ptr != ',')
	break;
      ptr++;
    }

  return part;
}

static grub_partition_map_t
get_partmap (struct grub_disk *disk)
{
  grub_partition_map_t partmap = 0;
  struct grub_partition part;
  int found = 0;
  auto int part_map_iterate (const grub_partition_map_t p);
  auto int part_map_iterate_hook (grub_disk_t d,
				  const grub_partition_t partition);
  int part_map_iterate_hook (grub_disk_t d __attribute__ ((unused)),
			     const grub_partition_t partition)
    {
      found = 1;
      part = *partition;
      return 1;
    }

  int part_map_iterate (const grub_partition_map_t p)
    {
      grub_dprintf ("partition", "Detecting %s...\n", p->name);
      p->iterate (disk, part_map_iterate_hook);

      if (grub_errno != GRUB_ERR_NONE || ! found)
	{
	  /* Continue to next partition map type.  */
	  grub_dprintf ("partition", "%s detection failed.\n", p->name);
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}
      grub_free (part.data);

      grub_dprintf ("partition", "%s detection succeeded.\n", p->name);
      partmap = p;
      return 1;
    }
  grub_partition_map_iterate (part_map_iterate);
  return partmap;
}

int
grub_partition_iterate (struct grub_disk *disk,
			int (*hook) (grub_disk_t disk,
				     const grub_partition_t partition))
{
  int ret = 0;

  auto int part_iterate (grub_disk_t dsk, const grub_partition_t p);

  int part_iterate (grub_disk_t dsk,
		    const grub_partition_t partition)
    {
      struct grub_partition p = *partition;
      grub_partition_map_t partmap = 0;
      p.parent = dsk->partition;
      dsk->partition = 0;
      if (hook (dsk, &p))
	  return 1;
      if (p.start != 0)
	{
	  dsk->partition = &p;
	  partmap = get_partmap (dsk);
	  if (partmap)
	    ret = partmap->iterate (dsk, part_iterate);
	}
      dsk->partition = p.parent;
      return ret;
    }

  {
    grub_partition_map_t partmap = 0;
    partmap = get_partmap (disk);
    if (partmap)
      ret = partmap->iterate (disk, part_iterate);
  }

  return ret;
}

char *
grub_partition_get_name (const grub_partition_t partition)
{
  char *out = 0;
  /* Even on 64-bit machines this buffer is enough to hold longest number.  */
  char buf[25];
  int curlen = 0;
  grub_partition_t part;
  for (part = partition; part; part = part->parent)
    {
      int strl;
      grub_snprintf (buf, sizeof (buf), "%d", part->number + 1);
      strl = grub_strlen (buf);
      if (curlen)
	{
	  out = grub_realloc (out, curlen + strl + 2);
	  grub_memcpy (out + strl + 1, out, curlen);
	  out[curlen + 1 + strl] = 0;
	  grub_memcpy (out, buf, strl);
	  out[strl] = ',';
	  curlen = curlen + 1 + strl;
	}
      else
	{
	  curlen = strl;
	  out = grub_strdup (buf);
	}
    }
  return out;
}
