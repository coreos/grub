/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/machine/biosdisk.h>
#include <pupa/machine/memory.h>
#include <pupa/disk.h>
#include <pupa/mm.h>
#include <pupa/types.h>
#include <pupa/misc.h>
#include <pupa/err.h>
#include <pupa/term.h>

/* Drive Parameters.  */
struct pupa_biosdisk_drp
{
  pupa_uint16_t size;
  pupa_uint16_t flags;
  pupa_uint32_t cylinders;
  pupa_uint32_t heads;
  pupa_uint32_t sectors;
  pupa_uint64_t total_sectors;
  pupa_uint16_t bytes_per_sector;
  /* ver 2.0 or higher */
  pupa_uint32_t EDD_configuration_parameters;
  /* ver 3.0 or higher */
  pupa_uint16_t signature_dpi;
  pupa_uint8_t length_dpi;
  pupa_uint8_t reserved[3];
  pupa_uint8_t name_of_host_bus[4];
  pupa_uint8_t name_of_interface_type[8];
  pupa_uint8_t interface_path[8];
  pupa_uint8_t device_path[8];
  pupa_uint8_t reserved2;
  pupa_uint8_t checksum;
  
  /* XXX: This is necessary, because the BIOS of Thinkpad X20
     writes a garbage to the tail of drive parameters,
     regardless of a size specified in a caller.  */
  pupa_uint8_t dummy[16];
} __attribute__ ((packed));

/* Disk Address Packet.  */
struct pupa_biosdisk_dap
{
  pupa_uint8_t length;
  pupa_uint8_t reserved;
  pupa_uint16_t blocks;
  pupa_uint32_t buffer;
  pupa_uint64_t block;
} __attribute__ ((packed));


static int
pupa_biosdisk_get_drive (const char *name)
{
  unsigned long drive;

  if ((name[0] != 'f' && name[0] != 'h') || name[1] != 'd')
    goto fail;
    
  drive = pupa_strtoul (name + 2, 0, 10);
  if (pupa_errno != PUPA_ERR_NONE)
    goto fail;

  if (name[0] == 'h')
    drive += 0x80;
  
  return (int) drive ;

 fail:
  pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "not a biosdisk");
  return -1;
}

static int
pupa_biosdisk_call_hook (int (*hook) (const char *name), int drive)
{
  char name[10];

  pupa_sprintf (name, (drive & 0x80) ? "hd%d" : "fd%d", drive & (~0x80));
  return hook (name);
}

static int
pupa_biosdisk_iterate (int (*hook) (const char *name))
{
  int drive;
  int num_floppies;

  /* For floppy disks, we can get the number safely.  */
  num_floppies = pupa_biosdisk_get_num_floppies ();
  for (drive = 0; drive < num_floppies; drive++)
    if (pupa_biosdisk_call_hook (hook, drive))
      return 1;
  
  /* For hard disks, attempt to read the MBR.  */
  for (drive = 0x80; drive < 0x90; drive++)
    {
      if (pupa_biosdisk_rw_standard (0x02, drive, 0, 0, 1, 1,
				     PUPA_MEMORY_MACHINE_SCRATCH_SEG) != 0)
	break;
      
      if (pupa_biosdisk_call_hook (hook, drive))
	return 1;
    }
  
  return 0;
}

static pupa_err_t
pupa_biosdisk_open (const char *name, pupa_disk_t disk)
{
  unsigned long total_sectors = 0;
  int drive;
  struct pupa_biosdisk_data *data;

  drive = pupa_biosdisk_get_drive (name);
  if (drive < 0)
    return pupa_errno;

  disk->has_partitions = (drive & 0x80);
  disk->id = drive;
  
  data = (struct pupa_biosdisk_data *) pupa_malloc (sizeof (*data));
  if (! data)
    return pupa_errno;
  
  data->drive = drive;
  data->flags = 0;
  
  if (drive & 0x80)
    {
      /* HDD */
      int version;
      
      version = pupa_biosdisk_check_int13_extensions (drive);
      if (version)
	{
	  struct pupa_biosdisk_drp *drp
	    = (struct pupa_biosdisk_drp *) PUPA_MEMORY_MACHINE_SCRATCH_ADDR;

	  /* Clear out the DRP.  */
	  pupa_memset (drp, 0, sizeof (*drp));
	  drp->size = sizeof (*drp);
	  if (pupa_biosdisk_get_diskinfo_int13_extensions (drive, drp))
	    {
	      data->flags = PUPA_BIOSDISK_FLAG_LBA;

	      /* FIXME: 2TB limit.  */
	      if (drp->total_sectors)
		total_sectors = drp->total_sectors & ~0L;
	      else
                /* Some buggy BIOSes doesn't return the total sectors
                   correctly but returns zero. So if it is zero, compute
                   it by C/H/S returned by the LBA BIOS call.  */
                total_sectors = drp->cylinders * drp->heads * drp->sectors;
	    }
	}
    }

  if (pupa_biosdisk_get_diskinfo_standard (drive,
					   &data->cylinders,
					   &data->heads,
					   &data->sectors) != 0)
    {
      pupa_free (data);
      return pupa_error (PUPA_ERR_BAD_DEVICE, "cannot get C/H/S values");
    }

  if (! total_sectors)
    total_sectors = data->cylinders * data->heads * data->sectors;

  disk->total_sectors = total_sectors;
  disk->data = data;
  
  return PUPA_ERR_NONE;
}

