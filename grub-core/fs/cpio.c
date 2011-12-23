/* cpio.c - cpio and tar filesystem.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2009 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define ATTR_TYPE  0160000
#define ATTR_FILE  0100000
#define ATTR_DIR   0040000
#define ATTR_LNK   0120000

#ifndef MODE_USTAR
/* cpio support */
#define	MAGIC_BCPIO	070707
struct head
{
  grub_uint16_t magic;
  grub_uint16_t dev;
  grub_uint16_t ino;
  grub_uint16_t mode;
  grub_uint16_t uid;
  grub_uint16_t gid;
  grub_uint16_t nlink;
  grub_uint16_t rdev;
  grub_uint16_t mtime_1;
  grub_uint16_t mtime_2;
  grub_uint16_t namesize;
  grub_uint16_t filesize_1;
  grub_uint16_t filesize_2;
} __attribute__ ((packed));
#else
/* tar support */
#define MAGIC_USTAR	"ustar"
struct head
{
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
} __attribute__ ((packed));
#endif

struct grub_cpio_data
{
  grub_disk_t disk;
  grub_off_t hofs;
  grub_off_t dofs;
  grub_off_t size;
#ifdef MODE_USTAR
  char *linkname;
  grub_size_t linkname_alloc;
#endif
};

static grub_dl_t my_mod;

static inline void
canonicalize (char *name)
{
  char *iptr, *optr;
  for (iptr = name, optr = name; *iptr; )
    {
      while (*iptr == '/')
	iptr++;
      if (iptr[0] == '.' && (iptr[1] == '/' || iptr[1] == 0))
	{
	  iptr += 2;
	  continue;
	}
      while (*iptr && *iptr != '/')
	*optr++ = *iptr++;
      if (*iptr)
	*optr++ = *iptr++;
    }
  *optr = 0;
}

static inline unsigned long long
read_number (const char *str, grub_size_t size)
{
  long long ret = 0;
  while (size-- && *str >= '0' && *str <= '7')
    ret = (ret << 3) | (*str++ & ~'0');
  return ret;
}

