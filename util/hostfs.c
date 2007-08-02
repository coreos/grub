/* hostfs.c - Dummy filesystem to provide access to the hosts filesystem  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <grub/fs.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/misc.h>

#include <dirent.h>
#include <stdio.h>

static grub_err_t
grub_hostfs_dir (grub_device_t device, const char *path, 
		 int (*hook) (const char *filename, int dir))
{
  DIR *dir;

  /* Check if the disk is our dummy disk.  */
  if (grub_strcmp (device->disk->name, "host"))
    return grub_error (GRUB_ERR_BAD_FS, "not a hostfs");

  dir = opendir (path);
  if (! dir)
    return grub_error (GRUB_ERR_BAD_FILENAME,
		       "can't open the hostfs directory `%s'", path);

  while (1)
    {
      struct dirent *de;

      de = readdir (dir);
      if (! de)
	break;

      hook (de->d_name, de->d_type == DT_DIR);
    }

  closedir (dir);

  return GRUB_ERR_NONE;
}

/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_hostfs_open (struct grub_file *file, const char *name)
{
  FILE *f;

  f = fopen (name, "r");
  if (! f)
    return grub_error (GRUB_ERR_BAD_FILENAME,
		       "can't open `%s'", name);
  file->data = f;

  fseek (f, 0, SEEK_END);
  file->size = ftell (f);
  fseek (f, 0, SEEK_SET);

  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_hostfs_read (grub_file_t file, char *buf, grub_size_t len)
{
  FILE *f;

  f = (FILE *) file->data;
  int s= fread (buf, 1, len, f);

  return s;
}

static grub_err_t
grub_hostfs_close (grub_file_t file)
{
  FILE *f;

  f = (FILE *) file->data;
  fclose (f);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_hostfs_label (grub_device_t device __attribute ((unused)),
		   char **label __attribute ((unused)))
{
  return GRUB_ERR_NONE;
}

static struct grub_fs grub_hostfs_fs =
  {
    .name = "hostfs",
    .dir = grub_hostfs_dir,
    .open = grub_hostfs_open,
    .read = grub_hostfs_read,
    .close = grub_hostfs_close,
    .label = grub_hostfs_label,
    .next = 0
  };



void
grub_hostfs_init (void)
{
  grub_fs_register (&grub_hostfs_fs);
}

void
grub_hostfs_fini (void)
{
  grub_fs_unregister (&grub_hostfs_fs);
}
