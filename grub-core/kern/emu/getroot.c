/* getroot.c - Get root device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/util/misc.h>
#include <grub/cryptodisk.h>

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>
#endif

#ifdef __GNU__
#include <hurd.h>
#include <hurd/lookup.h>
#include <hurd/fs.h>
#include <sys/mman.h>
#endif

#ifdef __linux__
# include <sys/types.h>
# include <sys/wait.h>
#endif

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
# include <grub/util/libzfs.h>
# include <grub/util/libnvpair.h>
#endif

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>

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

#ifdef __linux__

struct mountinfo_entry
{
  int id;
  int major, minor;
  char enc_root[PATH_MAX], enc_path[PATH_MAX];
  char fstype[PATH_MAX], device[PATH_MAX];
};

/* Statting something on a btrfs filesystem always returns a virtual device
   major/minor pair rather than the real underlying device, because btrfs
   can span multiple underlying devices (and even if it's currently only
   using a single device it can be dynamically extended onto another).  We
   can't deal with the multiple-device case yet, but in the meantime, we can
   at least cope with the single-device case by scanning
   /proc/self/mountinfo.  */
char *
grub_find_root_device_from_mountinfo (const char *dir, char **relroot)
{
  FILE *fp;
  char *buf = NULL;
  size_t len = 0;
  char *ret = NULL;
  int entry_len = 0, entry_max = 4;
  struct mountinfo_entry *entries;
  struct mountinfo_entry parent_entry = { 0, 0, 0, "", "", "", "" };
  int i;

  if (! *dir)
    dir = "/";
  if (relroot)
    *relroot = NULL;

  fp = fopen ("/proc/self/mountinfo", "r");
  if (! fp)
    return NULL; /* fall through to other methods */

  entries = xmalloc (entry_max * sizeof (*entries));

  /* First, build a list of relevant visible mounts.  */
  while (getline (&buf, &len, fp) > 0)
    {
      struct mountinfo_entry entry;
      int count;
      size_t enc_path_len;
      const char *sep;

      if (sscanf (buf, "%d %d %u:%u %s %s%n",
		  &entry.id, &parent_entry.id, &entry.major, &entry.minor,
		  entry.enc_root, entry.enc_path, &count) < 6)
	continue;

      enc_path_len = strlen (entry.enc_path);
      /* Check that enc_path is a prefix of dir.  The prefix must either be
         the entire string, or end with a slash, or be immediately followed
         by a slash.  */
      if (strncmp (dir, entry.enc_path, enc_path_len) != 0 ||
	  (enc_path_len && dir[enc_path_len - 1] != '/' &&
	   dir[enc_path_len] && dir[enc_path_len] != '/'))
	continue;

      sep = strstr (buf + count, " - ");
      if (!sep)
	continue;

      sep += sizeof (" - ") - 1;
      if (sscanf (sep, "%s %s", entry.fstype, entry.device) != 2)
	continue;

      /* Using the mount IDs, find out where this fits in the list of
	 visible mount entries we've seen so far.  There are three
	 interesting cases.  Firstly, it may be inserted at the end: this is
	 the usual case of /foo/bar being mounted after /foo.  Secondly, it
	 may be inserted at the start: for example, this can happen for
	 filesystems that are mounted before / and later moved under it.
	 Thirdly, it may occlude part or all of the existing filesystem
	 tree, in which case the end of the list needs to be pruned and this
	 new entry will be inserted at the end.  */
      if (entry_len >= entry_max)
	{
	  entry_max <<= 1;
	  entries = xrealloc (entries, entry_max * sizeof (*entries));
	}

      if (!entry_len)
	{
	  /* Initialise list.  */
	  entry_len = 2;
	  entries[0] = parent_entry;
	  entries[1] = entry;
	}
      else
	{
	  for (i = entry_len - 1; i >= 0; i--)
	    {
	      if (entries[i].id == parent_entry.id)
		{
		  /* Insert at end, pruning anything previously above this.  */
		  entry_len = i + 2;
		  entries[i + 1] = entry;
		  break;
		}
	      else if (i == 0 && entries[i].id == entry.id)
		{
		  /* Insert at start.  */
		  entry_len++;
		  memmove (entries + 1, entries,
			   (entry_len - 1) * sizeof (*entries));
		  entries[0] = parent_entry;
		  entries[1] = entry;
		  break;
		}
	    }
	}
    }

  /* Now scan visible mounts for the ones we're interested in.  */
  for (i = entry_len - 1; i >= 0; i--)
    {
      if (!*entries[i].device)
	continue;

      ret = strdup (entries[i].device);
      if (relroot)
	*relroot = strdup (entries[i].enc_root);
      break;
    }

  free (buf);
  free (entries);
  fclose (fp);
  return ret;
}

