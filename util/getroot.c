/* getroot.c - Get root device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010,2011  Free Software Foundation, Inc.
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
#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <grub/util/misc.h>
#include <grub/util/lvm.h>
#include <grub/cryptodisk.h>
#include <grub/i18n.h>

#ifdef __linux__
#include <sys/ioctl.h>         /* ioctl */
#include <sys/mount.h>
#ifndef MAJOR
# ifndef MINORBITS
#  define MINORBITS	8
# endif /* ! MINORBITS */
# define MAJOR(dev)	((unsigned) ((dev) >> MINORBITS))
#endif /* ! MAJOR */
#ifndef FLOPPY_MAJOR
# define FLOPPY_MAJOR	2
#endif /* ! FLOPPY_MAJOR */
#endif

#ifdef __GNU__
#include <hurd.h>
#include <hurd/lookup.h>
#include <hurd/fs.h>
#include <sys/mman.h>
#endif

#include <sys/types.h>

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
# include <grub/util/libzfs.h>
# include <grub/util/libnvpair.h>
#endif

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>

#ifdef __CYGWIN__
# include <sys/ioctl.h>
# include <cygwin/fs.h> /* BLKGETSIZE64 */
# include <cygwin/hdreg.h> /* HDIO_GETGEO */
# include <sys/cygwin.h>

# define MAJOR(dev)	((unsigned) ((dev) >> 16))
# define FLOPPY_MAJOR	2
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# define MAJOR(dev) major(dev)
# define FLOPPY_MAJOR	2
#endif

#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
#include <sys/mount.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
# include <sys/disk.h>    /* struct dkwedge_info */
#include <sys/param.h>
#include <sys/mount.h>
#endif /* defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */

#if defined(__NetBSD__) || defined(__OpenBSD__)
# define MAJOR(dev) major(dev)
# ifdef HAVE_GETRAWPARTITION
#  include <util.h>    /* getrawpartition */
# endif /* HAVE_GETRAWPARTITION */
#if defined(__NetBSD__)
# include <sys/fdio.h>
#endif
# ifndef FLOPPY_MAJOR
#  define FLOPPY_MAJOR	2
# endif /* ! FLOPPY_MAJOR */
# ifndef RAW_FLOPPY_MAJOR
#  define RAW_FLOPPY_MAJOR	9
# endif /* ! RAW_FLOPPY_MAJOR */
#endif /* defined(__NetBSD__) */

#if !defined (__MINGW32__) && !defined (__CYGWIN__)

static void
pull_lvm_by_command (const char *os_dev);
#endif

#if ! defined(__CYGWIN__)

static void
strip_extra_slashes (char *dir)
{
  char *p = dir;

  while ((p = strchr (p, '/')) != 0)
    {
      if (p[1] == '/')
	{
	  memmove (p, p + 1, strlen (p));
	  continue;
	}
      else if (p[1] == '\0')
	{
	  if (p > dir)
	    p[0] = '\0';
	  break;
	}

      p++;
    }
}

static char *
xgetcwd (void)
{
  size_t size = 10;
  char *path;

  path = xmalloc (size);
  while (! getcwd (path, size))
    {
      size <<= 1;
      path = xrealloc (path, size);
    }

  return path;
}

#endif

#if !defined (__MINGW32__) && !defined (__CYGWIN__)

#include <sys/types.h>
#include <sys/wait.h>

pid_t
grub_util_exec_pipe (char **argv, int *fd)
{
  int mdadm_pipe[2];
  pid_t mdadm_pid;

  *fd = 0;

  if (pipe (mdadm_pipe) < 0)
    {
      grub_util_warn (_("Unable to create pipe: %s"),
		      strerror (errno));
      return 0;
    }
  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (mdadm_pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
      grub_util_devmapper_cleanup ();
      grub_diskfilter_fini ();

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      close (mdadm_pipe[0]);
      dup2 (mdadm_pipe[1], STDOUT_FILENO);
      close (mdadm_pipe[1]);

      execvp (argv[0], argv);
      exit (127);
    }
  else
    {
      close (mdadm_pipe[1]);
      *fd = mdadm_pipe[0];
      return mdadm_pid;
    }
}

#endif

