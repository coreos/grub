/* blocklist.c - print the block list of a file */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/disk.h>

static grub_err_t
grub_cmd_blocklist (struct grub_arg_list *state __attribute__ ((unused)),
		    int argc, char **args)
{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  unsigned long start_sector = 0;
  unsigned num_sectors = 0;
  int num_entries = 0;
  auto void read_blocklist (unsigned long sector, unsigned offset,
			    unsigned length);
  auto void print_blocklist (unsigned long sector, unsigned num,
			     unsigned offset, unsigned length);
  
  void read_blocklist (unsigned long sector, unsigned offset,
		       unsigned length)
    {
      if (num_sectors > 0)
	{
	  if (start_sector + num_sectors == sector
	      && offset == 0 && length == GRUB_DISK_SECTOR_SIZE)
	    {
	      num_sectors++;
	      return;
	    }
	  
	  print_blocklist (start_sector, num_sectors, 0, 0);
	  num_sectors = 0;
	}

      if (offset == 0 && length == GRUB_DISK_SECTOR_SIZE)
	{
	  start_sector = sector;
	  num_sectors++;
	}
      else
	print_blocklist (sector, 0, offset, length);
    }
  
  void print_blocklist (unsigned long sector, unsigned num,
			unsigned offset, unsigned length)
    {
      if (num_entries++)
	grub_printf (",");

      grub_printf ("%lu", sector);
      if (num > 0)
	grub_printf ("+%u", num);
      if (offset != 0 || length != 0)
	grub_printf ("[%u-%u]", offset, offset + length);
    }
  
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");

  file = grub_file_open (args[0]);
  if (! file)
    return grub_errno;

  file->read_hook = read_blocklist;

  while (grub_file_read (file, buf, sizeof (buf)) > 0)
    ;

  if (num_sectors > 0)
    print_blocklist (start_sector, num_sectors, 0, 0);
  
  grub_file_close (file);

  return grub_errno;
}


GRUB_MOD_INIT(blocklist)
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("blocklist", grub_cmd_blocklist,
			 GRUB_COMMAND_FLAG_BOTH,
			 "blocklist FILE",
			 "Print a block list.", 0);
}

GRUB_MOD_FINI(blocklist)
{
  grub_unregister_command ("blocklist");
}
