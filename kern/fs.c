/* fs.c - filesystem manager */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/disk.h>
#include <pupa/net.h>
#include <pupa/fs.h>
#include <pupa/file.h>
#include <pupa/err.h>
#include <pupa/misc.h>
#include <pupa/types.h>
#include <pupa/mm.h>
#include <pupa/term.h>

static pupa_fs_t pupa_fs_list;

void
pupa_fs_register (pupa_fs_t fs)
{
  fs->next = pupa_fs_list;
  pupa_fs_list = fs;
}

void
pupa_fs_unregister (pupa_fs_t fs)
{
  pupa_fs_t *p, q;

  for (p = &pupa_fs_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == fs)
      {
	*p = q->next;
	break;
      }
}

void
pupa_fs_iterate (int (*hook) (const pupa_fs_t fs))
{
  pupa_fs_t p;

  for (p = pupa_fs_list; p; p = p->next)
    if (hook (p))
      break;
}

pupa_fs_t
pupa_fs_probe (pupa_device_t device)
{
  pupa_fs_t p;
  auto int dummy_func (const char *filename, int dir);

  int dummy_func (const char *filename __attribute__ ((unused)),
		  int dir __attribute__ ((unused)))
    {
      return 1;
    }

  if (device->disk)
    {
      for (p = pupa_fs_list; p; p = p->next)
	{
	  (p->dir) (device, "/", dummy_func);
	  if (pupa_errno == PUPA_ERR_NONE)
	    return p;
	  
	  if (pupa_errno != PUPA_ERR_BAD_FS)
	    return 0;
	  
	  pupa_errno = PUPA_ERR_NONE;
	}
    }
  else if (device->net->fs)
    return device->net->fs;

  pupa_error (PUPA_ERR_UNKNOWN_FS, "unknown filesystem");
  return 0;
}



/* Block list support routines.  */

struct pupa_fs_block
{
  unsigned long offset;
  unsigned long length;
};

static pupa_err_t
pupa_fs_blocklist_open (pupa_file_t file, const char *name)
{
  char *p = (char *) name;
  unsigned num = 0;
  unsigned i;
  pupa_disk_t disk = file->device->disk;
  struct pupa_fs_block *blocks;
  
  /* First, count the number of blocks.  */
  do
    {
      num++;
      p = pupa_strchr (p, ',');
    }
  while (p);

  /* Allocate a block list.  */
  blocks = pupa_malloc (sizeof (struct pupa_fs_block) * (num + 1));
  if (! blocks)
    return 0;

  file->size = 0;
  p = (char *) name;
  for (i = 0; i < num; i++)
    {
      if (*p != '+')
	{
	  blocks[i].offset = pupa_strtoul (p, &p, 0);
	  if (pupa_errno != PUPA_ERR_NONE || *p != '+')
	    {
	      pupa_error (PUPA_ERR_BAD_FILENAME,
			  "invalid file name `%s'", name);
	      goto fail;
	    }
	}
      else
	blocks[i].offset = 0;

      p++;
      blocks[i].length = pupa_strtoul (p, &p, 0);
      if (pupa_errno != PUPA_ERR_NONE
	  || blocks[i].length == 0
	  || (*p && *p != ',' && ! pupa_isspace (*p)))
	{
	  pupa_error (PUPA_ERR_BAD_FILENAME,
		      "invalid file name `%s'", name);
	  goto fail;
	}

      if (disk->total_sectors < blocks[i].offset + blocks[i].length)
	{
	  pupa_error (PUPA_ERR_BAD_FILENAME, "beyond the total sectors");
	  goto fail;
	}
      
      file->size += (blocks[i].length << PUPA_DISK_SECTOR_BITS);
      p++;
    }

  blocks[i].length = 0;
  file->data = blocks;
  
  return PUPA_ERR_NONE;

 fail:
  pupa_free (blocks);
  return pupa_errno;
}

static pupa_ssize_t
pupa_fs_blocklist_read (pupa_file_t file, char *buf, pupa_ssize_t len)
{
  struct pupa_fs_block *p;
  unsigned long sector;
  unsigned long offset;
  pupa_ssize_t ret = 0;

  if (len > file->size - file->offset)
    len = file->size - file->offset;

  sector = (file->offset >> PUPA_DISK_SECTOR_BITS);
  offset = (file->offset & (PUPA_DISK_SECTOR_SIZE - 1));
  for (p = file->data; p->length && len > 0; p++)
    {
      if (sector < p->length)
	{
	  pupa_ssize_t size;

	  size = len;
	  if (((size + offset + PUPA_DISK_SECTOR_SIZE - 1)
	       >> PUPA_DISK_SECTOR_BITS) > p->length - sector)
	    size = ((p->length - sector) << PUPA_DISK_SECTOR_BITS) - offset;
	  
	  if (pupa_disk_read (file->device->disk, p->offset + sector, offset,
			      size, buf) != PUPA_ERR_NONE)
	    return -1;
	  
	  ret += size;
	  len -= size;
	  sector -= ((size + offset) >> PUPA_DISK_SECTOR_BITS);
	  offset = ((size + offset) & (PUPA_DISK_SECTOR_SIZE - 1));
	}
      else
	sector -= p->length;
    }

  return ret;
}

struct pupa_fs pupa_fs_blocklist =
  {
    .name = "blocklist",
    .dir = 0,
    .open = pupa_fs_blocklist_open,
    .read = pupa_fs_blocklist_read,
    .close = 0,
    .next = 0
  };