#if !defined (__CYGWIN__) && !defined(__MINGW32__) && !defined (__GNU__)
char **
grub_util_find_root_devices_from_poolname (char *poolname)
{
  char **devices = 0;
  size_t ndevices = 0;
  size_t devices_allocated = 0;

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  zpool_handle_t *zpool;
  libzfs_handle_t *libzfs;
  nvlist_t *config, *vdev_tree;
  nvlist_t **children;
  unsigned int nvlist_count;
  unsigned int i;
  char *device = 0;

  libzfs = grub_get_libzfs_handle ();
  if (! libzfs)
    return NULL;

  zpool = zpool_open (libzfs, poolname);
  config = zpool_get_config (zpool, NULL);

  if (nvlist_lookup_nvlist (config, "vdev_tree", &vdev_tree) != 0)
    error (1, errno, "nvlist_lookup_nvlist (\"vdev_tree\")");

  if (nvlist_lookup_nvlist_array (vdev_tree, "children", &children, &nvlist_count) != 0)
    error (1, errno, "nvlist_lookup_nvlist_array (\"children\")");
  assert (nvlist_count > 0);

  while (nvlist_lookup_nvlist_array (children[0], "children",
				     &children, &nvlist_count) == 0)
    assert (nvlist_count > 0);

  for (i = 0; i < nvlist_count; i++)
    {
      if (nvlist_lookup_string (children[i], "path", &device) != 0)
	error (1, errno, "nvlist_lookup_string (\"path\")");

      struct stat st;
      if (stat (device, &st) == 0)
	{
#ifdef __sun__
	  if (grub_memcmp (device, "/dev/dsk/", sizeof ("/dev/dsk/") - 1)
	      == 0)
	    device = xasprintf ("/dev/rdsk/%s",
				device + sizeof ("/dev/dsk/") - 1);
	  else if (grub_memcmp (device, "/devices", sizeof ("/devices") - 1)
		   == 0
		   && grub_memcmp (device + strlen (device) - 4,
				   ",raw", 4) != 0)
	    device = xasprintf ("%s,raw", device);
	  else
#endif
	    device = xstrdup (device);
	  if (ndevices >= devices_allocated)
	    {
	      devices_allocated = 2 * (devices_allocated + 8);
	      devices = xrealloc (devices, sizeof (devices[0])
				  * devices_allocated);
	    }
	  devices[ndevices++] = device;
	}

      device = NULL;
    }

  zpool_close (zpool);
#else
  FILE *fp;
  int ret;
  char *line;
  size_t len;
  int st;

  char name[PATH_MAX + 1], state[257], readlen[257], writelen[257];
  char cksum[257], notes[257];
  unsigned int dummy;
  char *argv[4];
  pid_t pid;
  int fd;

  /* execvp has inconvenient types, hence the casts.  None of these
     strings will actually be modified.  */
  argv[0] = (char *) "zpool";
  argv[1] = (char *) "status";
  argv[2] = (char *) poolname;
  argv[3] = NULL;

  pid = grub_util_exec_pipe (argv, &fd);
  if (!pid)
    return NULL;

  fp = fdopen (fd, "r");
  if (!fp)
    {
      grub_util_warn (_("Unable to open stream from %s: %s"),
		      "zpool", strerror (errno));
      goto out;
    }

  st = 0;
  while (1)
    {
      line = NULL;
      ret = getline (&line, &len, fp);
      if (ret == -1)
	break;
	
      if (sscanf (line, " %s %256s %256s %256s %256s %256s",
		  name, state, readlen, writelen, cksum, notes) >= 5)
	switch (st)
	  {
	  case 0:
	    if (!strcmp (name, "NAME")
		&& !strcmp (state, "STATE")
		&& !strcmp (readlen, "READ")
		&& !strcmp (writelen, "WRITE")
		&& !strcmp (cksum, "CKSUM"))
	      st++;
	    break;
	  case 1:
	    {
	      char *ptr = line;
	      while (1)
		{
		  if (strncmp (ptr, poolname, strlen (poolname)) == 0
		      && grub_isspace(ptr[strlen (poolname)]))
		    st++;
		  if (!grub_isspace (*ptr))
		    break;
		  ptr++;
		}
	    }
	    break;
	  case 2:
	    if (strcmp (name, "mirror") && !sscanf (name, "mirror-%u", &dummy)
		&& !sscanf (name, "raidz%u", &dummy)
		&& !sscanf (name, "raidz1%u", &dummy)
		&& !sscanf (name, "raidz2%u", &dummy)
		&& !sscanf (name, "raidz3%u", &dummy)
		&& !strcmp (state, "ONLINE"))
	      {
		if (ndevices >= devices_allocated)
		  {
		    devices_allocated = 2 * (devices_allocated + 8);
		    devices = xrealloc (devices, sizeof (devices[0])
					* devices_allocated);
		  }
		if (name[0] == '/')
		  devices[ndevices++] = xstrdup (name);
		else
		  devices[ndevices++] = xasprintf ("/dev/%s", name);
	      }
	    break;
	  }
	
      free (line);
    }

 out:
  close (fd);
  waitpid (pid, NULL, 0);
#endif
  if (devices)
    {
      if (ndevices >= devices_allocated)
	{
	  devices_allocated = 2 * (devices_allocated + 8);
	  devices = xrealloc (devices, sizeof (devices[0])
			      * devices_allocated);
	}
      devices[ndevices++] = 0;
    }
  return devices;
}

#endif

#if !defined (__MINGW32__) && !defined (__CYGWIN__) && !defined (__GNU__)

static char **
find_root_devices_from_libzfs (const char *dir)
{
  char **devices = NULL;
  char *poolname;
  char *poolfs;

  grub_find_zpool_from_dir (dir, &poolname, &poolfs);
  if (! poolname)
    return NULL;

  devices = grub_util_find_root_devices_from_poolname (poolname);

  free (poolname);
  if (poolfs)
    free (poolfs);

  return devices;
}

#endif

#ifdef __MINGW32__

char *
grub_find_device (const char *dir __attribute__ ((unused)),
                  dev_t dev __attribute__ ((unused)))
{
  return 0;
}

#elif ! defined(__CYGWIN__)

