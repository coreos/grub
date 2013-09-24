/* hostdisk.c - emulate biosdisk */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#ifdef __linux__
# include <sys/ioctl.h>         /* ioctl */
# include <sys/mount.h>
# ifndef BLKFLSBUF
#  define BLKFLSBUF     _IO (0x12,97)   /* flush buffer cache */
# endif /* ! BLKFLSBUF */
# include <sys/ioctl.h>		/* ioctl */
#endif /* __linux__ */

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# include <sys/sysctl.h>
#endif

static struct
{
  char *drive;
  char *device;
  int device_map;
} map[256];

static int
unescape_cmp (const char *a, const char *b_escaped)
{
  while (*a || *b_escaped)
    {
      if (*b_escaped == '\\' && b_escaped[1] != 0)
	b_escaped++;
      if (*a < *b_escaped)
	return -1;
      if (*a > *b_escaped)
	return +1;
      a++;
      b_escaped++;
    }
  if (*a)
    return +1;
  if (*b_escaped)
    return -1;
  return 0;
}

static int
find_grub_drive (const char *name)
{
  unsigned int i;

  if (name)
    {
      for (i = 0; i < ARRAY_SIZE (map); i++)
	if (map[i].drive && unescape_cmp (map[i].drive, name) == 0)
	  return i;
    }

  return -1;
}

static int
find_free_slot (void)
{
  unsigned int i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (! map[i].drive)
      return i;

  return -1;
}

static int
grub_util_biosdisk_iterate (grub_disk_dev_iterate_hook_t hook, void *hook_data,
			    grub_disk_pull_t pull)
{
  unsigned i;

  if (pull != GRUB_DISK_PULL_NONE)
    return 0;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (map[i].drive && hook (map[i].drive, hook_data))
      return 1;

  return 0;
}

static grub_err_t
grub_util_biosdisk_open (const char *name, grub_disk_t disk)
{
  int drive;
  struct stat st;
  struct grub_util_hostdisk_data *data;

  drive = find_grub_drive (name);
  grub_util_info ("drive = %d", drive);
  if (drive < 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "no mapping exists for `%s'", name);

  disk->id = drive;
  disk->data = data = xmalloc (sizeof (struct grub_util_hostdisk_data));
  data->dev = NULL;
  data->access_mode = 0;
  data->fd = GRUB_UTIL_FD_INVALID;
  data->is_disk = 0;
  data->device_map = map[drive].device_map;

  /* Get the size.  */
  {
    grub_util_fd_t fd;

    fd = grub_util_fd_open (map[drive].device, O_RDONLY);

    if (!GRUB_UTIL_FD_IS_VALID(fd))
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, N_("cannot open `%s': %s"),
			 map[drive].device, strerror (errno));

    disk->total_sectors = grub_util_get_fd_size (fd, map[drive].device,
						 &disk->log_sector_size);
    disk->total_sectors >>= disk->log_sector_size;

#if !defined(__MINGW32__) && !defined(__CYGWIN__)

# if GRUB_DISK_DEVS_ARE_CHAR
    if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode))
# else
    if (fstat (fd, &st) < 0 || ! S_ISBLK (st.st_mode))
# endif
      data->is_disk = 1;
#endif

    grub_util_fd_close (fd);

    grub_util_info ("the size of %s is %" PRIuGRUB_UINT64_T,
		    name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }
}

const char *
grub_hostdisk_os_dev_to_grub_drive (const char *os_disk, int add)
{
  unsigned int i;
  char *canon;

  canon = canonicalize_file_name (os_disk);
  if (!canon)
    canon = xstrdup (os_disk);

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (! map[i].device)
      break;
    else if (strcmp (map[i].device, canon) == 0)
      {
	free (canon);
	return map[i].drive;
      }

  if (!add)
    {
      free (canon);
      return NULL;
    }

  if (i == ARRAY_SIZE (map))
    /* TRANSLATORS: it refers to the lack of free slots.  */
    grub_util_error ("%s", _("device count exceeds limit"));

  map[i].device = canon;
  map[i].drive = xmalloc (sizeof ("hostdisk/") + strlen (os_disk));
  strcpy (map[i].drive, "hostdisk/");
  strcpy (map[i].drive + sizeof ("hostdisk/") - 1, os_disk);
  map[i].device_map = 0;

  grub_hostdisk_flush_initial_buffer (os_disk);

  return map[i].drive;
}

