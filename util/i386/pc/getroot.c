/* getroot.c - Get root device */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
 *  Copyright (C) 2003  Marco Gerards <metgerards@student.han.nl>
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <pupa/util/misc.h>

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

char *
pupa_get_prefix (const char *dir)
{
  char *saved_cwd;
  char *abs_dir, *prev_dir;
  char *prefix;
  struct stat st, prev_st;
  
  /* Save the current directory.  */
  saved_cwd = xgetcwd ();

  if (chdir (dir) < 0)
    pupa_util_error ("Cannot change directory to `%s'", dir);

  abs_dir = xgetcwd ();
  strip_extra_slashes (abs_dir);
  prev_dir = xstrdup (abs_dir);
  
  if (stat (".", &prev_st) < 0)
    pupa_util_error ("Cannot stat `%s'", dir);

  if (! S_ISDIR (prev_st.st_mode))
    pupa_util_error ("`%s' is not a directory", dir);

  while (1)
    {
      if (chdir ("..") < 0)
	pupa_util_error ("Cannot change directory to the parent");

      if (stat (".", &st) < 0)
	pupa_util_error ("Cannot stat current directory");

      if (! S_ISDIR (st.st_mode))
	pupa_util_error ("Current directory is not a directory???");

      if (prev_st.st_dev != st.st_dev || prev_st.st_ino == st.st_ino)
	break;

      free (prev_dir);
      prev_dir = xgetcwd ();
      prev_st = st;
    }

  strip_extra_slashes (prev_dir);
  prefix = xmalloc (strlen (abs_dir) - strlen (prev_dir) + 2);
  prefix[0] = '/';
  strcpy (prefix + 1, abs_dir + strlen (prev_dir));
  strip_extra_slashes (prefix);

  if (chdir (saved_cwd) < 0)
    pupa_util_error ("Cannot change directory to `%s'", dir);

  free (saved_cwd);
  free (abs_dir);
  free (prev_dir);

  pupa_util_info ("prefix = %s", prefix);
  return prefix;
}

static char *
find_root_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;
  
  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  pupa_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }
  
  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;
      
      if (strcmp (ent->d_name, ".") == 0 || strcmp (ent->d_name, "..") == 0)
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode))
	/* Don't follow symbolic links.  */
	continue;
      
      if (S_ISDIR (st.st_mode))
	{
	  /* Find it recursively.  */
	  char *res;

	  res = find_root_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		pupa_util_error ("Cannot restore the original directory");
	      
	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
	{
	  /* Found!  */
	  char *res;
	  char *cwd;

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 2);
	  sprintf (res, "%s/%s", cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  if (chdir (saved_cwd) < 0)
	    pupa_util_error ("Cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    pupa_util_error ("Cannot restore the original directory");

  free (saved_cwd);
  closedir (dp);
  return 0;
}

char *
pupa_guess_root_device (const char *dir)
{
  struct stat st;
  char *os_dev;
  
  if (stat (dir, &st) < 0)
    pupa_util_error ("Cannot stat `%s'", dir);

  /* This might be truly slow, but is there any better way?  */
  os_dev = find_root_device ("/dev", st.st_dev);
  if (! os_dev)
    return 0;

  return pupa_util_biosdisk_get_pupa_dev (os_dev);
}