char *
grub_find_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;

  if (! dir)
    {
#ifdef __CYGWIN__
      return NULL;
#else
      dir = "/dev";
#endif
    }

  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  grub_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }

  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;

      /* Avoid:
	 - dotfiles (like "/dev/.tmp.md0") since they could be duplicates.
	 - dotdirs (like "/dev/.static") since they could contain duplicates.  */
      if (ent->d_name[0] == '.')
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode)) {
#ifdef __linux__
	if (strcmp (dir, "mapper") == 0 || strcmp (dir, "/dev/mapper") == 0) {
	  /* Follow symbolic links under /dev/mapper/; the canonical name
	     may be something like /dev/dm-0, but the names under
	     /dev/mapper/ are more human-readable and so we prefer them if
	     we can get them.  */
	  if (stat (ent->d_name, &st) < 0)
	    continue;
	} else
#endif /* __linux__ */
	/* Don't follow other symbolic links.  */
	continue;
      }

      if (S_ISDIR (st.st_mode))
	{
	  /* Find it recursively.  */
	  char *res;

	  res = grub_find_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		grub_util_error ("%s",
				 _("cannot restore the original directory"));

	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
      if (S_ISCHR (st.st_mode) && st.st_rdev == dev)
#else
      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
#endif
	{
#ifdef __linux__
	  /* Skip device names like /dev/dm-0, which are short-hand aliases
	     to more descriptive device names, e.g. those under /dev/mapper */
	  if (ent->d_name[0] == 'd' &&
	      ent->d_name[1] == 'm' &&
	      ent->d_name[2] == '-' &&
	      ent->d_name[3] >= '0' &&
	      ent->d_name[3] <= '9')
	    continue;
#endif

	  /* Found!  */
	  char *res;
	  char *cwd;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	  /* Convert this block device to its character (raw) device.  */
	  const char *template = "%s/r%s";
#else
	  /* Keep the device name as it is.  */
	  const char *template = "%s/%s";
#endif

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 3);
	  sprintf (res, template, cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  /* /dev/root is not a real block device keep looking, takes care
	     of situation where root filesystem is on the same partition as
	     grub files */

	  if (strcmp(res, "/dev/root") == 0)
	    {
	      free (res);
	      continue;
	    }

	  if (chdir (saved_cwd) < 0)
	    grub_util_error ("%s", _("cannot restore the original directory"));

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    grub_util_error ("%s", _("cannot restore the original directory"));

  free (saved_cwd);
  closedir (dp);
  return 0;
}

#else /* __CYGWIN__ */

/* Read drive/partition serial number from mbr/boot sector,
   return 0 on read error, ~0 on unknown serial.  */
static unsigned
get_bootsec_serial (const char *os_dev, int mbr)
{
  /* Read boot sector.  */
  int fd = open (os_dev, O_RDONLY);
  if (fd < 0)
    return 0;
  unsigned char buf[0x200];
  int n = read (fd, buf, sizeof (buf));
  close (fd);
  if (n != sizeof(buf))
    return 0;

  /* Check signature.  */
  if (!(buf[0x1fe] == 0x55 && buf[0x1ff] == 0xaa))
    return ~0;

  /* Serial number offset depends on boot sector type.  */
  if (mbr)
    n = 0x1b8;
  else if (memcmp (buf + 0x03, "NTFS", 4) == 0)
    n = 0x048;
  else if (memcmp (buf + 0x52, "FAT32", 5) == 0)
    n = 0x043;
  else if (memcmp (buf + 0x36, "FAT", 3) == 0)
    n = 0x027;
  else
    return ~0;

  unsigned serial = *(unsigned *)(buf + n);
  if (serial == 0)
    return ~0;
  return serial;
}

#pragma GCC diagnostic warning "-Wdeprecated-declarations"

char *
grub_find_device (const char *path, dev_t dev)
{
  /* No root device for /cygdrive.  */
  if (dev == (DEV_CYGDRIVE_MAJOR << 16))
    return 0;

  /* Convert to full POSIX and Win32 path.  */
  char fullpath[PATH_MAX], winpath[PATH_MAX];

  cygwin_conv_path (CCP_WIN_A_TO_POSIX, path, fullpath, sizeof (fullpath));
  cygwin_conv_path (CCP_POSIX_TO_WIN_A, fullpath, winpath, sizeof (winpath));

  /* If identical, this is no real filesystem path.  */
  if (strcmp (fullpath, winpath) == 0)
    return 0;

  /* Check for floppy drive letter.  */
  if (winpath[0] && winpath[1] == ':' && strchr ("AaBb", winpath[0]))
    return xstrdup (strchr ("Aa", winpath[0]) ? "/dev/fd0" : "/dev/fd1");

  /* Cygwin returns the partition serial number in stat.st_dev.
     This is never identical to the device number of the emulated
     /dev/sdXN device, so above grub_find_device () does not work.
     Search the partition with the same serial in boot sector instead.  */
  char devpath[sizeof ("/dev/sda15") + 13]; /* Size + Paranoia.  */
  int d;
  for (d = 'a'; d <= 'z'; d++)
    {
      sprintf (devpath, "/dev/sd%c", d);
      if (get_bootsec_serial (devpath, 1) == 0)
	continue;
      int p;
      for (p = 1; p <= 15; p++)
	{
	  sprintf (devpath, "/dev/sd%c%d", d, p);
	  unsigned ser = get_bootsec_serial (devpath, 0);
	  if (ser == 0)
	    break;
	  if (ser != (unsigned)~0 && dev == (dev_t)ser)
	    return xstrdup (devpath);
	}
    }
  return 0;
}

#endif /* __CYGWIN__ */

char **
grub_guess_root_devices (const char *dir)
{
  char **os_dev = NULL;
#ifndef __GNU__
  struct stat st;
  dev_t dev;

#ifdef __linux__
  if (!os_dev)
    os_dev = grub_find_root_devices_from_mountinfo (dir, NULL);
#endif /* __linux__ */

#if !defined (__MINGW32__) && !defined (__CYGWIN__)
  if (!os_dev)
    os_dev = find_root_devices_from_libzfs (dir);
#endif

  if (os_dev)
    {
      char **cur;
      for (cur = os_dev; *cur; cur++)
	{
	  char *tmp = *cur;
	  int root, dm;
	  if (strcmp (*cur, "/dev/root") == 0
	      || strncmp (*cur, "/dev/dm-", sizeof ("/dev/dm-") - 1) == 0)
	    *cur = tmp;
	  else
	    {
	      *cur = canonicalize_file_name (tmp);
	      if (*cur == NULL)
		grub_util_error (_("failed to get canonical path of `%s'"), tmp);
	      free (tmp);
	    }
	  root = (strcmp (*cur, "/dev/root") == 0);
	  dm = (strncmp (*cur, "/dev/dm-", sizeof ("/dev/dm-") - 1) == 0);
	  if (!dm && !root)
	    continue;
	  if (stat (*cur, &st) < 0)
	    break;
	  free (*cur);
	  dev = st.st_rdev;
	  *cur = grub_find_device (dm ? "/dev/mapper" : "/dev", dev);
	}
      if (!*cur)
	return os_dev;
      for (cur = os_dev; *cur; cur++)
	free (*cur);
      free (os_dev);
      os_dev = 0;
    }

  if (stat (dir, &st) < 0)
    grub_util_error (_("cannot stat `%s': %s"), dir, strerror (errno));

  dev = st.st_dev;
#endif /* !__GNU__ */

  os_dev = xmalloc (2 * sizeof (os_dev[0]));
  
#ifdef __CYGWIN__
  /* Cygwin specific function.  */
  os_dev[0] = grub_find_device (dir, dev);

#elif defined __GNU__
  /* GNU/Hurd specific function.  */
  os_dev[0] = grub_util_find_hurd_root_device (dir);

#else

  /* This might be truly slow, but is there any better way?  */
  os_dev[0] = grub_find_device ("/dev", dev);
#endif
  if (!os_dev[0])
    {
      free (os_dev);
      return 0;
    }

  os_dev[1] = 0;

  return os_dev;
}

grub_disk_addr_t
grub_util_find_partition_start (const char *dev)
{
  grub_disk_addr_t partition_start;
  if (grub_util_device_is_mapped (dev)
      && grub_util_get_dm_node_linear_info (dev, 0, 0, &partition_start))
    return partition_start;

  return grub_util_find_partition_start_os (dev);
}

void
grub_util_pull_device (const char *os_dev)
{
  enum grub_dev_abstraction_types ab;
  ab = grub_util_get_dev_abstraction (os_dev);
  switch (ab)
    {
    case GRUB_DEV_ABSTRACTION_LVM:
#if !defined (__MINGW32__) && !defined (__CYGWIN__)
      pull_lvm_by_command (os_dev);
#endif
      /* Fallthrough in case that lvm-tools are unavailable.  */
    case GRUB_DEV_ABSTRACTION_LUKS:
      grub_util_pull_devmapper (os_dev);
      return;

    default:
      if (grub_util_pull_device_os (os_dev, ab))
	return;
    case GRUB_DEV_ABSTRACTION_NONE:
      free (grub_util_biosdisk_get_grub_dev (os_dev));
      return;
    }
}

char *
grub_util_get_grub_dev (const char *os_dev)
{
  char *ret;

  grub_util_pull_device (os_dev);

  ret = grub_util_get_devmapper_grub_dev (os_dev);
  if (ret)
    return ret;
  ret = grub_util_get_grub_dev_os (os_dev);
  if (ret)
    return ret;
  return grub_util_biosdisk_get_grub_dev (os_dev);
}


int
grub_util_get_dev_abstraction (const char *os_dev)
{
  enum grub_dev_abstraction_types ret;

  /* User explicitly claims that this drive is visible by BIOS.  */
  if (grub_util_biosdisk_is_present (os_dev))
    return GRUB_DEV_ABSTRACTION_NONE;

  /* Check for LVM and LUKS.  */
  ret = grub_util_get_dm_abstraction (os_dev);

  if (ret != GRUB_DEV_ABSTRACTION_NONE)
    return ret;

  return grub_util_get_dev_abstraction_os (os_dev);
}

#if !defined (__MINGW32__) && !defined (__CYGWIN__)

static void
pull_lvm_by_command (const char *os_dev)
{
  char *argv[8];
  int fd;
  pid_t pid;
  FILE *mdadm;
  char *buf = NULL;
  size_t len = 0;
  char *vgname = NULL;
  const char *iptr;
  char *optr;
  char *vgid = NULL;
  grub_size_t vgidlen = 0;

  vgid = grub_util_get_vg_uuid (os_dev);
  if (vgid)
    vgidlen = grub_strlen (vgid);

  if (!vgid)
    {
      if (strncmp (os_dev, LVM_DEV_MAPPER_STRING,
		   sizeof (LVM_DEV_MAPPER_STRING) - 1)
	  != 0)
	return;

      vgname = xmalloc (strlen (os_dev + sizeof (LVM_DEV_MAPPER_STRING) - 1) + 1);
      for (iptr = os_dev + sizeof (LVM_DEV_MAPPER_STRING) - 1, optr = vgname; *iptr; )
	if (*iptr != '-')
	  *optr++ = *iptr++;
	else if (iptr[0] == '-' && iptr[1] == '-')
	  {
	    iptr += 2;
	    *optr++ = '-';
	  }
	else
	  break;
      *optr = '\0';
    }

  /* execvp has inconvenient types, hence the casts.  None of these
     strings will actually be modified.  */
  /* by default PV name is left aligned in 10 character field, meaning that
     we do not know where name ends. Using dummy --separator disables
     alignment. We have a single field, so separator itself is not output */
  argv[0] = (char *) "vgs";
  argv[1] = (char *) "--options";
  if (vgid)
    argv[2] = (char *) "vg_uuid,pv_name";
  else
    argv[2] = (char *) "pv_name";
  argv[3] = (char *) "--noheadings";
  argv[4] = (char *) "--separator";
  argv[5] = (char *) ":";
  argv[6] = vgname;
  argv[7] = NULL;

  pid = grub_util_exec_pipe (argv, &fd);
  free (vgname);

  if (!pid)
    return;

  /* Parent.  Read mdadm's output.  */
  mdadm = fdopen (fd, "r");
  if (! mdadm)
    {
      grub_util_warn (_("Unable to open stream from %s: %s"),
		      "vgs", strerror (errno));
      goto out;
    }

  while (getline (&buf, &len, mdadm) > 0)
    {
      char *ptr;
      /* LVM adds two spaces as standard prefix */
      for (ptr = buf; ptr < buf + 2 && *ptr == ' '; ptr++);

      if (vgid && (grub_strncmp (vgid, ptr, vgidlen) != 0
		   || ptr[vgidlen] != ':'))
	continue;
      if (vgid)
	ptr += vgidlen + 1;
      if (*ptr == '\0')
	continue;
      *(ptr + strlen (ptr) - 1) = '\0';
      grub_util_pull_device (ptr);
    }

out:
  close (fd);
  waitpid (pid, NULL, 0);
  free (buf);
}

#endif

int
grub_util_biosdisk_is_floppy (grub_disk_t disk)
{
  struct stat st;
  int fd;
  const char *dname;

  dname = grub_util_biosdisk_get_osdev (disk);

  if (!dname)
    return 0;

  fd = open (dname, O_RDONLY);
  /* Shouldn't happen.  */
  if (fd == -1)
    return 0;

  /* Shouldn't happen either.  */
  if (fstat (fd, &st) < 0)
    {
      close (fd);
      return 0;
    }

  close (fd);

#if defined(__NetBSD__)
  if (major(st.st_rdev) == RAW_FLOPPY_MAJOR)
    return 1;
#endif

#if defined(FLOPPY_MAJOR)
  if (major(st.st_rdev) == FLOPPY_MAJOR)
#else
  /* Some kernels (e.g. kFreeBSD) don't have a static major number
     for floppies, but they still use a "fd[0-9]" pathname.  */
  if (dname[5] == 'f'
      && dname[6] == 'd'
      && dname[7] >= '0'
      && dname[7] <= '9')
#endif
    return 1;

  return 0;
}

static char *
convert_system_partition_to_system_disk (const char *os_dev, struct stat *st,
					 int *is_part)
{
  *is_part = 0;

  if (grub_util_device_is_mapped_stat (st))
    return grub_util_devmapper_part_to_disk (st, is_part, os_dev);

  *is_part = 0;

  return grub_util_part_to_disk (os_dev, st, is_part);
}

static const char *
find_system_device (const char *os_dev, struct stat *st, int convert, int add)
{
  char *os_disk;
  const char *drive;
  int is_part;

  if (convert)
    os_disk = convert_system_partition_to_system_disk (os_dev, st, &is_part);
  else
    os_disk = xstrdup (os_dev);
  if (! os_disk)
    return NULL;

  drive = grub_hostdisk_os_dev_to_grub_drive (os_disk, add);
  free (os_disk);
  return drive;
}

/*
 * Note: we do not use the new partition naming scheme as dos_part does not
 * necessarily correspond to an msdos partition.
 */
static char *
make_device_name (const char *drive, int dos_part, int bsd_part)
{
  char *ret, *ptr, *end;
  const char *iptr;

  ret = xmalloc (strlen (drive) * 2 
		 + sizeof (",XXXXXXXXXXXXXXXXXXXXXXXXXX"
			   ",XXXXXXXXXXXXXXXXXXXXXXXXXX"));
  end = (ret + strlen (drive) * 2 
	 + sizeof (",XXXXXXXXXXXXXXXXXXXXXXXXXX"
		   ",XXXXXXXXXXXXXXXXXXXXXXXXXX"));
  ptr = ret;
  for (iptr = drive; *iptr; iptr++)
    {
      if (*iptr == ',')
	*ptr++ = '\\';
      *ptr++ = *iptr;
    }
  *ptr = 0;
  if (dos_part >= 0)
    snprintf (ptr, end - ptr, ",%d", dos_part + 1);
  ptr += strlen (ptr);
  if (bsd_part >= 0)
    snprintf (ptr, end - ptr, ",%d", bsd_part + 1); 

  return ret;
}

char *
grub_util_get_os_disk (const char *os_dev)
{
  struct stat st;
  int is_part;

  grub_util_info ("Looking for %s", os_dev);

  if (stat (os_dev, &st) < 0)
    {
      const char *errstr = strerror (errno); 
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot stat `%s': %s"),
		  os_dev, errstr);
      grub_util_info (_("cannot stat `%s': %s"), os_dev, errstr);
      return 0;
    }

  return convert_system_partition_to_system_disk (os_dev, &st, &is_part);
}

#if !defined(__APPLE__)
/* Context for grub_util_biosdisk_get_grub_dev.  */
struct grub_util_biosdisk_get_grub_dev_ctx
{
  char *partname;
  grub_disk_addr_t start;
};

/* Helper for grub_util_biosdisk_get_grub_dev.  */
static int
find_partition (grub_disk_t dsk __attribute__ ((unused)),
		const grub_partition_t partition, void *data)
{
  struct grub_util_biosdisk_get_grub_dev_ctx *ctx = data;
  grub_disk_addr_t part_start = 0;
  grub_util_info ("Partition %d starts from %" PRIuGRUB_UINT64_T,
		  partition->number, partition->start);

  part_start = grub_partition_get_start (partition);

  if (ctx->start == part_start)
    {
      ctx->partname = grub_partition_get_name (partition);
      return 1;
    }

  return 0;
}
#endif

char *
grub_util_biosdisk_get_grub_dev (const char *os_dev)
{
  struct stat st;
  const char *drive;
  char *sys_disk;
  int is_part;

  grub_util_info ("Looking for %s", os_dev);

  if (stat (os_dev, &st) < 0)
    {
      const char *errstr = strerror (errno); 
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot stat `%s': %s"), os_dev,
		  errstr);
      grub_util_info (_("cannot stat `%s': %s"), os_dev, errstr);
      return 0;
    }

  drive = find_system_device (os_dev, &st, 1, 1);
  sys_disk = convert_system_partition_to_system_disk (os_dev, &st, &is_part);
  if (!sys_disk)
    return 0;
  grub_util_info ("%s is a parent of %s", sys_disk, os_dev);
  if (!is_part)
    {
      free (sys_disk);
      return make_device_name (drive, -1, -1);
    }
  free (sys_disk);

#if GRUB_DISK_DEVS_ARE_CHAR
  if (! S_ISCHR (st.st_mode))
#else
  if (! S_ISBLK (st.st_mode))
#endif
    return make_device_name (drive, -1, -1);

#if defined(__APPLE__)
  /* Apple uses "/dev/r?disk[0-9]+(s[0-9]+)?".  */
  {
    const char *p;
    int disk = (grub_memcmp (os_dev, "/dev/disk", sizeof ("/dev/disk") - 1)
		 == 0);
    int rdisk = (grub_memcmp (os_dev, "/dev/rdisk", sizeof ("/dev/rdisk") - 1)
		 == 0);
 
    if (!disk && !rdisk)
      return make_device_name (drive, -1, -1);

    p = os_dev + sizeof ("/dev/disk") + rdisk - 1;
    while (*p >= '0' && *p <= '9')
      p++;
    if (*p != 's')
      return make_device_name (drive, -1, -1);
    p++;

    return make_device_name (drive, strtol (p, NULL, 10) - 1, -1);
  }

#else

  /* Linux counts partitions uniformly, whether a BSD partition or a DOS
     partition, so mapping them to GRUB devices is not trivial.
     Here, get the start sector of a partition by HDIO_GETGEO, and
     compare it with each partition GRUB recognizes.

     Cygwin /dev/sdXN emulation uses Windows partition mapping. It
     does not count the extended partition and missing primary
     partitions.  Use same method as on Linux here.

     For NetBSD and FreeBSD, proceed as for Linux, except that the start
     sector is obtained from the disk label.  */
  {
    char *name;
    grub_disk_t disk;
    struct grub_util_biosdisk_get_grub_dev_ctx ctx;

    name = make_device_name (drive, -1, -1);

# ifdef FLOPPY_MAJOR
    if (MAJOR (st.st_rdev) == FLOPPY_MAJOR)
      return name;
# else
    /* Since os_dev and convert_system_partition_to_system_disk (os_dev) are
     * different, we know that os_dev cannot be a floppy device.  */
# endif

    ctx.start = grub_util_find_partition_start (os_dev);
    if (grub_errno != GRUB_ERR_NONE)
      {
	free (name);
	return 0;
      }

#if defined(__GNU__)
    /* Some versions of Hurd use badly glued Linux code to handle partitions
       resulting in partitions being promoted to disks.  */
    /* GNU uses "/dev/[hs]d[0-9]+(s[0-9]+[a-z]?)?".  */
    if (ctx.start == (grub_disk_addr_t) -1)
      {
	char *p;
	int dos_part = -1;
	int bsd_part = -1;

	p = strrchr (os_dev + sizeof ("/dev/hd") - 1, 's');
	if (p)
	  {
	    long int n;
	    char *q;

	    p++;
	    n = strtol (p, &q, 10);
	    if (p != q && n != GRUB_LONG_MIN && n != GRUB_LONG_MAX)
	      {
		dos_part = (int) n - 1;

		if (*q >= 'a' && *q <= 'g')
		  bsd_part = *q - 'a';
	      }
	  }

	return make_device_name (drive, dos_part, bsd_part);
      }
#endif

    grub_util_info ("%s starts from %" PRIuGRUB_UINT64_T, os_dev, ctx.start);

    if (ctx.start == 0 && !is_part)
      return name;

    grub_util_info ("opening the device %s", name);
    disk = grub_disk_open (name);
    free (name);

    if (! disk)
      {
	/* We already know that the partition exists.  Given that we already
	   checked the device map above, we can only get
	   GRUB_ERR_UNKNOWN_DEVICE at this point if the disk does not exist.
	   This can happen on Xen, where disk images in the host can be
	   assigned to devices that have partition-like names in the guest
	   but are really more like disks.  */
	if (grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
	  {
	    char *canon;
	    grub_util_warn
	      (_("disk does not exist, so falling back to partition device %s"),
	       os_dev);
	    grub_errno = GRUB_ERR_NONE;

	    canon = canonicalize_file_name (os_dev);
	    drive = find_system_device (canon ? : os_dev, &st, 0, 1);
	    if (canon)
	      free (canon);
	    return make_device_name (drive, -1, -1);
	  }
	else
	  return 0;
      }

    name = grub_util_get_ldm (disk, ctx.start);
    if (name)
      return name;

    ctx.partname = NULL;

    grub_partition_iterate (disk, find_partition, &ctx);
    if (grub_errno != GRUB_ERR_NONE)
      {
	grub_disk_close (disk);
	return 0;
      }

    if (ctx.partname == NULL)
      {
	grub_disk_close (disk);
	grub_util_info ("cannot find the partition of `%s'", os_dev);
	grub_error (GRUB_ERR_BAD_DEVICE,
		    "cannot find the partition of `%s'", os_dev);
	return 0;
      }

    name = grub_xasprintf ("%s,%s", disk->name, ctx.partname);
    free (ctx.partname);
    grub_disk_close (disk);
    return name;
  }

#endif
}

int
grub_util_biosdisk_is_present (const char *os_dev)
{
  struct stat st;

  if (stat (os_dev, &st) < 0)
    return 0;

  int ret= (find_system_device (os_dev, &st, 1, 0) != NULL);
  grub_util_info ((ret ? "%s is present" : "%s is not present"), 
		  os_dev);
  return ret;
}

const char *
grub_util_check_block_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error (_("cannot stat `%s': %s"), blk_dev,
		     strerror (errno));

  if (S_ISBLK (st.st_mode))
    return (blk_dev);
  else
    return 0;
}

const char *
grub_util_check_char_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error (_("cannot stat `%s': %s"), blk_dev, strerror (errno));

  if (S_ISCHR (st.st_mode))
    return (blk_dev);
  else
    return 0;
}