#ifndef __linux__
grub_util_fd_t
grub_util_fd_open_device (const grub_disk_t disk, grub_disk_addr_t sector, int flags,
			  grub_disk_addr_t *max)
{
  grub_util_fd_t fd;
  struct grub_util_hostdisk_data *data = disk->data;

  *max = ~0ULL;

#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_SYNC
  flags |= O_SYNC;
#endif
#ifdef O_FSYNC
  flags |= O_FSYNC;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
  int sysctl_flags, sysctl_oldflags;
  size_t sysctl_size = sizeof (sysctl_flags);

  if (sysctlbyname ("kern.geom.debugflags", &sysctl_oldflags, &sysctl_size, NULL, 0))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot get current flags of sysctl kern.geom.debugflags");
      return GRUB_UTIL_FD_INVALID;
    }
  sysctl_flags = sysctl_oldflags | 0x10;
  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_flags, sysctl_size))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot set flags of sysctl kern.geom.debugflags");
      return GRUB_UTIL_FD_INVALID;
    }
#endif

  if (data->dev && strcmp (data->dev, map[disk->id].device) == 0 &&
      data->access_mode == (flags & O_ACCMODE))
    {
      grub_dprintf ("hostdisk", "reusing open device `%s'\n", data->dev);
      fd = data->fd;
    }
  else
    {
      free (data->dev);
      data->dev = 0;
      if (GRUB_UTIL_FD_IS_VALID(data->fd))
	{
	    if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	      grub_util_fd_sync (data->fd);
	    grub_util_fd_close (data->fd);
	    data->fd = GRUB_UTIL_FD_INVALID;
	}

      fd = grub_util_fd_open (map[disk->id].device, flags);
      if (GRUB_UTIL_FD_IS_VALID(fd))
	{
	  data->dev = xstrdup (map[disk->id].device);
	  data->access_mode = (flags & O_ACCMODE);
	  data->fd = fd;
	}
    }

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_oldflags, sysctl_size))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot set flags back to the old value for sysctl kern.geom.debugflags");
      return GRUB_UTIL_FD_INVALID;
    }
#endif

#if defined(__APPLE__)
  /* If we can't have exclusive access, try shared access */
  if (fd < 0)
    fd = open(map[disk->id].device, flags | O_SHLOCK);
#endif

  if (!GRUB_UTIL_FD_IS_VALID(data->fd))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  map[disk->id].device, strerror (errno));
      return GRUB_UTIL_FD_INVALID;
    }

  grub_hostdisk_configure_device_driver (fd);

  if (grub_util_fd_seek (fd, map[disk->id].device,
			 sector << disk->log_sector_size))
    {
      grub_util_fd_close (fd);
      return GRUB_UTIL_FD_INVALID;
    }

  return fd;
}
#endif


