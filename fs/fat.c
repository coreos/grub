/* fat.c - FAT filesystem */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

#include <pupa/fs.h>
#include <pupa/disk.h>
#include <pupa/file.h>
#include <pupa/types.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/dl.h>

#define PUPA_FAT_DIR_ENTRY_SIZE	32

#define PUPA_FAT_ATTR_READ_ONLY	0x01
#define PUPA_FAT_ATTR_HIDDEN	0x02
#define PUPA_FAT_ATTR_SYSTEM	0x04
#define PUPA_FAT_ATTR_VOLUME_ID	0x08
#define PUPA_FAT_ATTR_DIRECTORY	0x10
#define PUPA_FAT_ATTR_ARCHIVE	0x20

#define PUPA_FAT_ATTR_LONG_NAME	(PUPA_FAT_ATTR_READ_ONLY \
				 | PUPA_FAT_ATTR_HIDDEN \
				 | PUPA_FAT_ATTR_SYSTEM \
				 | PUPA_FAT_ATTR_VOLUME_ID)
#define PUPA_FAT_ATTR_VALID	(PUPA_FAT_ATTR_READ_ONLY \
				 | PUPA_FAT_ATTR_HIDDEN \
				 | PUPA_FAT_ATTR_SYSTEM \
				 | PUPA_FAT_ATTR_DIRECTORY \
				 | PUPA_FAT_ATTR_ARCHIVE)

struct pupa_fat_bpb
{
  pupa_uint8_t jmp_boot[3];
  pupa_uint8_t oem_name[8];
  pupa_uint16_t bytes_per_sector;
  pupa_uint8_t sectors_per_cluster;
  pupa_uint16_t num_reserved_sectors;
  pupa_uint8_t num_fats;
  pupa_uint16_t num_root_entries;
  pupa_uint16_t num_total_sectors_16;
  pupa_uint8_t media;
  pupa_uint16_t sectors_per_fat_16;
  pupa_uint16_t sectors_per_track;
  pupa_uint16_t num_heads;
  pupa_uint32_t num_hidden_sectors;
  pupa_uint32_t num_total_sectors_32;
  
  /* The following fields are only used by FAT32.  */
  pupa_uint32_t sectors_per_fat_32;
  pupa_uint16_t extended_flags;
  pupa_uint16_t fs_version;
  pupa_uint32_t root_cluster;
  pupa_uint16_t fs_info;
  pupa_uint16_t backup_boot_sector;
} __attribute__ ((packed));

struct pupa_fat_dir_entry
{
  pupa_uint8_t name[11];
  pupa_uint8_t attr;
  pupa_uint8_t nt_reserved;
  pupa_uint8_t c_time_tenth;
  pupa_uint16_t c_time;
  pupa_uint16_t c_date;
  pupa_uint16_t a_date;
  pupa_uint16_t first_cluster_high;
  pupa_uint16_t w_time;
  pupa_uint16_t w_date;
  pupa_uint16_t first_cluster_low;
  pupa_uint32_t file_size;
} __attribute__ ((packed));

struct pupa_fat_long_name_entry
{
  pupa_uint8_t id;
  pupa_uint16_t name1[5];
  pupa_uint8_t attr;
  pupa_uint8_t reserved;
  pupa_uint8_t checksum;
  pupa_uint16_t name2[6];
  pupa_uint16_t first_cluster;
  pupa_uint16_t name3[2];
} __attribute__ ((packed));

struct pupa_fat_data
{
  int logical_sector_bits;
  pupa_uint32_t num_sectors;
  
  pupa_uint16_t fat_sector;
  pupa_uint32_t sectors_per_fat;
  int fat_size;
  
  pupa_uint32_t root_cluster;
  pupa_uint32_t root_sector;
  pupa_uint32_t num_root_sectors;
  
  int cluster_bits;
  pupa_uint32_t cluster_eof_mark;
  pupa_uint32_t cluster_sector;
  pupa_uint32_t num_clusters;

  pupa_uint8_t attr;
  pupa_ssize_t file_size;
  pupa_uint32_t file_cluster;
  pupa_uint32_t cur_cluster_num;
  pupa_uint32_t cur_cluster;
};