#ifdef __CYGWIN__
/* Convert POSIX path to Win32 path,
   remove drive letter, replace backslashes.  */
static char *
get_win32_path (const char *path)
{
  char winpath[PATH_MAX];
  if (cygwin_conv_path (CCP_POSIX_TO_WIN_A, path, winpath, sizeof(winpath)))
    grub_util_error ("%s", _("cygwin_conv_path() failed"));

  int len = strlen (winpath);
  int offs = (len > 2 && winpath[1] == ':' ? 2 : 0);

  int i;
  for (i = offs; i < len; i++)
    if (winpath[i] == '\\')
      winpath[i] = '/';
  return xstrdup (winpath + offs);
}
#endif

#ifdef HAVE_LIBZFS
static libzfs_handle_t *__libzfs_handle;

static void
fini_libzfs (void)
{
  libzfs_fini (__libzfs_handle);
}

libzfs_handle_t *
grub_get_libzfs_handle (void)
{
  if (! __libzfs_handle)
    {
      __libzfs_handle = libzfs_init ();

      if (__libzfs_handle)
	atexit (fini_libzfs);
    }

  return __libzfs_handle;
}
#endif /* HAVE_LIBZFS */

#if !defined (__MINGW32__) && !defined (__CYGWIN__)
/* ZFS has similar problems to those of btrfs (see above).  */
void
grub_find_zpool_from_dir (const char *dir, char **poolname, char **poolfs)
{
  char *slash;

  *poolname = *poolfs = NULL;

#if defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) && defined(HAVE_STRUCT_STATFS_F_MNTFROMNAME)
  /* FreeBSD and GNU/kFreeBSD.  */
  {
    struct statfs mnt;

    if (statfs (dir, &mnt) != 0)
      return;

    if (strcmp (mnt.f_fstypename, "zfs") != 0)
      return;

    *poolname = xstrdup (mnt.f_mntfromname);
  }
#elif defined(HAVE_GETEXTMNTENT)
  /* Solaris.  */
  {
    struct stat st;
    struct extmnttab mnt;

    if (stat (dir, &st) != 0)
      return;

    FILE *mnttab = fopen ("/etc/mnttab", "r");
    if (! mnttab)
      return;

    while (getextmntent (mnttab, &mnt, sizeof (mnt)) == 0)
      {
	if (makedev (mnt.mnt_major, mnt.mnt_minor) == st.st_dev
	    && !strcmp (mnt.mnt_fstype, "zfs"))
	  {
	    *poolname = xstrdup (mnt.mnt_special);
	    break;
	  }
      }

    fclose (mnttab);
  }
#endif

  if (! *poolname)
    return;

  slash = strchr (*poolname, '/');
  if (slash)
    {
      *slash = '\0';
      *poolfs = xstrdup (slash + 1);
    }
  else
    *poolfs = xstrdup ("");
}
#endif