static grub_err_t
grub_cpio_find_file (struct grub_cpio_data *data, char **name,
		     grub_int32_t *mtime, grub_disk_addr_t *ofs,
		     grub_uint32_t *mode)
{
#ifndef MODE_USTAR
  struct head hd;

  if (grub_disk_read (data->disk, 0, data->hofs, sizeof (hd), &hd))
    return grub_errno;

  if (hd.magic != MAGIC_BCPIO)
    return grub_error (GRUB_ERR_BAD_FS, "invalid cpio archive");

  data->size = (((grub_uint32_t) hd.filesize_1) << 16) + hd.filesize_2;
  if (mtime)
    *mtime = (((grub_uint32_t) hd.mtime_1) << 16) + hd.mtime_2;
  if (mode)
    *mode = hd.mode;

  if (hd.namesize & 1)
    hd.namesize++;

  *name = grub_malloc (hd.namesize + 1);
  if (*name == NULL)
    return grub_errno;

  if (grub_disk_read (data->disk, 0, data->hofs + sizeof (hd),
		      hd.namesize, *name))
    {
      grub_free (*name);
      return grub_errno;
    }
  (*name)[hd.namesize] = 0;

  if (data->size == 0 && hd.mode == 0 && hd.namesize == 11 + 1
      && grub_memcmp(*name, "TRAILER!!!", 11) == 0)
    {
      *ofs = 0;
      grub_free (*name);
      return GRUB_ERR_NONE;
    }

  canonicalize (*name);

  data->dofs = data->hofs + sizeof (hd) + hd.namesize;
  *ofs = data->dofs + data->size;
  if (data->size & 1)
    (*ofs)++;
#else
  struct head hd;
  int reread = 0, have_longname = 0, have_longlink = 0;

  for (reread = 0; reread < 3; reread++)
    {
      if (grub_disk_read (data->disk, 0, data->hofs, sizeof (hd), &hd))
	return grub_errno;

      if (!hd.name[0])
	{
	  *ofs = 0;
	  return GRUB_ERR_NONE;
	}

      if (grub_memcmp (hd.magic, MAGIC_USTAR, sizeof (MAGIC_USTAR) - 1))
	return grub_error (GRUB_ERR_BAD_FS, "invalid tar archive");

      if (hd.typeflag == 'L')
	{
	  grub_err_t err;
	  grub_size_t namesize = read_number (hd.size, sizeof (hd.size));
	  *name = grub_malloc (namesize + 1);
	  if (*name == NULL)
	    return grub_errno;
	  err = grub_disk_read (data->disk, 0,
				data->hofs + GRUB_DISK_SECTOR_SIZE, namesize,
				*name);
	  (*name)[namesize] = 0;
	  if (err)
	    return err;
	  data->hofs += GRUB_DISK_SECTOR_SIZE
	    + ((namesize + GRUB_DISK_SECTOR_SIZE - 1) &
	       ~(GRUB_DISK_SECTOR_SIZE - 1));
	  have_longname = 1;
	  continue;
	}

      if (hd.typeflag == 'K')
	{
	  grub_err_t err;
	  grub_size_t linksize = read_number (hd.size, sizeof (hd.size));
	  if (data->linkname_alloc < linksize + 1)
	    {
	      char *n;
	      n = grub_malloc (2 * (linksize + 1));
	      if (!n)
		return grub_errno;
	      grub_free (data->linkname);
	      data->linkname = n;
	      data->linkname_alloc = 2 * (linksize + 1);
	    }

	  err = grub_disk_read (data->disk, 0,
				data->hofs + GRUB_DISK_SECTOR_SIZE, linksize,
				data->linkname);
	  if (err)
	    return err;
	  data->linkname[linksize] = 0;
	  data->hofs += GRUB_DISK_SECTOR_SIZE
	    + ((linksize + GRUB_DISK_SECTOR_SIZE - 1) &
	       ~(GRUB_DISK_SECTOR_SIZE - 1));
	  have_longlink = 1;
	  continue;
	}

      if (!have_longname)
	{
	  *name = grub_strndup (hd.name, sizeof (hd.name));
	  if (*name == NULL)
	    return grub_errno;
	}

      data->size = read_number (hd.size, sizeof (hd.size));
      data->dofs = data->hofs + GRUB_DISK_SECTOR_SIZE;
      *ofs = data->dofs + ((data->size + GRUB_DISK_SECTOR_SIZE - 1) &
			   ~(GRUB_DISK_SECTOR_SIZE - 1));
      if (mtime)
	*mtime = read_number (hd.mtime, sizeof (hd.mtime));
      if (mode)
	{
	  *mode = read_number (hd.mode, sizeof (hd.mode));
	  switch (hd.typeflag)
	    {
	    case '2':
	      *mode |= ATTR_LNK;
	      break;
	    case '0':
	      *mode |= ATTR_FILE;
	      break;
	    case '5':
	      *mode |= ATTR_DIR;
	      break;
	    }
	}
      if (!have_longlink)
	{
	  if (data->linkname_alloc < 101)
	    {
	      char *n;
	      n = grub_malloc (101);
	      if (!n)
		return grub_errno;
	      grub_free (data->linkname);
	      data->linkname = n;
	      data->linkname_alloc = 101;
	    }
	  grub_memcpy (data->linkname, hd.linkname, sizeof (hd.linkname));
	  data->linkname[100] = 0;
	}

      canonicalize (*name);
      return GRUB_ERR_NONE;
    }
#endif
  return GRUB_ERR_NONE;
}

static struct grub_cpio_data *
grub_cpio_mount (grub_disk_t disk)
{
  struct head hd;
  struct grub_cpio_data *data;

  if (grub_disk_read (disk, 0, 0, sizeof (hd), &hd))
    goto fail;

#ifndef MODE_USTAR
  if (hd.magic != MAGIC_BCPIO)
#else
  if (grub_memcmp (hd.magic, MAGIC_USTAR,
		   sizeof (MAGIC_USTAR) - 1))
#endif
    goto fail;

  data = (struct grub_cpio_data *) grub_zalloc (sizeof (*data));
  if (!data)
    goto fail;

  data->disk = disk;

  return data;

fail:
  grub_error (GRUB_ERR_BAD_FS, "not a "
#ifdef MODE_USTAR
	      "tar"
#else
	      "cpio"
#endif
	      " filesystem");
  return 0;
}