#ifndef PUPA_UTIL
static pupa_dl_t my_mod;
#endif

static int
fat_log2 (unsigned x)
{
  int i;
  
  if (x == 0)
    return -1;

  for (i = 0; (x & 1) == 0; i++)
    x >>= 1;

  if (x != 1)
    return -1;

  return i;
}

static struct pupa_fat_data *
pupa_fat_mount (pupa_disk_t disk)
{
  struct pupa_fat_bpb bpb;
  struct pupa_fat_data *data = 0;
  pupa_uint32_t first_fat, magic;

  if (! disk)
    goto fail;

  data = (struct pupa_fat_data *) pupa_malloc (sizeof (*data));
  if (! data)
    goto fail;

  /* Read the BPB.  */
  if (pupa_disk_read (disk, 0, 0, sizeof (bpb), (char *) &bpb))
    goto fail;

  /* Get the sizes of logical sectors and clusters.  */
  data->logical_sector_bits =
    fat_log2 (pupa_le_to_cpu16 (bpb.bytes_per_sector));
  if (data->logical_sector_bits < PUPA_DISK_SECTOR_BITS)
    goto fail;
  data->logical_sector_bits -= PUPA_DISK_SECTOR_BITS;
  
  data->cluster_bits = fat_log2 (bpb.sectors_per_cluster);
  if (data->cluster_bits < 0)
    goto fail;
  data->cluster_bits += data->logical_sector_bits;

  /* Get information about FATs.  */
  data->fat_sector = (pupa_le_to_cpu16 (bpb.num_reserved_sectors)
		      << data->logical_sector_bits);
  if (data->fat_sector == 0)
    goto fail;

  data->sectors_per_fat = ((bpb.sectors_per_fat_16
			    ? pupa_le_to_cpu16 (bpb.sectors_per_fat_16)
			    : pupa_le_to_cpu32 (bpb.sectors_per_fat_32))
			   << data->logical_sector_bits);
  if (data->sectors_per_fat == 0)
    goto fail;

  /* Get the number of sectors in this volume.  */
  data->num_sectors = ((bpb.num_total_sectors_16
			? pupa_le_to_cpu16 (bpb.num_total_sectors_16)
			: pupa_le_to_cpu32 (bpb.num_total_sectors_32))
		       << data->logical_sector_bits);
  if (data->num_sectors == 0)
    goto fail;

  /* Get information about the root directory.  */
  if (bpb.num_fats == 0)
    goto fail;

  data->root_sector = data->fat_sector + bpb.num_fats * data->sectors_per_fat;
  data->num_root_sectors
    = ((((pupa_uint32_t) pupa_le_to_cpu16 (bpb.num_root_entries)
	 * PUPA_FAT_DIR_ENTRY_SIZE
	 + pupa_le_to_cpu16 (bpb.bytes_per_sector) - 1)
	>> (data->logical_sector_bits + PUPA_DISK_SECTOR_BITS))
       << (data->logical_sector_bits));

  data->cluster_sector = data->root_sector + data->num_root_sectors;
  data->num_clusters = (((data->num_sectors - data->cluster_sector)
			 >> (data->cluster_bits + data->logical_sector_bits))
			+ 2);

  if (data->num_clusters <= 2)
    goto fail;

  if (! bpb.sectors_per_fat_16)
    {
      /* FAT32.  */
      pupa_uint16_t flags = pupa_le_to_cpu16 (bpb.extended_flags);
      
      data->root_cluster = pupa_le_to_cpu32 (bpb.root_cluster);
      data->fat_size = 32;
      data->cluster_eof_mark = 0x0ffffff8;
      
      if (flags & 0x80)
	{
	  /* Get an active FAT.  */
	  unsigned active_fat = flags & 0xf;
	  
	  if (active_fat > bpb.num_fats)
	    goto fail;

	  data->fat_sector += active_fat * data->sectors_per_fat;
	}

      if (bpb.num_root_entries != 0 || bpb.fs_version != 0)
	goto fail;
    }
  else
    {
      /* FAT12 or FAT16.  */
      data->root_cluster = ~0UL;

      if (data->num_clusters <= 4085 + 2)
	{
	  /* FAT12.  */
	  data->fat_size = 12;
	  data->cluster_eof_mark = 0x0ff8;
	}
      else
	{
	  /* FAT16.  */
	  data->fat_size = 16;
	  data->cluster_eof_mark = 0xfff8;
	}
    }

  /* More sanity checks.  */
  if (data->num_sectors <= data->fat_sector)
    goto fail;

  if (pupa_disk_read (disk,
		      data->fat_sector,
		      0,
		      sizeof (first_fat),
		      (char *) &first_fat))
    goto fail;

  first_fat = pupa_le_to_cpu32 (first_fat);

  if (data->fat_size == 32)
    {
      first_fat &= 0x0fffffff;
      magic = 0x0fffff00;
    }
  else if (data->fat_size == 16)
    {
      first_fat &= 0x0000ffff;
      magic = 0xff00;
    }
  else
    {
      first_fat &= 0x00000fff;
      magic = 0x0f00;
    }
  
  if (first_fat != (magic | bpb.media))
    goto fail;

  /* Start from the root directory.  */
  data->file_cluster = data->root_cluster;
  data->cur_cluster_num = ~0UL;
  data->attr = PUPA_FAT_ATTR_DIRECTORY;
  return data;

 fail:

  pupa_free (data);
  pupa_error (PUPA_ERR_BAD_FS, "not a fat filesystem");
  return 0;
}