#endif /* __linux__ */

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
static char *
find_root_device_from_libzfs (const char *dir)
{
  char *device = NULL;
  char *poolname;
  char *poolfs;

  grub_find_zpool_from_dir (dir, &poolname, &poolfs);
  if (! poolname)
    return NULL;

  {
    zpool_handle_t *zpool;
    libzfs_handle_t *libzfs;
    nvlist_t *config, *vdev_tree;
    nvlist_t **children, **path;
    unsigned int nvlist_count;
    unsigned int i;

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
	    device = xstrdup (device);
	    break;
	  }

	device = NULL;
      }

    zpool_close (zpool);
  }

  free (poolname);
  if (poolfs)
    free (poolfs);

  return device;
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
		grub_util_error ("cannot restore the original directory");

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
#if defined(__NetBSD__)
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
		continue;

	  if (chdir (saved_cwd) < 0)
	    grub_util_error ("cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    grub_util_error ("cannot restore the original directory");

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

char *
grub_find_device (const char *path, dev_t dev)
{
  /* No root device for /cygdrive.  */
  if (dev == (DEV_CYGDRIVE_MAJOR << 16))
    return 0;

  /* Convert to full POSIX and Win32 path.  */
  char fullpath[PATH_MAX], winpath[PATH_MAX];
  cygwin_conv_to_full_posix_path (path, fullpath);
  cygwin_conv_to_full_win32_path (fullpath, winpath);

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

char *
grub_guess_root_device (const char *dir)
{
  char *os_dev = NULL;
#ifdef __GNU__
  file_t file;
  mach_port_t *ports;
  int *ints;
  loff_t *offsets;
  char *data;
  error_t err;
  mach_msg_type_number_t num_ports = 0, num_ints = 0, num_offsets = 0, data_len = 0;
  size_t name_len;

  file = file_name_lookup (dir, 0, 0);
  if (file == MACH_PORT_NULL)
    return 0;

  err = file_get_storage_info (file,
			       &ports, &num_ports,
			       &ints, &num_ints,
			       &offsets, &num_offsets,
			       &data, &data_len);

  if (num_ints < 1)
    grub_util_error ("Storage info for `%s' does not include type", dir);
  if (ints[0] != STORAGE_DEVICE)
    grub_util_error ("Filesystem of `%s' is not stored on local disk", dir);

  if (num_ints < 5)
    grub_util_error ("Storage info for `%s' does not include name", dir);
  name_len = ints[4];
  if (name_len < data_len)
    grub_util_error ("Bogus name length for storage info for `%s'", dir);
  if (data[name_len - 1] != '\0')
    grub_util_error ("Storage name for `%s' not NUL-terminated", dir);

  os_dev = xmalloc (strlen ("/dev/") + data_len);
  memcpy (os_dev, "/dev/", strlen ("/dev/"));
  memcpy (os_dev + strlen ("/dev/"), data, data_len);

  if (ports && num_ports > 0)
    {
      mach_msg_type_number_t i;
      for (i = 0; i < num_ports; i++)
        {
	  mach_port_t port = ports[i];
	  if (port != MACH_PORT_NULL)
	    mach_port_deallocate (mach_task_self(), port);
        }
      munmap ((caddr_t) ports, num_ports * sizeof (*ports));
    }

  if (ints && num_ints > 0)
    munmap ((caddr_t) ints, num_ints * sizeof (*ints));
  if (offsets && num_offsets > 0)
    munmap ((caddr_t) offsets, num_offsets * sizeof (*offsets));
  if (data && data_len > 0)
    munmap (data, data_len);
  mach_port_deallocate (mach_task_self (), file);
#else /* !__GNU__ */
  struct stat st;
  dev_t dev;

#ifdef __linux__
  if (!os_dev)
    os_dev = grub_find_root_device_from_mountinfo (dir, NULL);
#endif /* __linux__ */

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  if (!os_dev)
    os_dev = find_root_device_from_libzfs (dir);
#endif

  if (os_dev)
    {
      char *tmp = os_dev;
      os_dev = canonicalize_file_name (os_dev);
      free (tmp);
    }

  if (os_dev)
    {
      int dm = (strncmp (os_dev, "/dev/dm-", sizeof ("/dev/dm-") - 1) == 0);
      int root = (strcmp (os_dev, "/dev/root") == 0);
      if (!dm && !root)
	return os_dev;
      if (stat (os_dev, &st) >= 0)
	{
	  free (os_dev);
	  dev = st.st_rdev;
	  return grub_find_device (dm ? "/dev/mapper" : "/dev", dev);
	}
      free (os_dev);
    }

  if (stat (dir, &st) < 0)
    grub_util_error ("cannot stat `%s'", dir);

  dev = st.st_dev;
  
#ifdef __CYGWIN__
  /* Cygwin specific function.  */
  os_dev = grub_find_device (dir, dev);

#else

  /* This might be truly slow, but is there any better way?  */
  os_dev = grub_find_device ("/dev", dev);
#endif
#endif /* !__GNU__ */

  return os_dev;
}

#ifdef HAVE_DEVICE_MAPPER

static int
grub_util_open_dm (const char *os_dev, struct dm_tree **tree,
		   struct dm_tree_node **node)
{
  uint32_t maj, min;
  struct stat st;

  *node = NULL;
  *tree = NULL;

  if ((strncmp ("/dev/mapper/", os_dev, 12) != 0))
    return 0;

  if (stat (os_dev, &st) < 0)
    return 0;

  *tree = dm_tree_create ();
  if (! *tree)
    {
      grub_printf ("Failed to create tree\n");
      grub_dprintf ("hostdisk", "dm_tree_create failed\n");
      return 0;
    }

  maj = major (st.st_rdev);
  min = minor (st.st_rdev);

  if (! dm_tree_add_dev (*tree, maj, min))
    {
      grub_dprintf ("hostdisk", "dm_tree_add_dev failed\n");
      dm_tree_free (*tree);
      *tree = NULL;
      return 0;
    }

  *node = dm_tree_find_node (*tree, maj, min);
  if (! *node)
    {
      grub_dprintf ("hostdisk", "dm_tree_find_node failed\n");
      dm_tree_free (*tree);
      *tree = NULL;
      return 0;
    }
  return 1;
}

#endif

static char *
get_dm_uuid (const char *os_dev)
{
  if ((strncmp ("/dev/mapper/", os_dev, 12) != 0))
    return NULL;
  
#ifdef HAVE_DEVICE_MAPPER
  {
    struct dm_tree *tree;
    struct dm_tree_node *node;
    const char *node_uuid;
    char *ret;

    if (!grub_util_open_dm (os_dev, &tree, &node))
      return NULL;

    node_uuid = dm_tree_node_get_uuid (node);
    if (! node_uuid)
      {
	grub_dprintf ("hostdisk", "%s has no DM uuid\n", os_dev);
	dm_tree_free (tree);
	return NULL;
      }

    ret = grub_strdup (node_uuid);

    dm_tree_free (tree);

    return ret;
  }
#endif

  return NULL;
}

static enum grub_dev_abstraction_types
grub_util_get_dm_abstraction (const char *os_dev)
{
#ifdef HAVE_DEVICE_MAPPER
  char *uuid;

  uuid = get_dm_uuid (os_dev);

  if (uuid == NULL)
    return GRUB_DEV_ABSTRACTION_NONE;

  if (strncmp (uuid, "LVM-", 4) == 0)
    {
      grub_free (uuid);
      return GRUB_DEV_ABSTRACTION_LVM;
    }
  if (strncmp (uuid, "CRYPT-LUKS1-", 4) == 0)
    {
      grub_free (uuid);
      return GRUB_DEV_ABSTRACTION_LUKS;
    }

  grub_free (uuid);
  return GRUB_DEV_ABSTRACTION_NONE;
#else
  if ((strncmp ("/dev/mapper/", os_dev, 12) != 0))
    return GRUB_DEV_ABSTRACTION_NONE;
  return GRUB_DEV_ABSTRACTION_LVM;  
#endif
}

#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <libgeom.h>

/* FIXME: geom actually gives us the whole container hierarchy.
   It can be used more efficiently than this.  */
void
grub_util_follow_gpart_up (const char *name, grub_disk_addr_t *off_out, char **name_out)
{
  struct gmesh mesh;
  struct gclass *class;
  int error;
  struct ggeom *geom;

  grub_util_info ("following geom '%s'", name);

  error = geom_gettree (&mesh);
  if (error != 0)
    grub_util_error ("couldn't open geom");

  LIST_FOREACH (class, &mesh.lg_class, lg_class)
    if (strcasecmp (class->lg_name, "part") == 0)
      break;
  if (!class)
    grub_util_error ("couldn't open geom part");

  LIST_FOREACH (geom, &class->lg_geom, lg_geom)
    { 
      struct gprovider *provider;
      LIST_FOREACH (provider, &geom->lg_provider, lg_provider)
	if (strcmp (provider->lg_name, name) == 0)
	  {
	    char *name_tmp = xstrdup (geom->lg_name);
	    grub_disk_addr_t off = 0;
	    struct gconfig *config;
	    grub_util_info ("geom '%s' has parent '%s'", name, geom->lg_name);

	    grub_util_follow_gpart_up (name_tmp, &off, name_out);
	    free (name_tmp);
	    LIST_FOREACH (config, &provider->lg_config, lg_config)
	      if (strcasecmp (config->lg_name, "start") == 0)
		off += strtoull (config->lg_val, 0, 10);
	    if (off_out)
	      *off_out = off;
	    return;
	  }
    }
  grub_util_info ("geom '%s' has no parent", name);
  if (name_out)
    *name_out = xstrdup (name);
  if (off_out)
    *off_out = 0;
}

static const char *
grub_util_get_geom_abstraction (const char *dev)
{
  char *whole;
  struct gmesh mesh;
  struct gclass *class;
  const char *name;
  int error;

  if (strncmp (dev, "/dev/", sizeof ("/dev/") - 1) != 0)
    return 0;
  name = dev + sizeof ("/dev/") - 1;
  grub_util_follow_gpart_up (name, NULL, &whole);

  grub_util_info ("following geom '%s'", name);

  error = geom_gettree (&mesh);
  if (error != 0)
    grub_util_error ("couldn't open geom");

  LIST_FOREACH (class, &mesh.lg_class, lg_class)
    {
      struct ggeom *geom;
      LIST_FOREACH (geom, &class->lg_geom, lg_geom)
	{ 
	  struct gprovider *provider;
	  LIST_FOREACH (provider, &geom->lg_provider, lg_provider)
	    if (strcmp (provider->lg_name, name) == 0)
	      return class->lg_name;
	}
    }
  return NULL;
}
#endif

int
grub_util_get_dev_abstraction (const char *os_dev __attribute__((unused)))
{
#ifdef __linux__
  enum grub_dev_abstraction_types ret;

  /* User explicitly claims that this drive is visible by BIOS.  */
  if (grub_util_biosdisk_is_present (os_dev))
    return GRUB_DEV_ABSTRACTION_NONE;

  /* Check for LVM and LUKS.  */
  ret = grub_util_get_dm_abstraction (os_dev);

  if (ret != GRUB_DEV_ABSTRACTION_NONE)
    return ret;

  /* Check for RAID.  */
  if (!strncmp (os_dev, "/dev/md", 7) && ! grub_util_device_is_mapped (os_dev))
    return GRUB_DEV_ABSTRACTION_RAID;
#endif

#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
  const char *abs;
  abs = grub_util_get_geom_abstraction (os_dev);
  grub_util_info ("abstraction of %s is %s", os_dev, abs);
  if (abs && grub_strcasecmp (abs, "eli") == 0)
    return GRUB_DEV_ABSTRACTION_GELI;
#endif

  /* No abstraction found.  */
  return GRUB_DEV_ABSTRACTION_NONE;
}

#ifdef __linux__
static char *
get_mdadm_uuid (const char *os_dev)
{
  int mdadm_pipe[2];
  pid_t mdadm_pid;
  char *name = NULL;

  if (pipe (mdadm_pipe) < 0)
    {
      grub_util_warn ("Unable to create pipe for mdadm: %s", strerror (errno));
      return NULL;
    }

  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_warn ("Unable to fork mdadm: %s", strerror (errno));
  else if (mdadm_pid == 0)
    {
      /* Child.  */
      char *argv[5];

      close (mdadm_pipe[0]);
      dup2 (mdadm_pipe[1], STDOUT_FILENO);
      close (mdadm_pipe[1]);

      /* execvp has inconvenient types, hence the casts.  None of these
         strings will actually be modified.  */
      argv[0] = (char *) "mdadm";
      argv[1] = (char *) "--detail";
      argv[2] = (char *) "--export";
      argv[3] = (char *) os_dev;
      argv[4] = NULL;
      execvp ("mdadm", argv);
      exit (127);
    }
  else
    {
      /* Parent.  Read mdadm's output.  */
      FILE *mdadm;
      char *buf = NULL;
      size_t len = 0;

      close (mdadm_pipe[1]);
      mdadm = fdopen (mdadm_pipe[0], "r");
      if (! mdadm)
	{
	  grub_util_warn ("Unable to open stream from mdadm: %s",
			  strerror (errno));
	  goto out;
	}

      while (getline (&buf, &len, mdadm) > 0)
	{
	  if (strncmp (buf, "MD_UUID=", sizeof ("MD_UUID=") - 1) == 0)
	    {
	      char *name_start, *ptri, *ptro;
	      size_t name_len;

	      free (name);
	      name_start = buf + sizeof ("MD_UUID=") - 1;
	      ptro = name = xmalloc (strlen (name_start) + 1);
	      for (ptri = name_start; *ptri && *ptri != '\n' && *ptri != '\r';
		   ptri++)
		if ((*ptri >= '0' && *ptri <= '9')
		    || (*ptri >= 'a' && *ptri <= 'f')
		    || (*ptri >= 'A' && *ptri <= 'F'))
		  *ptro++ = *ptri;
	      *ptro = 0;
	    }
	}

out:
      close (mdadm_pipe[0]);
      waitpid (mdadm_pid, NULL, 0);
    }

  return name;
}
#endif /* __linux__ */

void
grub_util_pull_device (const char *os_dev)
{
  int ab;
  ab = grub_util_get_dev_abstraction (os_dev);
  switch (ab)
    {
    case GRUB_DEV_ABSTRACTION_GELI:
#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
      {
	char *whole;
	struct gmesh mesh;
	struct gclass *class;
	const char *name;
	int error;
	char *lastsubdev = NULL;

	if (strncmp (os_dev, "/dev/", sizeof ("/dev/") - 1) != 0)
	  return;
	name = os_dev + sizeof ("/dev/") - 1;
	grub_util_follow_gpart_up (name, NULL, &whole);

	grub_util_info ("following geom '%s'", name);

	error = geom_gettree (&mesh);
	if (error != 0)
	  grub_util_error ("couldn't open geom");

	LIST_FOREACH (class, &mesh.lg_class, lg_class)
	  {
	    struct ggeom *geom;
	    LIST_FOREACH (geom, &class->lg_geom, lg_geom)
	      { 
		struct gprovider *provider;
		LIST_FOREACH (provider, &geom->lg_provider, lg_provider)
		  if (strcmp (provider->lg_name, name) == 0)
		    {
		      struct gconsumer *consumer;
		      char *fname;
		      char *uuid;

		      LIST_FOREACH (consumer, &geom->lg_consumer, lg_consumer)
			break;
		      if (!consumer)
			grub_util_error ("couldn't find geli consumer");
		      fname = xasprintf ("/dev/%s", consumer->lg_provider->lg_name);
		      grub_util_info ("consumer %s", consumer->lg_provider->lg_name);
		      lastsubdev = consumer->lg_provider->lg_name;
		      grub_util_pull_device (fname);
		      free (fname);
		    }
	      }
	  }
	if (ab == GRUB_DEV_ABSTRACTION_GELI && lastsubdev)
	  {
	    char *fname = xasprintf ("/dev/%s", lastsubdev);
	    char *grdev = grub_util_get_grub_dev (fname);
	    free (fname);

	    if (grdev)
	      {
		grub_err_t err;
		err = grub_cryptodisk_cheat_mount (grdev, os_dev);
		if (err)
		  grub_util_error ("Can't mount crypto: %s", grub_errmsg);
	      }

	    grub_free (grdev);
	  }
      }
#endif
      break;

    case GRUB_DEV_ABSTRACTION_LVM:
    case GRUB_DEV_ABSTRACTION_LUKS:
#ifdef HAVE_DEVICE_MAPPER
      {
	struct dm_tree *tree;
	struct dm_tree_node *node;
	struct dm_tree_node *child;
	void *handle = NULL;
	char *lastsubdev = NULL;

	if (!grub_util_open_dm (os_dev, &tree, &node))
	  return;

	while ((child = dm_tree_next_child (&handle, node, 0)))
	  {
	    const struct dm_info *dm = dm_tree_node_get_info (child);
	    char *subdev;
	    if (!dm)
	      continue;
	    subdev = grub_find_device ("/dev", makedev (dm->major, dm->minor));
	    if (subdev)
	      {
		lastsubdev = subdev;
		grub_util_pull_device (subdev);
	      }
	  }
	if (ab == GRUB_DEV_ABSTRACTION_LUKS && lastsubdev)
	  {
	    char *grdev = grub_util_get_grub_dev (lastsubdev);
	    dm_tree_free (tree);
	    if (grdev)
	      {
		grub_err_t err;
		err = grub_cryptodisk_cheat_mount (grdev, os_dev);
		if (err)
		  grub_util_error ("Can't mount crypto: %s", grub_errmsg);
	      }
	    grub_free (grdev);
	  }
	else
	  dm_tree_free (tree);
      }
#endif
      return;
    case GRUB_DEV_ABSTRACTION_RAID:
#ifdef __linux__
      {
	char **devicelist = grub_util_raid_getmembers (os_dev, 0);
	int i;
	for (i = 0; devicelist[i];i++)
	  grub_util_pull_device (devicelist[i]);
	free (devicelist);
      }
#endif
      return;

    default:  /* GRUB_DEV_ABSTRACTION_NONE */
      grub_util_biosdisk_get_grub_dev (os_dev);
      return;
    }
}

char *
grub_util_get_grub_dev (const char *os_dev)
{
  char *grub_dev = NULL;

  grub_util_pull_device (os_dev);

  switch (grub_util_get_dev_abstraction (os_dev))
    {
    case GRUB_DEV_ABSTRACTION_LVM:

      {
	unsigned short i, len;
	grub_size_t offset = sizeof ("/dev/mapper/") - 1;

	len = strlen (os_dev) - offset + 1;
	grub_dev = xmalloc (len + sizeof ("lvm/"));

	grub_memcpy (grub_dev, "lvm/", sizeof ("lvm/") - 1);
	grub_memcpy (grub_dev + sizeof ("lvm/") - 1, os_dev + offset, len);
      }

      break;

    case GRUB_DEV_ABSTRACTION_LUKS:
      {
	char *uuid, *dash;
	uuid = get_dm_uuid (os_dev);
	if (!uuid)
	  break;
	dash = grub_strchr (uuid + sizeof ("CRYPT-LUKS1-") - 1, '-');
	if (dash)
	  *dash = 0;
	grub_dev = grub_xasprintf ("cryptouuid/%s",
				   uuid + sizeof ("CRYPT-LUKS1-") - 1);
	grub_free (uuid);
      }
      break;

    case GRUB_DEV_ABSTRACTION_GELI:
#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
      {
	char *whole;
	struct gmesh mesh;
	struct gclass *class;
	const char *name;
	int error;

	if (strncmp (os_dev, "/dev/", sizeof ("/dev/") - 1) != 0)
	  return 0;
	name = os_dev + sizeof ("/dev/") - 1;
	grub_util_follow_gpart_up (name, NULL, &whole);

	grub_util_info ("following geom '%s'", name);

	error = geom_gettree (&mesh);
	if (error != 0)
	  grub_util_error ("couldn't open geom");

	LIST_FOREACH (class, &mesh.lg_class, lg_class)
	  {
	    struct ggeom *geom;
	    LIST_FOREACH (geom, &class->lg_geom, lg_geom)
	      { 
		struct gprovider *provider;
		LIST_FOREACH (provider, &geom->lg_provider, lg_provider)
		  if (strcmp (provider->lg_name, name) == 0)
		    {
		      struct gconsumer *consumer;
		      char *fname;
		      char *uuid;

		      LIST_FOREACH (consumer, &geom->lg_consumer, lg_consumer)
			break;
		      if (!consumer)
			grub_util_error ("couldn't find geli consumer");
		      fname = xasprintf ("/dev/%s", consumer->lg_provider->lg_name);
		      uuid = grub_util_get_geli_uuid (fname);
		      if (!uuid)
			grub_util_error ("couldn't retrieve geli UUID");
		      grub_dev = xasprintf ("cryptouuid/%s", uuid);
		      free (fname);
		      free (uuid);
		    }
	      }
	  }
      }
#endif
      break;

    case GRUB_DEV_ABSTRACTION_RAID:

      if (os_dev[7] == '_' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md_dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md_d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md/dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md/d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] >= '0' && os_dev[7] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] >= '0' && os_dev[8] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md/") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/')
	{
	  /* mdraid 1.x with a free name.  */
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md/") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md/%s", p);
	  free (p);
	}
      else
	grub_util_error ("unknown kind of RAID device `%s'", os_dev);

#ifdef __linux__
      {
	char *mdadm_name = get_mdadm_uuid (os_dev);
	struct stat st;

	if (mdadm_name)
	  {
	    const char *q;

	    for (q = os_dev + strlen (os_dev) - 1; q >= os_dev
		   && grub_isdigit (*q); q--);

	    if (q >= os_dev && *q == 'p')
	      {
		free (grub_dev);
		grub_dev = xasprintf ("mduuid/%s,%s", mdadm_name, q + 1);
		goto done;
	      }
	    free (grub_dev);
	    grub_dev = xasprintf ("mduuid/%s", mdadm_name);

	  done:
	    free (mdadm_name);
	  }
      }
#endif /* __linux__ */

      break;

    default:  /* GRUB_DEV_ABSTRACTION_NONE */
      grub_dev = grub_util_biosdisk_get_grub_dev (os_dev);
    }

  return grub_dev;
}

const char *
grub_util_check_block_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("cannot stat `%s'", blk_dev);

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
    grub_util_error ("cannot stat `%s'", blk_dev);

  if (S_ISCHR (st.st_mode))
    return (blk_dev);
  else
    return 0;
}
