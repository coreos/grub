/* ls.c - command to list files and devices */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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

#include <pupa/types.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/dl.h>
#include <pupa/normal.h>
#include <pupa/arg.h>
#include <pupa/disk.h>
#include <pupa/device.h>
#include <pupa/term.h>
#include <pupa/machine/partition.h>
#include <pupa/file.h>

static const struct pupa_arg_option options[] =
  {
    {"long", 'l', 0, "Show a long list with more detailed information", 0, 0},
    {"human-readable", 'h', 0, "Print sizes in a human readable format", 0, 0},
    {"all", 'a', 0, "List all files", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static const char pupa_human_sizes[] = {' ', 'K', 'M', 'G', 'T'};

static pupa_err_t
pupa_ls_list_disks (int longlist)
{
  auto int pupa_ls_print_disks (const char *name);
  int pupa_ls_print_disks (const char *name)
    {
      pupa_device_t dev;
      auto int print_partition (const pupa_partition_t p);
      
      int print_partition (const pupa_partition_t p)
	{
	  char *pname = pupa_partition_get_name (p);

	  if (pname)
	    {
	      if (longlist)
		pupa_print_partinfo (dev, pname);
	      else
		pupa_printf ("(%s,%s) ", name, pname);
	    }

	  return 0;
	}
      
      dev = pupa_device_open (name);
      pupa_errno = PUPA_ERR_NONE;
      
      if (dev)
	{
	  if (longlist)
	    pupa_printf ("Disk: %s\n", name);
	  else
	    pupa_printf ("(%s) ", name);

	  if (dev->disk && dev->disk->has_partitions)
	    {
	      pupa_partition_iterate (dev->disk, print_partition);
	      pupa_errno = PUPA_ERR_NONE;
	    }

	  pupa_device_close (dev);
	}
  
      return 0;
    }
  
  pupa_disk_dev_iterate (pupa_ls_print_disks);
  pupa_putchar ('\n');
  pupa_refresh ();

 
  return 0;
}

static pupa_err_t
pupa_ls_list_files (const char *dirname, int longlist, int all, int human)
{
  char *device_name;
  pupa_fs_t fs;
  char *path;
  pupa_device_t dev;

  static int print_files (const char *filename, int dir)
    {
      if (all || filename[0] != '.')
	pupa_printf ("%s%s ", filename, dir ? "/" : "");
      
      return 0;
    }
     
  static int print_files_long (const char *filename, int dir)
    {
      char pathname[pupa_strlen (dirname) + pupa_strlen (filename) + 1];

      if ((! all) && (filename[0] == '.'))
	return 0;

      if (! dir)
	{
	  pupa_file_t file;
	  
	  if (dirname[pupa_strlen (dirname) - 1] == '/')
	    pupa_sprintf (pathname, "%s%s", dirname, filename);
	  else
	    pupa_sprintf (pathname, "%s/%s", dirname, filename);

	  /* XXX: For ext2fs symlinks are detected as files while they
	     should be reported as directories.  */
	  file = pupa_file_open (pathname);
	  if (! file)
	    {
	      pupa_errno = 0;
	      return 0;
	    }

	  if (! human)
	    pupa_printf ("%-12d", file->size);
	  else
	    {
	      float fsize = file->size;
	      int fsz = file->size;
	      int units = 0;
	      char buf[20];
	      
	      while (fsz / 1024)
		{
		  fsize /= 1024;
		  fsz /= 1024;
		  units++;
		}

	      if (units)
		{
		  pupa_sprintf (buf, "%0.2f%c", fsize, pupa_human_sizes[units]);
		  pupa_printf ("%-12s", buf);
		}
	      else
		pupa_printf ("%-12d", file->size);
	      
	    }
	  (fs->close) (file);
      	}
      else
	pupa_printf ("%-12s", "DIR");

      pupa_printf ("%s%s\n", filename, dir ? "/" : "");

      return 0;
    }

  device_name = pupa_file_get_device_name (dirname);
  dev = pupa_device_open (device_name);
  if (! dev)
    goto fail;

  fs = pupa_fs_probe (dev);
  path = pupa_strchr (dirname, '/');

  if (! path && ! device_name)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "invalid argument");
      goto fail;
    }
      
  if (! path)
    {
      if (pupa_errno == PUPA_ERR_UNKNOWN_FS)
	pupa_errno = PUPA_ERR_NONE;
	  
      pupa_printf ("(%s): Filesystem is %s.\n",
		   device_name, fs ? fs->name : "unknown");
    }
  else if (fs)
    {
      if (longlist)
	(fs->dir) (dev, path, print_files_long);
      else
	(fs->dir) (dev, path, print_files);
      pupa_putchar ('\n');
      pupa_refresh ();
    }

 fail:
  if (dev)
    pupa_device_close (dev);
      
  pupa_free (device_name);

  return 0;
}

static pupa_err_t
pupa_cmd_ls (struct pupa_arg_list *state, int argc, char **args)
{
  static int pupa_ls_print_files (const char *filename, int dir)
    {
      if (state[2].set/*all*/ || filename[0] != '.')
	pupa_printf ("%s%s ", filename, dir ? "/" : "");
      
      return 0;
    }

  if (argc == 0)
  pupa_ls_list_disks (state[0].set);
  else
    pupa_ls_list_files (args[0], state[0].set, state[2].set,
			state[1].set);

  return 0;
}

#ifdef PUPA_UTIL
void
pupa_ls_init (void)
{
  pupa_register_command ("ls", pupa_cmd_ls, PUPA_COMMAND_FLAG_BOTH,
			 "ls [OPTIONS...] [DIR]",
			 "List devices and files", options);
}

void
pupa_ls_fini (void)
{
  pupa_unregister_command ("ls");
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("ls", pupa_cmd_ls, PUPA_COMMAND_FLAG_BOTH,
			 "ls [OPTIONS...] [DIR]",
			 "List devices and files", options);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("ls");
}
#endif /* ! PUPA_UTIL */