/* Convert UTF-16 (little endian) to UTF8.  */
static pupa_uint8_t *
pupa_fat_utf16_to_utf8 (pupa_uint8_t *dest, pupa_uint16_t *src,
			pupa_size_t size)
{
  pupa_uint32_t code_high = 0;

  while (size--)
    {
      pupa_uint32_t code = pupa_le_to_cpu16 (*src++);

      if (code_high)
	{
	  if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Surrogate pair.  */
	      code = ((code_high - 0xD800) << 12) + (code - 0xDC00) + 0x10000;
	      
	      *dest++ = (code >> 18) | 0xF0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = ((code >> 6) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else
	    {
	      /* Error...  */
	      *dest++ = '?';
	    }

	  code_high = 0;
	}
      else
	{
	  if (code <= 0x007F)
	    *dest++ = code;
	  else if (code <= 0x07FF)
	    {
	      *dest++ = (code >> 6) | 0xC0;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else if (code >= 0xD800 && code <= 0xDBFF)
	    {
	      code_high = code;
	      continue;
	    }
	  else if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Error... */
	      *dest++ = '?';
	    }
	  else
	    {
	      *dest++ = (code >> 16) | 0xE0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	}
    }

  return dest;
}

static pupa_ssize_t
pupa_fat_read_data (pupa_disk_t disk, struct pupa_fat_data *data,
		    void (*read_hook) (unsigned long sector,
				       unsigned offset, unsigned length),
		    pupa_ssize_t offset, pupa_ssize_t len, char *buf)
{
  pupa_ssize_t size;
  pupa_uint32_t logical_cluster;
  unsigned logical_cluster_bits;
  pupa_ssize_t ret = 0;
  unsigned long sector;
  
  /* This is a special case. FAT12 and FAT16 doesn't have the root directory
     in clusters.  */
  if (data->file_cluster == ~0UL)
    {
      size = (data->num_root_sectors << PUPA_DISK_SECTOR_BITS) - offset;
      if (size > len)
	size = len;

      if (pupa_disk_read (disk, data->root_sector, offset, size, buf))
	return -1;

      return size;
    }

  /* Calculate the logical cluster number and offset.  */
  logical_cluster_bits = (data->cluster_bits
			  + data->logical_sector_bits
			  + PUPA_DISK_SECTOR_BITS);
  logical_cluster = offset >> logical_cluster_bits;
  offset &= (1 << logical_cluster_bits) - 1;

  if (logical_cluster < data->cur_cluster_num)
    {
      data->cur_cluster_num = 0;
      data->cur_cluster = data->file_cluster;
    }

  while (len)
    {
      while (logical_cluster > data->cur_cluster_num)
	{
	  /* Find next cluster.  */
	  pupa_uint32_t next_cluster;
	  unsigned long fat_offset;

	  switch (data->fat_size)
	    {
	    case 32:
	      fat_offset = data->cur_cluster << 2;
	      break;
	    case 16:
	      fat_offset = data->cur_cluster << 1;
	      break;
	    default:
	      /* case 12: */
	      fat_offset = data->cur_cluster + (data->cur_cluster >> 1);
	      break;
	    }

	  /* Read the FAT.  */
	  if (pupa_disk_read (disk, data->fat_sector, fat_offset,
			      (data->fat_size + 7) >> 3,
			      (char *) &next_cluster))
	    return -1;

	  next_cluster = pupa_le_to_cpu32 (next_cluster);
	  switch (data->fat_size)
	    {
	    case 16:
	      next_cluster &= 0xFFFF;
	      break;
	    case 12:
	      if (data->cur_cluster & 1)
		next_cluster >>= 4;
	      
	      next_cluster &= 0x0FFF;
	      break;
	    }

#if 0
	  pupa_printf ("%s:%d: fat_size=%d, next_cluster=%u\n",
		       __FILE__, __LINE__, data->fat_size, next_cluster);
#endif
	  
	  /* Check the end.  */
	  if (next_cluster >= data->cluster_eof_mark)
	    return ret;

	  if (next_cluster < 2 || next_cluster >= data->num_clusters)
	    {
	      pupa_error (PUPA_ERR_BAD_FS, "invalid cluster %u",
			  next_cluster);
	      return -1;
	    }

	  data->cur_cluster = next_cluster;
	  data->cur_cluster_num++;
	}

      /* Read the data here.  */
      sector = (data->cluster_sector
		+ ((data->cur_cluster - 2)
		   << (data->cluster_bits + data->logical_sector_bits)));
      size = (1 << logical_cluster_bits) - offset;
      if (size > len)
	size = len;

      disk->read_hook = read_hook;
      pupa_disk_read (disk, sector, offset, size, buf);
      disk->read_hook = 0;
      if (pupa_errno)
	return -1;

      len -= size;
      buf += size;
      ret += size;
      logical_cluster++;
      offset = 0;
    }

  return ret;
}

/* Find the underlying directory or file in PATH and return the
   next path. If there is no next path or an error occurs, return NULL.
   If HOOK is specified, call it with each file name.  */
static char *
pupa_fat_find_dir (pupa_disk_t disk, struct pupa_fat_data *data,
		   const char *path,
		   int (*hook) (const char *filename, int dir))
{
  struct pupa_fat_dir_entry dir;
  char *dirname, *dirp;
  char *filename, *filep = 0;
  pupa_uint16_t *unibuf;
  int slot = -1, slots = -1;
  int checksum = -1;
  pupa_ssize_t offset = -sizeof(dir);
  int call_hook;
  
  if (! (data->attr & PUPA_FAT_ATTR_DIRECTORY))
    {
      pupa_error (PUPA_ERR_BAD_FILE_TYPE, "not a directory");
      return 0;
    }
  
  /* Extract a directory name.  */
  while (*path == '/')
    path++;

  dirp = pupa_strchr (path, '/');
  if (dirp)
    {
      unsigned len = dirp - path;
      
      dirname = pupa_malloc (len + 1);
      if (! dirname)
	return 0;

      pupa_memcpy (dirname, path, len);
      dirname[len] = '\0';
    }
  else
    /* This is actually a file.  */
    dirname = pupa_strdup (path);

  call_hook = (! dirp && hook);
  
  /* Allocate space enough to hold a long name.  */
  filename = pupa_malloc (0x40 * 13 * 4 + 1);
  unibuf = (pupa_uint16_t *) pupa_malloc (0x40 * 13 * 2);
  if (! filename || ! unibuf)
    {
      pupa_free (filename);
      pupa_free (unibuf);
      pupa_free (dirname);
      return 0;
    }
      
  while (1)
    {
      unsigned i;

      /* Adjust the offset.  */
      offset += sizeof (dir);

      /* Read a directory entry.  */
      if ((pupa_fat_read_data (disk, data, 0,
			       offset, sizeof (dir), (char *) &dir)
	   != sizeof (dir))
	  || dir.name[0] == 0)
	{
	  if (pupa_errno == PUPA_ERR_NONE && ! call_hook)
	    pupa_error (PUPA_ERR_FILE_NOT_FOUND, "file not found");

	  break;
	}

      /* Handle long name entries.  */
      if (dir.attr == PUPA_FAT_ATTR_LONG_NAME)
	{
	  struct pupa_fat_long_name_entry *long_name
	    = (struct pupa_fat_long_name_entry *) &dir;
	  pupa_uint8_t id = long_name->id;
	  
	  if (id & 0x40)
	    {
	      id &= 0x3f;
	      slots = slot = id;
	      checksum = long_name->checksum;
	    }

	  if (id != slot || slot == 0 || checksum != long_name->checksum)
	    {
	      checksum = -1;
	      continue;
	    }

	  slot--;
	  pupa_memcpy (unibuf + slot * 13, long_name->name1, 5 * 2);
	  pupa_memcpy (unibuf + slot * 13 + 5, long_name->name2, 6 * 2);
	  pupa_memcpy (unibuf + slot * 13 + 11, long_name->name3, 2 * 2);
	  continue;
	}

      /* Check if this entry is valid.  */
      if (dir.name[0] == 0xe5 || (dir.attr & ~PUPA_FAT_ATTR_VALID))
	continue;

      /* This is a workaround for Japanese.  */
      if (dir.name[0] == 0x05)
	dir.name[0] = 0xe5;
      
      if (checksum != -1 && slot == 0)
	{
	  pupa_uint8_t sum;
	  
	  for (sum = 0, i = 0; i < sizeof (dir.name); i++)
	    sum = ((sum >> 1) | (sum << 7)) + dir.name[i];

	  if (sum == checksum)
	    {
	      *pupa_fat_utf16_to_utf8 (filename, unibuf, slots * 13) = '\0';
	      
	      if (*dirname == '\0' && call_hook)
		{
		  if (hook (filename, dir.attr & PUPA_FAT_ATTR_DIRECTORY))
		    break;
		  
		  checksum = -1;
		  continue;
		}

	      if (pupa_strcmp (dirname, filename) == 0)
		{
		  if (call_hook)
		    hook (filename, dir.attr & PUPA_FAT_ATTR_DIRECTORY);
		  
		  break;
		}
	    }

	  checksum = -1;
	}

      /* Convert the 8.3 file name.  */
      filep = filename;
      
      for (i = 0; i < 8 && dir.name[i] && ! pupa_isspace (dir.name[i]); i++)
	*filep++ = pupa_tolower (dir.name[i]);
      
      *filep = '.';
      
      for (i = 8; i < 11 && dir.name[i] && ! pupa_isspace (dir.name[i]); i++)
	*++filep = pupa_tolower (dir.name[i]);

      if (*filep != '.')
	filep++;
      
      *filep = '\0';

      if (*dirname == '\0' && call_hook)
	{
	  if (hook (filename, dir.attr & PUPA_FAT_ATTR_DIRECTORY))
	    break;
	}
      else if (pupa_strcmp (dirname, filename) == 0)
	{
	  if (call_hook)
	    hook (filename, dir.attr & PUPA_FAT_ATTR_DIRECTORY);

	  break;
	}
    }

  pupa_free (filename);
  pupa_free (dirname);

  data->attr = dir.attr;
  data->file_size = pupa_le_to_cpu32 (dir.file_size);
  data->file_cluster = ((pupa_le_to_cpu16 (dir.first_cluster_high) << 16)
			| pupa_le_to_cpu16 (dir.first_cluster_low));
  data->cur_cluster_num = ~0UL;
  
  return dirp;
}

static pupa_err_t
pupa_fat_dir (pupa_device_t device, const char *path,
	      int (*hook) (const char *filename, int dir))
{
  struct pupa_fat_data *data = 0;
  pupa_disk_t disk = device->disk;
  char *p = (char *) path;

#ifndef PUPA_UTIL
  pupa_dl_ref (my_mod);
#endif
  
  data = pupa_fat_mount (disk);
  if (! data)
    goto fail;

  do
    {
      p = pupa_fat_find_dir (disk, data, p, hook);
    }
  while (p && pupa_errno == PUPA_ERR_NONE);

 fail:
  
  pupa_free (data);
  
#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif
  
  return pupa_errno;
}

static pupa_err_t
pupa_fat_open (pupa_file_t file, const char *name)
{
  struct pupa_fat_data *data = 0;
  char *p = (char *) name;

#ifndef PUPA_UTIL
  pupa_dl_ref (my_mod);
#endif
  
  data = pupa_fat_mount (file->device->disk);
  if (! data)
    goto fail;

  do
    {
      p = pupa_fat_find_dir (file->device->disk, data, p, 0);
      if (pupa_errno != PUPA_ERR_NONE)
	goto fail;
    }
  while (p);

  if (data->attr & PUPA_FAT_ATTR_DIRECTORY)
    {
      pupa_error (PUPA_ERR_BAD_FILE_TYPE, "not a file");
      goto fail;
    }

  file->data = data;
  file->size = data->file_size;
  
  return PUPA_ERR_NONE;

 fail:
  
  pupa_free (data);
  
#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif
  
  return pupa_errno;
}

static pupa_ssize_t
pupa_fat_read (pupa_file_t file, char *buf, pupa_ssize_t len)
{
  return pupa_fat_read_data (file->device->disk, file->data, file->read_hook,
			     file->offset, len, buf);
}

static pupa_err_t
pupa_fat_close (pupa_file_t file)
{
  pupa_free (file->data);
  
#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif
  
  return pupa_errno;
}

static pupa_err_t
pupa_fat_label (pupa_device_t device, char **label)
{
  struct pupa_fat_data *data;
  pupa_disk_t disk = device->disk;
  pupa_ssize_t offset = -sizeof(struct pupa_fat_dir_entry);


#ifndef PUPA_UTIL
  pupa_dl_ref (my_mod);
#endif
  
  data = pupa_fat_mount (disk);
  if (! data)
    goto fail;

  if (! (data->attr & PUPA_FAT_ATTR_DIRECTORY))
    {
      pupa_error (PUPA_ERR_BAD_FILE_TYPE, "not a directory");
      return 0;
    }

  while (1)
    {
      struct pupa_fat_dir_entry dir;

      /* Adjust the offset.  */
      offset += sizeof (dir);
      
      /* Read a directory entry.  */
      if ((pupa_fat_read_data (disk, data, 0,
			       offset, sizeof (dir), (char *) &dir)
	   != sizeof (dir))
	  || dir.name[0] == 0)
	{
	  if (pupa_errno != PUPA_ERR_NONE)
	    goto fail;
	  else
	    {
	      *label = 0;
	      return PUPA_ERR_NONE;
	    }
	}

      if (dir.attr == PUPA_FAT_ATTR_VOLUME_ID)
	{
	  *label = pupa_strndup (dir.name, 11);
	  return PUPA_ERR_NONE;
	}
    }

  *label = 0;
  
 fail:

#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif

  pupa_free (data);

  return pupa_errno;
}

static struct pupa_fs pupa_fat_fs =
  {
    .name = "fat",
    .dir = pupa_fat_dir,
    .open = pupa_fat_open,
    .read = pupa_fat_read,
    .close = pupa_fat_close,
    .label = pupa_fat_label,
    .next = 0
  };

#ifdef PUPA_UTIL
void
pupa_fat_init (void)
{
  pupa_fs_register (&pupa_fat_fs);
}

void
pupa_fat_fini (void)
{
  pupa_fs_unregister (&pupa_fat_fs);
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  pupa_fs_register (&pupa_fat_fs);
  my_mod = mod;
}

PUPA_MOD_FINI
{
  pupa_fs_unregister (&pupa_fat_fs);
}
#endif /* ! PUPA_UTIL */