static grub_err_t
grub_util_biosdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
			 grub_size_t size, char *buf)
{
  while (size)
    {
      grub_util_fd_t fd;
      grub_disk_addr_t max = ~0ULL;
      fd = grub_util_fd_open_device (disk, sector, O_RDONLY, &max);
      if (!GRUB_UTIL_FD_IS_VALID (fd))
	return grub_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are read together with the MBR in one read.  It
	   should only remap the MBR, so we split the read in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (grub_util_fd_read (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return grub_error (GRUB_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   map[disk->id].device, strerror (errno));
      size -= max;
      buf += (max << disk->log_sector_size);
      sector += max;
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_util_biosdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
			  grub_size_t size, const char *buf)
{
  while (size)
    {
      grub_util_fd_t fd;
      grub_disk_addr_t max = ~0ULL;
      fd = grub_util_fd_open_device (disk, sector, O_WRONLY, &max);
      if (!GRUB_UTIL_FD_IS_VALID (fd))
	return grub_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are write together with the MBR in one write.  It
	   should only remap the MBR, so we split the write in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (grub_util_fd_write (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return grub_error (GRUB_ERR_WRITE_ERROR, N_("cannot write to `%s': %s"),
			   map[disk->id].device, strerror (errno));
      size -= max;
      buf += (max << disk->log_sector_size);
    }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_util_biosdisk_flush (struct grub_disk *disk)
{
  struct grub_util_hostdisk_data *data = disk->data;

  if (disk->dev->id != GRUB_DISK_DEVICE_BIOSDISK_ID)
    return GRUB_ERR_NONE;
  if (!GRUB_UTIL_FD_IS_VALID (data->fd))
    {
      grub_disk_addr_t max;
      data->fd = grub_util_fd_open_device (disk, 0, O_RDONLY, &max);
      if (!GRUB_UTIL_FD_IS_VALID (data->fd))
	return grub_errno;
    }
  grub_util_fd_sync (data->fd);
#ifdef __linux__
  if (data->is_disk)
    ioctl (data->fd, BLKFLSBUF, 0);
#endif
  return GRUB_ERR_NONE;
}

static void
grub_util_biosdisk_close (struct grub_disk *disk)
{
  struct grub_util_hostdisk_data *data = disk->data;

  free (data->dev);
  if (GRUB_UTIL_FD_IS_VALID (data->fd))
    {
      if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	grub_util_biosdisk_flush (disk);
      grub_util_fd_close (data->fd);
    }
  free (data);
}

static struct grub_disk_dev grub_util_biosdisk_dev =
  {
    .name = "hostdisk",
    .id = GRUB_DISK_DEVICE_HOSTDISK_ID,
    .iterate = grub_util_biosdisk_iterate,
    .open = grub_util_biosdisk_open,
    .close = grub_util_biosdisk_close,
    .read = grub_util_biosdisk_read,
    .write = grub_util_biosdisk_write,
    .next = 0
  };

static int
grub_util_check_file_presence (const char *p)
{
#if defined (__MINGW32__) || defined(__CYGWIN__)
  HANDLE h;
  h = grub_util_fd_open (p, O_RDONLY);
  if (!GRUB_UTIL_FD_IS_VALID(h))
    return 0;
  CloseHandle (h);
  return 1;
#else
  struct stat st;

  if (stat (p, &st) == -1)
    return 0;
  return 1;
#endif
}

static void
read_device_map (const char *dev_map)
{
  FILE *fp;
  char buf[1024];	/* XXX */
  int lineno = 0;

  if (dev_map[0] == '\0')
    {
      grub_util_info ("no device.map");
      return;
    }

  fp = fopen (dev_map, "r");
  if (! fp)
    {
      grub_util_info (_("cannot open `%s': %s"), dev_map, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof (buf), fp))
    {
      char *p = buf;
      char *e;
      char *drive_e, *drive_p;
      int drive;

      lineno++;

      /* Skip leading spaces.  */
      while (*p && grub_isspace (*p))
	p++;

      /* If the first character is `#' or NUL, skip this line.  */
      if (*p == '\0' || *p == '#')
	continue;

      if (*p != '(')
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), '(');
	  grub_util_error ("%s:%d: %s", dev_map, lineno, tmp);
	}

      p++;
      /* Find a free slot.  */
      drive = find_free_slot ();
      if (drive < 0)
	grub_util_error ("%s:%d: %s", dev_map, lineno, _("device count exceeds limit"));

      e = p;
      p = strchr (p, ')');
      if (! p)
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), ')');
	  grub_util_error ("%s:%d: %s", dev_map, lineno, tmp);
	}

      map[drive].drive = 0;
      if ((e[0] == 'f' || e[0] == 'h' || e[0] == 'c') && e[1] == 'd')
	{
	  char *ptr;
	  for (ptr = e + 2; ptr < p; ptr++)
	    if (!grub_isdigit (*ptr))
	      break;
	  if (ptr == p)
	    {
	      map[drive].drive = xmalloc (p - e + sizeof ('\0'));
	      strncpy (map[drive].drive, e, p - e + sizeof ('\0'));
	      map[drive].drive[p - e] = '\0';
	    }
	  if (*ptr == ',')
	    {
	      *p = 0;

	      /* TRANSLATORS: Only one entry is ignored. However the suggestion
		 is to correct/delete the whole file.
		 device.map is a file indicating which
		 devices are available at boot time. Fedora populated it with
		 entries like (hd0,1) /dev/sda1 which would mean that every
		 partition is a separate disk for BIOS. Such entries were
		 inactive in GRUB due to its bug which is now gone. Without
		 this additional check these entries would be harmful now.
	      */
	      grub_util_warn (_("the device.map entry `%s' is invalid. "
				"Ignoring it. Please correct or "
				"delete your device.map"), e);
	      continue;
	    }
	}
      drive_e = e;
      drive_p = p;
      map[drive].device_map = 1;

      p++;
      /* Skip leading spaces.  */
      while (*p && grub_isspace (*p))
	p++;

      if (*p == '\0')
	grub_util_error ("%s:%d: %s", dev_map, lineno, _("filename expected"));

      /* NUL-terminate the filename.  */
      e = p;
      while (*e && ! grub_isspace (*e))
	e++;
      *e = '\0';

      if (!grub_util_check_file_presence (p))
	{
	  free (map[drive].drive);
	  map[drive].drive = NULL;
	  grub_util_info ("Cannot stat `%s', skipping", p);
	  continue;
	}

      /* On Linux, the devfs uses symbolic links horribly, and that
	 confuses the interface very much, so use realpath to expand
	 symbolic links.  */
      map[drive].device = canonicalize_file_name (p);
      if (! map[drive].device)
	map[drive].device = xstrdup (p);
      
      if (!map[drive].drive)
	{
	  char c;
	  map[drive].drive = xmalloc (sizeof ("hostdisk/") + strlen (p));
	  memcpy (map[drive].drive, "hostdisk/", sizeof ("hostdisk/") - 1);
	  strcpy (map[drive].drive + sizeof ("hostdisk/") - 1, p);
	  c = *drive_p;
	  *drive_p = 0;
	  /* TRANSLATORS: device.map is a filename. Not to be translated.
	     device.map specifies disk correspondance overrides. Previously
	     one could create any kind of device name with this. Due to
	     some problems we decided to limit it to just a handful
	     possibilities.  */
	  grub_util_warn (_("the drive name `%s' in device.map is incorrect. "
			    "Using %s instead. "
			    "Please use the form [hfc]d[0-9]* "
			    "(E.g. `hd0' or `cd')"),
			  drive_e, map[drive].drive);
	  *drive_p = c;
	}

      grub_util_info ("adding `%s' -> `%s' from device.map", map[drive].drive,
		      map[drive].device);

      grub_hostdisk_flush_initial_buffer (map[drive].device);
    }

  fclose (fp);
}

void
grub_util_biosdisk_init (const char *dev_map)
{
  read_device_map (dev_map);
  grub_disk_dev_register (&grub_util_biosdisk_dev);
}

void
grub_util_biosdisk_fini (void)
{
  unsigned i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    {
      if (map[i].drive)
	free (map[i].drive);
      if (map[i].device)
	free (map[i].device);
      map[i].drive = map[i].device = NULL;
    }

  grub_disk_dev_unregister (&grub_util_biosdisk_dev);
}

const char *
grub_util_biosdisk_get_compatibility_hint (grub_disk_t disk)
{
  if (disk->dev != &grub_util_biosdisk_dev || map[disk->id].device_map)
    return disk->name;
  return 0;
}

const char *
grub_util_biosdisk_get_osdev (grub_disk_t disk)
{
  if (disk->dev != &grub_util_biosdisk_dev)
    return 0;

  return map[disk->id].device;
}