/* This function never prints trailing slashes (so that its output
   can be appended a slash unconditionally).  */
char *
grub_make_system_path_relative_to_its_root (const char *path)
{
  struct stat st;
  char *p, *buf, *buf2, *buf3, *ret;
  uintptr_t offset = 0;
  dev_t num;
  size_t len;
  char *poolfs = NULL;

  /* canonicalize.  */
  p = canonicalize_file_name (path);
  if (p == NULL)
    grub_util_error (_("failed to get canonical path of `%s'"), path);

  /* For ZFS sub-pool filesystems, could be extended to others (btrfs?).  */
#if !defined (__MINGW32__) && !defined (__CYGWIN__)
  {
    char *dummy;
    grub_find_zpool_from_dir (p, &dummy, &poolfs);
  }
#endif

  len = strlen (p) + 1;
  buf = xstrdup (p);
  free (p);

  if (stat (buf, &st) < 0)
    grub_util_error (_("cannot stat `%s': %s"), buf, strerror (errno));

  buf2 = xstrdup (buf);
  num = st.st_dev;

  /* This loop sets offset to the number of chars of the root
     directory we're inspecting.  */
  while (1)
    {
      p = strrchr (buf, '/');
      if (p == NULL)
	/* This should never happen.  */
	grub_util_error ("%s",
			 /* TRANSLATORS: canonical pathname is the
			    complete one e.g. /etc/fstab. It has
			    to contain `/' normally, if it doesn't
			    we're in trouble and throw this error.  */
			 _("no `/' in canonical filename"));
      if (p != buf)
	*p = 0;
      else
	*++p = 0;

      if (stat (buf, &st) < 0)
	grub_util_error (_("cannot stat `%s': %s"), buf, strerror (errno));

      /* buf is another filesystem; we found it.  */
      if (st.st_dev != num)
	{
	  /* offset == 0 means path given is the mount point.
	     This works around special-casing of "/" in Un*x.  This function never
	     prints trailing slashes (so that its output can be appended a slash
	     unconditionally).  Each slash in is considered a preceding slash, and
	     therefore the root directory is an empty string.  */
	  if (offset == 0)
	    {
	      free (buf);
#ifdef __linux__
	      {
		char *bind;
		grub_free (grub_find_root_devices_from_mountinfo (buf2, &bind));
		if (bind && bind[0] && bind[1])
		  {
		    buf3 = bind;
		    goto parsedir;
		  }
		grub_free (bind);
	      }
#endif
	      free (buf2);
	      if (poolfs)
		return xasprintf ("/%s/@", poolfs);
	      return xstrdup ("");
	    }
	  else
	    break;
	}

      offset = p - buf;
      /* offset == 1 means root directory.  */
      if (offset == 1)
	{
	  /* Include leading slash.  */
	  offset = 0;
	  break;
	}
    }
  free (buf);
  buf3 = xstrdup (buf2 + offset);
  buf2[offset] = 0;
#ifdef __linux__
  {
    char *bind;
    grub_free (grub_find_root_devices_from_mountinfo (buf2, &bind));
    if (bind && bind[0] && bind[1])
      {
	char *temp = buf3;
	buf3 = grub_xasprintf ("%s%s%s", bind, buf3[0] == '/' ?"":"/", buf3);
	grub_free (temp);
      }
    grub_free (bind);
  }
#endif
  
  free (buf2);

#ifdef __CYGWIN__
  if (st.st_dev != (DEV_CYGDRIVE_MAJOR << 16))
    {
      /* Reached some mount point not below /cygdrive.
	 GRUB does not know Cygwin's emulated mounts,
	 convert to Win32 path.  */
      grub_util_info ("Cygwin path = %s\n", buf3);
      char * temp = get_win32_path (buf3);
      free (buf3);
      buf3 = temp;
    }
#endif

#ifdef __linux__
 parsedir:
#endif
  /* Remove trailing slashes, return empty string if root directory.  */
  len = strlen (buf3);
  while (len > 0 && buf3[len - 1] == '/')
    {
      buf3[len - 1] = '\0';
      len--;
    }

  if (poolfs)
    {
      ret = xasprintf ("/%s/@%s", poolfs, buf3);
      free (buf3);
    }
  else
    ret = buf3;

  return ret;
}