static void
pupa_biosdisk_close (pupa_disk_t disk)
{
  pupa_free (disk->data);
}

/* For readability.  */
#define PUPA_BIOSDISK_READ	0
#define PUPA_BIOSDISK_WRITE	1

static pupa_err_t
pupa_biosdisk_rw (int cmd, pupa_disk_t disk,
		  unsigned long sector, unsigned long size,
		  unsigned segment)
{
  struct pupa_biosdisk_data *data = disk->data;
  
  if (data->flags & PUPA_BIOSDISK_FLAG_LBA)
    {
      struct pupa_biosdisk_dap *dap;
      
      dap = (struct pupa_biosdisk_dap *) (PUPA_MEMORY_MACHINE_SCRATCH_ADDR
					  + (data->sectors
					     << PUPA_DISK_SECTOR_BITS));
      dap->length = sizeof (*dap);
      dap->reserved = 0;
      dap->blocks = size;
      dap->buffer = segment << 16;	/* The format SEGMENT:ADDRESS.  */
      dap->block = sector;

      if (pupa_biosdisk_rw_int13_extensions (cmd + 0x42, data->drive, dap))
	{
	  /* Fall back to the CHS mode.  */
	  data->flags &= ~PUPA_BIOSDISK_FLAG_LBA;
	  disk->total_sectors = data->cylinders * data->heads * data->sectors;
	  return pupa_biosdisk_rw (cmd, disk, sector, size, segment);
	}
    }
  else
    {
      unsigned coff, hoff, soff;
      unsigned head;
      
      soff = sector % data->sectors + 1;
      head = sector / data->sectors;
      hoff = head % data->heads;
      coff = head / data->heads;

      if (coff >= data->cylinders)
	return pupa_error (PUPA_ERR_OUT_OF_RANGE, "out of disk");

      if (pupa_biosdisk_rw_standard (cmd + 0x02, data->drive,
				     coff, hoff, soff, size, segment))
	{
	  switch (cmd)
	    {
	    case PUPA_BIOSDISK_READ:
	      return pupa_error (PUPA_ERR_READ_ERROR, "biosdisk read error");
	    case PUPA_BIOSDISK_WRITE:
	      return pupa_error (PUPA_ERR_WRITE_ERROR, "biosdisk write error");
	    }
	}
    }

  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_biosdisk_read (pupa_disk_t disk, unsigned long sector,
		    unsigned long size, char *buf)
{
  struct pupa_biosdisk_data *data = disk->data;

  while (size)
    {
      unsigned long len;

      len = data->sectors - (sector % data->sectors);
      if (len > size)
	len = size;

      if (pupa_biosdisk_rw (PUPA_BIOSDISK_READ, disk, sector, len,
			    PUPA_MEMORY_MACHINE_SCRATCH_SEG))
	return pupa_errno;

      pupa_memcpy (buf, (void *) PUPA_MEMORY_MACHINE_SCRATCH_ADDR,
		   len << PUPA_DISK_SECTOR_BITS);
      buf += len << PUPA_DISK_SECTOR_BITS;
      sector += len;
      size -= len;
    }

  return pupa_errno;
}

static pupa_err_t
pupa_biosdisk_write (pupa_disk_t disk, unsigned long sector,
		     unsigned long size, const char *buf)
{
  struct pupa_biosdisk_data *data = disk->data;

  while (size)
    {
      unsigned long len;

      len = data->sectors - (sector % data->sectors);
      if (len > size)
	len = size;

      pupa_memcpy ((void *) PUPA_MEMORY_MACHINE_SCRATCH_ADDR, buf,
		   len << PUPA_DISK_SECTOR_BITS);

      if (pupa_biosdisk_rw (PUPA_BIOSDISK_WRITE, disk, sector, len,
			    PUPA_MEMORY_MACHINE_SCRATCH_SEG))
	return pupa_errno;

      buf += len << PUPA_DISK_SECTOR_BITS;
      sector += len;
      size -= len;
    }

  return pupa_errno;
}

static struct pupa_disk_dev pupa_biosdisk_dev =
  {
    .name = "biosdisk",
    .iterate = pupa_biosdisk_iterate,
    .open = pupa_biosdisk_open,
    .close = pupa_biosdisk_close,
    .read = pupa_biosdisk_read,
    .write = pupa_biosdisk_write,
    .next = 0
  };

void
pupa_biosdisk_init (void)
{
  pupa_disk_dev_register (&pupa_biosdisk_dev);
}
