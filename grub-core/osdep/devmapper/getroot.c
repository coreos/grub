/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010,2011,2012,2013  Free Software Foundation, Inc.
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

#include <grub/emu/getroot.h>
#include <grub/mm.h>

#ifdef HAVE_DEVICE_MAPPER

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

#include <libdevmapper.h>

#include <grub/types.h>
#include <grub/util/misc.h>

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>

static int
grub_util_open_dm (const char *os_dev, struct dm_tree **tree,
		   struct dm_tree_node **node)
{
  uint32_t maj, min;
  struct stat st;

  *node = NULL;
  *tree = NULL;

  if (stat (os_dev, &st) < 0)
    return 0;

  maj = major (st.st_rdev);
  min = minor (st.st_rdev);

  if (!dm_is_dm_major (maj))
    return 0;

  *tree = dm_tree_create ();
  if (! *tree)
    {
      grub_puts_ (N_("Failed to create `device-mapper' tree"));
      grub_dprintf ("hostdisk", "dm_tree_create failed\n");
      return 0;
    }

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

static char *
get_dm_uuid (const char *os_dev)
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

enum grub_dev_abstraction_types
grub_util_get_dm_abstraction (const char *os_dev)
{
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
}

void
grub_util_pull_devmapper (const char *os_dev)
{
  struct dm_tree *tree;
  struct dm_tree_node *node;
  struct dm_tree_node *child;
  void *handle = NULL;
  char *lastsubdev = NULL;
  char *uuid;

  uuid = get_dm_uuid (os_dev);

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
  if (uuid && strncmp (uuid, "CRYPT-LUKS1-", sizeof ("CRYPT-LUKS1-") - 1) == 0
      && lastsubdev)
    {
      char *grdev = grub_util_get_grub_dev (lastsubdev);
      dm_tree_free (tree);
      if (grdev)
	{
	  grub_err_t err;
	  err = grub_cryptodisk_cheat_mount (grdev, os_dev);
	  if (err)
	    grub_util_error (_("can't mount encrypted volume `%s': %s"),
			     lastsubdev, grub_errmsg);
	}
      grub_free (grdev);
    }
  else
    dm_tree_free (tree);
}

char *
grub_util_devmapper_part_to_disk (struct stat *st,
				  int *is_part, const char *path)
{
  struct dm_tree *tree;
  uint32_t maj, min;
  struct dm_tree_node *node = NULL, *child;
  void *handle;
  const char *node_uuid, *mapper_name = NULL, *child_uuid, *child_name;

  tree = dm_tree_create ();
  if (! tree)
    {
      grub_util_info ("dm_tree_create failed");
      goto devmapper_out;
    }

  maj = major (st->st_rdev);
  min = minor (st->st_rdev);
  if (! dm_tree_add_dev (tree, maj, min))
    {
      grub_util_info ("dm_tree_add_dev failed");
      goto devmapper_out;
    }

  node = dm_tree_find_node (tree, maj, min);
  if (! node)
    {
      grub_util_info ("dm_tree_find_node failed");
      goto devmapper_out;
    }
 reiterate:
  node_uuid = dm_tree_node_get_uuid (node);
  if (! node_uuid)
    {
      grub_util_info ("%s has no DM uuid", path);
      goto devmapper_out;
    }
  if (strncmp (node_uuid, "LVM-", 4) == 0)
    {
      grub_util_info ("%s is an LVM", path);
      goto devmapper_out;
    }
  if (strncmp (node_uuid, "mpath-", 6) == 0)
    {
      /* Multipath partitions have partN-mpath-* UUIDs, and are
	 linear mappings so are handled by
	 grub_util_get_dm_node_linear_info.  Multipath disks are not
	 linear mappings and must be handled specially.  */
      grub_util_info ("%s is a multipath disk", path);
      goto devmapper_out;
    }
  if (strncmp (node_uuid, "DMRAID-", 7) != 0)
    {
      int major, minor;
      const char *node_name;
      grub_util_info ("%s is not DM-RAID", path);

      if ((node_name = dm_tree_node_get_name (node))
	  && grub_util_get_dm_node_linear_info (node_name,
						&major, &minor, 0))
	{
	  *is_part = 1;
	  if (tree)
	    dm_tree_free (tree);
	  char *ret = grub_find_device ("/dev",
					(major << 8) | minor);
	  return ret;
	}

      goto devmapper_out;
    }

  handle = NULL;
  /* Counter-intuitively, device-mapper refers to the disk-like
     device containing a DM-RAID partition device as a "child" of
     the partition device.  */
  child = dm_tree_next_child (&handle, node, 0);
  if (! child)
    {
      grub_util_info ("%s has no DM children", path);
      goto devmapper_out;
    }
  child_uuid = dm_tree_node_get_uuid (child);
  if (! child_uuid)
    {
      grub_util_info ("%s child has no DM uuid", path);
      goto devmapper_out;
    }
  else if (strncmp (child_uuid, "DMRAID-", 7) != 0)
    {
      grub_util_info ("%s child is not DM-RAID", path);
      goto devmapper_out;
    }
  child_name = dm_tree_node_get_name (child);
  if (! child_name)
    {
      grub_util_info ("%s child has no DM name", path);
      goto devmapper_out;
    }
  mapper_name = child_name;
  *is_part = 1;
  node = child;
  goto reiterate;

 devmapper_out:
  if (! mapper_name && node)
    {
      /* This is a DM-RAID disk, not a partition.  */
      mapper_name = dm_tree_node_get_name (node);
      if (! mapper_name)
	grub_util_info ("%s has no DM name", path);
    }
  char *ret;
  if (mapper_name)
    ret = xasprintf ("/dev/mapper/%s", mapper_name);
  else
    ret = NULL;

  if (tree)
    dm_tree_free (tree);
  return ret;
}

int
grub_util_device_is_mapped_stat (struct stat *st)
{
#if GRUB_DISK_DEVS_ARE_CHAR
  if (! S_ISCHR (st->st_mode))
#else
  if (! S_ISBLK (st->st_mode))
#endif
    return 0;

  if (!grub_device_mapper_supported ())
    return 0;

  return dm_is_dm_major (major (st->st_rdev));
}

char *
grub_util_get_devmapper_grub_dev (const char *os_dev)
{
  char *uuid, *optr;
  char *grub_dev;

  uuid = get_dm_uuid (os_dev);
  if (!uuid)
    return NULL;
  
  if (strncmp (uuid, "LVM-", sizeof ("LVM-") - 1) == 0)
    {
	unsigned i;
	int dashes[] = { 0, 6, 10, 14, 18, 22, 26, 32, 38, 42, 46, 50, 54, 58};
	grub_dev = xmalloc (grub_strlen (uuid) + 40);
	optr = grub_stpcpy (grub_dev, "lvmid/");
	for (i = 0; i < ARRAY_SIZE (dashes) - 1; i++)
	  {
	    memcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i],
		    dashes[i+1] - dashes[i]);
	    optr += dashes[i+1] - dashes[i];
	    *optr++ = '-';
	  }
	optr = stpcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i]);
	*optr = '\0';
	grub_dev[sizeof("lvmid/xxxxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxxxx") - 1]
	  = '/';
	free (uuid);
	return grub_dev;
      }

  if (strncmp (uuid, "CRYPT-LUKS1-", sizeof ("CRYPT-LUKS1-") - 1) == 0)
    {
      char *dash;
      dash = grub_strchr (uuid + sizeof ("CRYPT-LUKS1-") - 1, '-');
      if (dash)
	*dash = 0;
      grub_dev = grub_xasprintf ("cryptouuid/%s",
				 uuid + sizeof ("CRYPT-LUKS1-") - 1);
      grub_free (uuid);
      return grub_dev;
    }
  return NULL;
}

