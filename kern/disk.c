/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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
#include <pupa/err.h>
#include <pupa/mm.h>
#include <pupa/types.h>
#include <pupa/machine/partition.h>
#include <pupa/misc.h>
#include <pupa/machine/time.h>
#include <pupa/file.h>

#define	PUPA_CACHE_TIMEOUT	2

/* The last time the disk was used.  */
static unsigned long pupa_last_time = 0;


/* Disk cache.  */
struct pupa_disk_cache
{
  unsigned long id;
  unsigned long sector;
  char *data;
  int lock;
};

static struct pupa_disk_cache pupa_disk_cache_table[PUPA_DISK_CACHE_NUM];

#if 0
static unsigned long pupa_disk_cache_hits;
static unsigned long pupa_disk_cache_misses;

void
pupa_disk_cache_get_performance (unsigned long *hits, unsigned long *misses)
{
  *hits = pupa_disk_cache_hits;
  *misses = pupa_disk_cache_misses;
}
#endif

static unsigned
pupa_disk_cache_get_index (unsigned long id, unsigned long sector)
{
  return ((id * 2606459 + (sector >> PUPA_DISK_CACHE_BITS))
	  % PUPA_DISK_CACHE_NUM);
}

static void
pupa_disk_cache_invalidate (unsigned long id, unsigned long sector)
{
  unsigned index;
  struct pupa_disk_cache *cache;

  sector &= ~(PUPA_DISK_CACHE_SIZE - 1);
  index = pupa_disk_cache_get_index (id, sector);
  cache = pupa_disk_cache_table + index;

  if (cache->id == id && cache->sector == sector && cache->data)
    {
      cache->lock = 1;
      pupa_free (cache->data);
      cache->data = 0;
      cache->lock = 0;
    }
}

void
pupa_disk_cache_invalidate_all (void)
{
  unsigned i;

  for (i = 0; i < PUPA_DISK_CACHE_NUM; i++)
    {
      struct pupa_disk_cache *cache = pupa_disk_cache_table + i;

      if (cache->data && ! cache->lock)
	{
	  pupa_free (cache->data);
	  cache->data = 0;
	}
    }
}

static char *
pupa_disk_cache_fetch (unsigned long id, unsigned long sector)
{
  struct pupa_disk_cache *cache;
  unsigned index;

  index = pupa_disk_cache_get_index (id, sector);
  cache = pupa_disk_cache_table + index;

  if (cache->id == id && cache->sector == sector)
    {
      cache->lock = 1;
#if 0
      pupa_disk_cache_hits++;
#endif
      return cache->data;
    }

#if 0
  pupa_disk_cache_misses++;
#endif
  
  return 0;
}

static void
pupa_disk_cache_unlock (unsigned long id, unsigned long sector)
{
  struct pupa_disk_cache *cache;
  unsigned index;

  index = pupa_disk_cache_get_index (id, sector);
  cache = pupa_disk_cache_table + index;

  if (cache->id == id && cache->sector == sector)
    cache->lock = 0;
}

static pupa_err_t
pupa_disk_cache_store (unsigned long id, unsigned long sector,
		       const char *data)
{
  unsigned index;
  struct pupa_disk_cache *cache;
  
  pupa_disk_cache_invalidate (id, sector);
  
  index = pupa_disk_cache_get_index (id, sector);
  cache = pupa_disk_cache_table + index;
  
  cache->data = pupa_malloc (PUPA_DISK_SECTOR_SIZE << PUPA_DISK_CACHE_BITS);
  if (! cache->data)
    return pupa_errno;
  
  pupa_memcpy (cache->data, data,
	       PUPA_DISK_SECTOR_SIZE << PUPA_DISK_CACHE_BITS);
  cache->id = id;
  cache->sector = sector;

  return PUPA_ERR_NONE;
}



static pupa_disk_dev_t pupa_disk_dev_list;

void
pupa_disk_dev_register (pupa_disk_dev_t dev)
{
  dev->next = pupa_disk_dev_list;
  pupa_disk_dev_list = dev;
}