static grub_err_t
handle_symlink (struct grub_cpio_data *data,
		const char *fn, char **name,
		grub_uint32_t mode, int *restart)
{
  grub_size_t flen;
  char *target;
#ifndef MODE_USTAR
  grub_err_t err;
#endif
  char *ptr;
  char *lastslash;
  grub_size_t prefixlen;
  char *rest;
  grub_size_t size;

  *restart = 0;

  if ((mode & ATTR_TYPE) != ATTR_LNK)
    return GRUB_ERR_NONE;
  flen = grub_strlen (fn);
  if (grub_memcmp (*name, fn, flen) != 0 
      || ((*name)[flen] != 0 && (*name)[flen] != '/'))
    return GRUB_ERR_NONE;
  rest = *name + flen;
  lastslash = rest;
  if (*rest)
    rest++;
  while (lastslash >= *name && *lastslash != '/')
    lastslash--;
  if (lastslash >= *name)
    prefixlen = lastslash - *name;
  else
    prefixlen = 0;

  if (prefixlen)
    prefixlen++;

#ifdef MODE_USTAR
  size = grub_strlen (data->linkname);
#else
  size = data->size;
#endif
  if (size == 0)
    return GRUB_ERR_NONE;
  target = grub_malloc (size + grub_strlen (*name) + 2);
  if (!target)
    return grub_errno;

#ifdef MODE_USTAR
  grub_memcpy (target + prefixlen, data->linkname, size);
#else
  err = grub_disk_read (data->disk, 0, data->dofs, data->size, 
			target + prefixlen);
  if (err)
    return err;
#endif
  if (target[prefixlen] == '/')
    {
      grub_memmove (target, target + prefixlen, data->size - 1);
      ptr = target + data->size - 1;
      ptr = grub_stpcpy (ptr, rest);
      *ptr = 0;
      grub_dprintf ("cpio", "symlink redirected %s to %s\n",
		    *name, target);
      grub_free (*name);
      *name = target;
      *restart = 1;
      return GRUB_ERR_NONE;
    }
  if (prefixlen)
    {
      grub_memcpy (target, *name, prefixlen);
      target[prefixlen] = '/';
    }
  ptr = target + prefixlen + size;
  ptr = grub_stpcpy (ptr, rest);
  *ptr = 0;
  grub_dprintf ("cpio", "symlink redirected %s to %s\n",
		*name, target);
  grub_free (*name);
  *name = target;
  *restart = 1;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cpio_dir (grub_device_t device, const char *path_in,
	       int (*hook) (const char *filename,
			    const struct grub_dirhook_info *info))
{
  struct grub_cpio_data *data;
  grub_disk_addr_t ofs;
  char *prev, *name, *path, *ptr;
  grub_size_t len;
  int symlinknest = 0;

  path = grub_strdup (path_in + 1);
  if (!path)
    return grub_errno;
  for (ptr = path + grub_strlen (path) - 1; ptr >= path && *ptr == '/'; ptr--)
    *ptr = 0;

  grub_dl_ref (my_mod);

  prev = 0;

  data = grub_cpio_mount (device->disk);
  if (!data)
    {
      grub_free (path);
      return grub_errno;
    }

  len = grub_strlen (path);
  data->hofs = 0;
  while (1)
    {
      grub_int32_t mtime;
      grub_uint32_t mode;
      grub_err_t err;

      if (grub_cpio_find_file (data, &name, &mtime, &ofs, &mode))
	goto fail;

      if (!ofs)
	break;

      if (grub_memcmp (path, name, len) == 0
	  && (name[len] == 0 || name[len] == '/' || len == 0))
	{
	  char *p, *n;

	  n = name + len;
	  while (*n == '/')
	    n++;

	  p = grub_strchr (n, '/');
	  if (p)
	    *p = 0;

	  if (((!prev) || (grub_strcmp (prev, n) != 0)) && *n != 0)
	    {
	      struct grub_dirhook_info info;
	      grub_memset (&info, 0, sizeof (info));
	      info.dir = (p != NULL);
	      info.mtime = mtime;
	      info.mtimeset = 1;

	      hook (n, &info);
	      grub_free (prev);
	      prev = name;
	    }
	  else
	    {
	      int restart;
	      err = handle_symlink (data, name, &path, mode, &restart);
	      grub_free (name);
	      if (err)
		goto fail;
	      if (restart)
		{
		  len = grub_strlen (path);
		  if (++symlinknest == 8)
		    {
		      grub_error (GRUB_ERR_SYMLINK_LOOP,
				  "too deep nesting of symlinks");
		      goto fail;
		    }
		  ofs = 0;
		}
	    }
	}
      else
	grub_free (name);
      data->hofs = ofs;
    }

fail:

  grub_free (path);
  grub_free (prev);
#ifdef MODE_USTAR
  grub_free (data->linkname);
#endif
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_err_t
grub_cpio_open (grub_file_t file, const char *name_in)
{
  struct grub_cpio_data *data;
  grub_disk_addr_t ofs;
  char *fn;
  char *name = grub_strdup (name_in + 1);
  int symlinknest = 0;

  if (!name)
    return grub_errno;

  grub_dl_ref (my_mod);

  data = grub_cpio_mount (file->device->disk);
  if (!data)
    {
      grub_free (name);
      return grub_errno;
    }

  data->hofs = 0;
  while (1)
    {
      grub_uint32_t mode;
      int restart;
      
      if (grub_cpio_find_file (data, &fn, NULL, &ofs, &mode))
	goto fail;

      if (!ofs)
	{
	  grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	  break;
	}

      if (handle_symlink (data, fn, &name, mode, &restart))
	{
	  grub_free (fn);
	  goto fail;
	}

      if (restart)
	{
	  ofs = 0;
	  if (++symlinknest == 8)
	    {
	      grub_error (GRUB_ERR_SYMLINK_LOOP,
			  "too deep nesting of symlinks");
	      goto fail;
	    }
	  goto no_match;
	}

      if (grub_strcmp (name, fn) != 0)
	goto no_match;

      file->data = data;
      file->size = data->size;
      grub_free (fn);
      grub_free (name);

      return GRUB_ERR_NONE;

    no_match:

      grub_free (fn);
      data->hofs = ofs;
    }

fail:
#ifdef MODE_USTAR
  grub_free (data->linkname);
#endif
  grub_free (data);
  grub_free (name);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_ssize_t
grub_cpio_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_cpio_data *data;

  data = file->data;
  return (grub_disk_read (data->disk, 0, data->dofs + file->offset,
			  len, buf)) ? -1 : (grub_ssize_t) len;
}

static grub_err_t
grub_cpio_close (grub_file_t file)
{
  struct grub_cpio_data *data;

  data = file->data;
#ifdef MODE_USTAR
  grub_free (data->linkname);
#endif
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static struct grub_fs grub_cpio_fs = {
#ifdef MODE_USTAR
  .name = "tarfs",
#else
  .name = "cpiofs",
#endif
  .dir = grub_cpio_dir,
  .open = grub_cpio_open,
  .read = grub_cpio_read,
  .close = grub_cpio_close,
#ifdef GRUB_UTIL
  .reserved_first_sector = 0,
#endif
};

#ifdef MODE_USTAR
GRUB_MOD_INIT (tar)
#else
GRUB_MOD_INIT (cpio)
#endif
{
  grub_fs_register (&grub_cpio_fs);
  my_mod = mod;
}

#ifdef MODE_USTAR
GRUB_MOD_FINI (tar)
#else
GRUB_MOD_FINI (cpio)
#endif
{
  grub_fs_unregister (&grub_cpio_fs);
}