char *
grub_util_get_vg_uuid (const char *os_dev)
{
  char *uuid, *vgid;
  int dashes[] = { 0, 6, 10, 14, 18, 22, 26, 32};
  unsigned i;
  char *optr;

  uuid = get_dm_uuid (os_dev);
  if (!uuid)
    return NULL;

  vgid = xmalloc (grub_strlen (uuid));
  optr = vgid;
  for (i = 0; i < ARRAY_SIZE (dashes) - 1; i++)
    {
      memcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i],
	      dashes[i+1] - dashes[i]);
      optr += dashes[i+1] - dashes[i];
      *optr++ = '-';
    }
  optr--;
  *optr = '\0';
  return vgid;
}

void
grub_util_devmapper_cleanup (void)
{
  dm_lib_release ();
}

#else
void
grub_util_pull_devmapper (const char *os_dev __attribute__ ((unused)))
{
  return;
}

int
grub_util_device_is_mapped_stat (struct stat *st __attribute__ ((unused)))
{
  return 0;
}

void
grub_util_devmapper_cleanup (void)
{
}

enum grub_dev_abstraction_types
grub_util_get_dm_abstraction (const char *os_dev __attribute__ ((unused)))
{
  return GRUB_DEV_ABSTRACTION_NONE;
}

char *
grub_util_get_vg_uuid (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

char *
grub_util_devmapper_part_to_disk (struct stat *st __attribute__ ((unused)),
				  int *is_part __attribute__ ((unused)),
				  const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

char *
grub_util_get_devmapper_grub_dev (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

#endif