void
pupa_disk_dev_unregister (pupa_disk_dev_t dev)
{
  pupa_disk_dev_t *p, q;
  
  for (p = &pupa_disk_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}

void
pupa_disk_dev_iterate (int (*hook) (const char *name))
{
  pupa_disk_dev_t p;

  for (p = pupa_disk_dev_list; p; p = p->next)
    if ((p->iterate) (hook))
      break;
}

pupa_disk_t
pupa_disk_open (const char *name)
{
  char *p;
  pupa_disk_t disk;
  pupa_disk_dev_t dev;
  char *raw = (char *) name;
  unsigned long current_time;
  
  disk = (pupa_disk_t) pupa_malloc (sizeof (*disk));
  if (! disk)
    return 0;

  disk->dev = 0;
  disk->read_hook = 0;
  disk->partition = 0;
  disk->data = 0;
  disk->name = pupa_strdup (name);
  if (! disk->name)
    goto fail;
  
  p = pupa_strchr (name, ',');
  if (p)
    {
      pupa_size_t len = p - name;
      
      raw = pupa_malloc (len + 1);
      if (! raw)
	goto fail;

      pupa_memcpy (raw, name, len);
      raw[len] = '\0';
    }

  for (dev = pupa_disk_dev_list; dev; dev = dev->next)
    {
      if ((dev->open) (raw, disk) == PUPA_ERR_NONE)
	break;
      else if (pupa_errno == PUPA_ERR_UNKNOWN_DEVICE)
	pupa_errno = PUPA_ERR_NONE;
      else
	goto fail;
    }

  if (! dev)
    {
      pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "no such disk");
      goto fail;
    }

  if (p && ! disk->has_partitions)
    {
      pupa_error (PUPA_ERR_BAD_DEVICE, "no partition on this disk");
      goto fail;
    }
  
  disk->dev = dev;

  if (p)
    disk->partition = pupa_partition_probe (disk, p + 1);

  /* The cache will be invalidated about 2 seconds after a device was
     closed.  */
  current_time = pupa_get_rtc ();

  if (current_time > pupa_last_time + PUPA_CACHE_TIMEOUT * PUPA_TICKS_PER_SECOND)
    pupa_disk_cache_invalidate_all ();
  
  pupa_last_time = current_time;
  
 fail:
  
  if (raw && raw != name)
    pupa_free (raw);

  if (pupa_errno != PUPA_ERR_NONE)
    {
      pupa_disk_close (disk);
      return 0;
    }

  return disk;
}

void
pupa_disk_close (pupa_disk_t disk)
{
  if (disk->dev && disk->dev->close)
    (disk->dev->close) (disk);

  /* Reset the timer.  */
  pupa_last_time = pupa_get_rtc ();

  pupa_free (disk->partition);
  pupa_free ((void *) disk->name);
  pupa_free (disk);
}

static pupa_err_t
pupa_disk_check_range (pupa_disk_t disk, unsigned long *sector,
		       unsigned long *offset, pupa_ssize_t size)
{
  *sector += *offset >> PUPA_DISK_SECTOR_BITS;
  *offset &= PUPA_DISK_SECTOR_SIZE - 1;
  
  if (disk->partition)
    {
      unsigned long start, len;

      start = pupa_partition_get_start (disk->partition);
      len = pupa_partition_get_len (disk->partition);

      if (*sector >= len
	  || len - *sector < ((*offset + size + PUPA_DISK_SECTOR_SIZE - 1)
			      >> PUPA_DISK_SECTOR_BITS))
	return pupa_error (PUPA_ERR_OUT_OF_RANGE, "out of partition");

      *sector += start;
    }

  if (disk->total_sectors <= *sector
      || ((*offset + size + PUPA_DISK_SECTOR_SIZE - 1)
	  >> PUPA_DISK_SECTOR_BITS) > disk->total_sectors - *sector)
    return pupa_error (PUPA_ERR_OUT_OF_RANGE, "out of disk");

  return PUPA_ERR_NONE;
}

