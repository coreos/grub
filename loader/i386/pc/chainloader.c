/* chainloader.c - boot another boot loader */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/loader.h>
#include <pupa/machine/loader.h>
#include <pupa/file.h>
#include <pupa/err.h>
#include <pupa/device.h>
#include <pupa/disk.h>
#include <pupa/misc.h>
#include <pupa/types.h>
#include <pupa/machine/init.h>
#include <pupa/machine/partition.h>
#include <pupa/machine/memory.h>
#include <pupa/rescue.h>
#include <pupa/dl.h>

/* Allocate space statically, because this is very small anyway.  */
static char pupa_chainloader_boot_sector[PUPA_DISK_SECTOR_SIZE];

static pupa_err_t
pupa_chainloader_boot (void)
{
  pupa_device_t dev;
  int drive = -1;
  void *part_addr = 0;
  
  /* Open the root device.  */
  dev = pupa_device_open (0);
  if (dev)
    {
      pupa_disk_t disk = dev->disk;
      
      if (disk)
	{
	  pupa_partition_t p = disk->partition;
	  
	  /* In i386-pc, the id is equal to the BIOS drive number.  */
	  drive = (int) disk->id;

	  if (p)
	    {
	      pupa_disk_read (disk, p->offset, 446, 64,
			      (char *) PUPA_MEMORY_MACHINE_PART_TABLE_ADDR);
	      
	      /* Ignore errors. Perhaps it's not fatal.  */
	      part_addr = (void *) (PUPA_MEMORY_MACHINE_PART_TABLE_ADDR
				    + (p->index << 4));
	    }
	}

      pupa_device_close (dev);
    }

  pupa_chainloader_real_boot (drive, part_addr);

  /* Never reach here.  */
  return PUPA_ERR_NONE;
}

static void
pupa_rescue_cmd_chainloader (int argc, char *argv[])
{
  pupa_file_t file;
  pupa_uint16_t signature;
  int force = 0;

  if (argc > 0 && pupa_strcmp (argv[0], "--force") == 0)
    {
      force = 1;
      argc--;
      argv++;
    }

  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "no file specified");
      return;
    }

  file = pupa_file_open (argv[0]);
  if (! file)
    return;

  /* Read the first block.  */
  if (pupa_file_read (file, pupa_chainloader_boot_sector,
		      PUPA_DISK_SECTOR_SIZE) != PUPA_DISK_SECTOR_SIZE)
    {
      if (pupa_errno == PUPA_ERR_NONE)
	pupa_error (PUPA_ERR_BAD_OS, "too small");

      pupa_file_close (file);
      return;
    }

  /* Check the signature.  */
  signature = *((pupa_uint16_t *) (pupa_chainloader_boot_sector
				   + PUPA_DISK_SECTOR_SIZE - 2));
  if (signature != pupa_le_to_cpu16 (0xaa55) && ! force)
    pupa_error (PUPA_ERR_BAD_OS, "invalid signature");

  pupa_file_close (file);

  if (pupa_errno == PUPA_ERR_NONE)
    pupa_loader_set (0, pupa_chainloader_boot, 0);
}

static const char loader_name[] = "chainloader";

PUPA_MOD_INIT
{
  pupa_rescue_register_command (loader_name,
				pupa_rescue_cmd_chainloader,
				"load another boot loader");
}

PUPA_MOD_FINI
{
  pupa_rescue_unregister_command (loader_name);
}
