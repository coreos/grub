/* hostdisk.c - emulate biosdisk */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2006,2007,2008,2009,2010,2011,2012,2013  Free Software Foundation, Inc.
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

#include <config-util.h>

#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

# include <sys/ioctl.h>         /* ioctl */
# include <sys/mount.h>
# ifndef BLKFLSBUF
#  define BLKFLSBUF     _IO (0x12,97)   /* flush buffer cache */
# endif /* ! BLKFLSBUF */
# include <sys/ioctl.h>		/* ioctl */
# ifndef HDIO_GETGEO
#  define HDIO_GETGEO	0x0301	/* get device geometry */
/* If HDIO_GETGEO is not defined, it is unlikely that hd_geometry is
   defined.  */
struct hd_geometry
{
  unsigned char heads;
  unsigned char sectors;
  unsigned short cylinders;
  unsigned long start;
};
# endif /* ! HDIO_GETGEO */
# ifndef BLKGETSIZE64
#  define BLKGETSIZE64  _IOR(0x12,114,size_t)    /* return device size */
# endif /* ! BLKGETSIZE64 */


grub_int64_t
grub_util_get_fd_size_os (int fd, const char *name, unsigned *log_secsize)
{
  unsigned long long nr;
  unsigned sector_size, log_sector_size;

  if (ioctl (fd, BLKGETSIZE64, &nr))
    return -1;

  if (ioctl (fd, BLKSSZGET, &sector_size))
    return -1;

  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;
  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  if (nr & ((1 << log_sector_size) - 1))
    grub_util_error ("%s", _("unaligned device size"));

  return nr;
}

grub_disk_addr_t
grub_util_find_partition_start_os (const char *dev)
{
  int fd;
  struct hd_geometry hdg;

  fd = open (dev, O_RDONLY);
  if (fd == -1)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  dev, strerror (errno));
      return 0;
    }

  if (ioctl (fd, HDIO_GETGEO, &hdg))
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
		  "cannot get disk geometry of `%s'", dev);
      close (fd);
      return 0;
    }

  close (fd);

  return hdg.start;
}

/* Cache of partition start sectors for each disk.  */
struct linux_partition_cache
{
  struct linux_partition_cache *next;
  struct linux_partition_cache **prev;
  char *dev;
  unsigned long start;
  int partno;
};

struct linux_partition_cache *linux_partition_cache_list;

/* Check if we have devfs support.  */
static int
have_devfs (void)
{
  static int dev_devfsd_exists = -1;

  if (dev_devfsd_exists < 0)
    {
      struct stat st;

      dev_devfsd_exists = stat ("/dev/.devfsd", &st) == 0;
    }

  return dev_devfsd_exists;
}

int
grub_hostdisk_linux_find_partition (char *dev, grub_disk_addr_t sector)
{
  size_t len = strlen (dev);
  const char *format;
  char *p;
  int i;
  char real_dev[PATH_MAX];
  struct linux_partition_cache *cache;
  int missing = 0;

  strcpy(real_dev, dev);

  if (have_devfs () && strcmp (real_dev + len - 5, "/disc") == 0)
    {
      p = real_dev + len - 4;
      format = "part%d";
    }
  else if (strncmp (real_dev, "/dev/disk/by-id/",
		    sizeof ("/dev/disk/by-id/") - 1) == 0)
    {
      p = real_dev + len;
      format = "-part%d";
    }
  else if (real_dev[len - 1] >= '0' && real_dev[len - 1] <= '9')
    {
      p = real_dev + len;
      format = "p%d";
    }
  else
    {
      p = real_dev + len;
      format = "%d";
    }

  for (cache = linux_partition_cache_list; cache; cache = cache->next)
    {
      if (strcmp (cache->dev, dev) == 0 && cache->start == sector)
	{
	  sprintf (p, format, cache->partno);
	  strcpy (dev, real_dev);
	  return 1;
	}
    }

  for (i = 1; i < 10000; i++)
    {
      int fd;
      grub_disk_addr_t start;

      sprintf (p, format, i);

      fd = open (real_dev, O_RDONLY);
      if (fd == -1)
	{
	  if (missing++ < 10)
	    continue;
	  else
	    return 0;
	}
      missing = 0;
      close (fd);

      if (!grub_util_device_is_mapped (real_dev)
	  || !grub_util_get_dm_node_linear_info (real_dev, 0, 0, &start))
	start = grub_util_find_partition_start_os (real_dev);
      /* We don't care about errors here.  */
      grub_errno = GRUB_ERR_NONE;

      if (start == sector)
	{
	  struct linux_partition_cache *new_cache_item;

	  new_cache_item = xmalloc (sizeof *new_cache_item);
	  new_cache_item->dev = xstrdup (dev);
	  new_cache_item->start = start;
	  new_cache_item->partno = i;
	  grub_list_push (GRUB_AS_LIST_P (&linux_partition_cache_list),
			  GRUB_AS_LIST (new_cache_item));

	  strcpy (dev, real_dev);
	  return 1;
	}
    }

  return 0;
}

void
grub_hostdisk_flush_initial_buffer (const char *os_dev)
{
  int fd;
  struct stat st;

  fd = open (os_dev, O_RDONLY);
  if (fd >= 0 && fstat (fd, &st) >= 0 && S_ISBLK (st.st_mode))
    ioctl (fd, BLKFLSBUF, 0);
  if (fd >= 0)
    close (fd);
}

void
grub_hostdisk_configure_device_driver (int fd __attribute__ ((unused)))
{
}