/* Read data from the disk.  */
pupa_err_t
pupa_disk_read (pupa_disk_t disk, unsigned long sector,
		unsigned long offset, unsigned long size, char *buf)
{
  char *tmp_buf;

  /* First of all, check if the region is within the disk.  */
  if (pupa_disk_check_range (disk, &sector, &offset, size) != PUPA_ERR_NONE)
    return pupa_errno;

  /* Allocate a temporary buffer.  */
  tmp_buf = pupa_malloc (PUPA_DISK_SECTOR_SIZE << PUPA_DISK_CACHE_BITS);
  if (! tmp_buf)
    return pupa_errno;

  /* Until SIZE is zero...  */
  while (size)
    {
      char *data;
      unsigned long start_sector;
      unsigned long len;
      unsigned long pos;

      /* For reading bulk data.  */
      start_sector = sector & ~(PUPA_DISK_CACHE_SIZE - 1);
      pos = (sector - start_sector) << PUPA_DISK_SECTOR_BITS;
      len = (PUPA_DISK_SECTOR_SIZE << PUPA_DISK_CACHE_BITS) - pos - offset;
      if (len > size)
	len = size;

      /* Fetch the cache.  */
      data = pupa_disk_cache_fetch (disk->id, start_sector);
      if (data)
	{
	  /* Just copy it!  */
	  pupa_memcpy (buf, data + pos + offset, len);
	  pupa_disk_cache_unlock (disk->id, start_sector);
	}
      else
	{
	  /* Otherwise read data from the disk actually.  */
	  if ((disk->dev->read) (disk, start_sector,
				 PUPA_DISK_CACHE_SIZE, tmp_buf)
	      != PUPA_ERR_NONE)
	    {
	      /* Uggh... Failed. Instead, just read necessary data.  */
	      unsigned num;

	      pupa_errno = PUPA_ERR_NONE;

	      /* If more data is required, no way.  */
	      if (pos + size
		  >= (PUPA_DISK_SECTOR_SIZE << PUPA_DISK_CACHE_BITS))
		goto finish;

	      num = ((size + PUPA_DISK_SECTOR_SIZE - 1)
		     >> PUPA_DISK_SECTOR_BITS);
	      if ((disk->dev->read) (disk, sector, num, tmp_buf))
		goto finish;

	      pupa_memcpy (buf, tmp_buf + offset, size);

	      /* Call the read hook, if any.  */
	      if (disk->read_hook)
		while (size)
		  {
		    (disk->read_hook) (sector, offset,
				       ((size > PUPA_DISK_SECTOR_SIZE)
					? PUPA_DISK_SECTOR_SIZE
					: size));
		    sector++;
		    size -= PUPA_DISK_SECTOR_SIZE - offset;
		    offset = 0;
		  }

	      /* This must be the end.  */
	      goto finish;
	    }

	  /* Copy it and store it in the disk cache.  */
	  pupa_memcpy (buf, tmp_buf + pos + offset, len);
	  pupa_disk_cache_store (disk->id, start_sector, tmp_buf);
	}

      /* Call the read hook, if any.  */
      if (disk->read_hook)
	{
	  unsigned long s = sector;
	  unsigned long l = len;
	  
	  while (l)
	    {
	      (disk->read_hook) (s, offset,
				 ((l > PUPA_DISK_SECTOR_SIZE)
				  ? PUPA_DISK_SECTOR_SIZE
				  : l));
	      
	      if (l < PUPA_DISK_SECTOR_SIZE - offset)
		break;
	      
	      s++;
	      l -= PUPA_DISK_SECTOR_SIZE - offset;
	      offset = 0;
	    }
	}
      
      sector = start_sector + PUPA_DISK_CACHE_SIZE;
      buf += len;
      size -= len;
      offset = 0;
    }
  
 finish:
  
  pupa_free (tmp_buf);
  
  return pupa_errno;
}

pupa_err_t
pupa_disk_write (pupa_disk_t disk, unsigned long sector,
		 unsigned long offset, unsigned long size, const char *buf)
{
  if (pupa_disk_check_range (disk, &sector, &offset, size) != PUPA_ERR_NONE)
    return -1;

  while (size)
    {
      if (offset != 0 || (size < PUPA_DISK_SECTOR_SIZE && size != 0))
	{
	  char tmp_buf[PUPA_DISK_SECTOR_SIZE];
	  unsigned long len;
	  
	  if (pupa_disk_read (disk, sector, 0, PUPA_DISK_SECTOR_SIZE, tmp_buf)
	      != PUPA_ERR_NONE)
	    goto finish;

	  len = PUPA_DISK_SECTOR_SIZE - offset;
	  if (len > size)
	    len = size;
	  
	  pupa_memcpy (tmp_buf + offset, buf, len);

	  pupa_disk_cache_invalidate (disk->id, sector);

	  if ((disk->dev->write) (disk, sector, 1, tmp_buf) != PUPA_ERR_NONE)
	    goto finish;

	  sector++;
	  buf += len;
	  size -= len;
	  offset = 0;
	}
      else
	{
	  unsigned long len;
	  unsigned long n;

	  len = size & ~(PUPA_DISK_SECTOR_SIZE - 1);
	  n = size >> PUPA_DISK_SECTOR_BITS;
	  
	  if ((disk->dev->write) (disk, sector, n, buf) != PUPA_ERR_NONE)
	    goto finish;

	  while (n--)
	    pupa_disk_cache_invalidate (disk->id, sector++);

	  buf += len;
	  size -= len;
	}
    }

 finish:

  return pupa_errno;
}

pupa_err_t
pupa_print_partinfo (pupa_device_t disk, char *partname)
{
  pupa_fs_t fs = 0;
  pupa_device_t part;
  char devname[20];
      
  pupa_sprintf (devname, "%s,%s", disk->disk->name, partname);
  part = pupa_device_open (devname);
  if (!part)
    pupa_printf ("\tPartition num:%s, Filesystem cannot be accessed",
		 partname);
  else
    {
      char *label;

      fs = pupa_fs_probe (part);
      /* Ignore all errors.  */
      pupa_errno = 0;

      pupa_printf ("\tPartition num:%s, Filesystem type %s",
		   partname, fs ? fs->name : "Unknown");
	  
      if (fs)
	{
	  (fs->label) (part, &label);
	  if (pupa_errno == PUPA_ERR_NONE)
	    {
	      if (label && pupa_strlen (label))
		pupa_printf (", Label: %s", label);
	      pupa_free (label);
	    }
	  pupa_errno = PUPA_ERR_NONE;
	}
      pupa_device_close (part);
    }

  pupa_printf ("\n");
  return pupa_errno;
}
