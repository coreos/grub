/* bios.c - implement C part of low-level BIOS disk input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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

#include "shared.h"


/* These are defined in asm.S, and never be used elsewhere, so declare the
   prototypes here.  */
extern int biosdisk_int13_extensions (int ah, int drive, void *dap);
extern int biosdisk_standard (int ah, int drive,
			      int coff, int hoff, int soff,
			      int nsec, int segment);
extern int check_int13_extensions (int drive);
extern int get_diskinfo_int13_extensions (int drive, void *drp);
extern int get_diskinfo_standard (int drive,
				  unsigned long *cylinders,
				  unsigned long *heads,
				  unsigned long *sectors);
extern int get_diskinfo_floppy (int drive,
				unsigned long *cylinders,
				unsigned long *heads,
				unsigned long *sectors);


/* Read/write NSEC sectors starting from SECTOR in DRIVE disk with GEOMETRY
   from/into SEGMENT segment. If READ is BIOSDISK_READ, then read it,
   else if READ is BIOSDISK_WRITE, then write it. If an geometry error
   occurs, return BIOSDISK_ERROR_GEOMETRY, and if other error occurs, then
   return the error number. Otherwise, return 0.  */
int
biosdisk (int read, int drive, struct geometry *geometry,
	  int sector, int nsec, int segment)
{
  int err;
  
  if (geometry->flags & BIOSDISK_FLAG_LBA_EXTENSION)
    {
      struct disk_address_packet
      {
	unsigned char length;
	unsigned char reserved;
	unsigned short blocks;
	unsigned long buffer;
	unsigned long long block;
      } dap;
      
      if (sector >= geometry->total_sectors)
	return BIOSDISK_ERROR_GEOMETRY;

      /* FIXME: sizeof (DAP) must be 0x10. Should assert that the compiler
	 can't add any padding.  */
      dap.length = sizeof (dap);
      dap.block = sector;
      dap.blocks = nsec;
      /* This is undocumented part. The address is formated in
	 SEGMENT:ADDRESS.  */
      dap.buffer = segment << 16;
      
      err = biosdisk_int13_extensions (read + 0x42, drive, &dap);

#define NO_INT13_FALLBACK	1
#ifndef NO_INT13_FALLBACK
      if (err)
	{
	  geometry->flags &= ~BIOSDISK_FLAG_LBA_EXTENSION;
	  geometry->total_sectors = (geometry->cylinders
				     * geometry->heads
				     * geometry->sectors);
	  return biosdisk (read, drive, geometry, sector, nsec, segment);
	}
#endif /* ! NO_INT13_FALLBACK */
      
    }
  else
    {
      int cylinder_offset, head_offset, sector_offset;
      int head;

      /* SECTOR_OFFSET is counted from one, while HEAD_OFFSET and
	 CYLINDER_OFFSET are counted from zero.  */
      sector_offset = sector % geometry->sectors + 1;
      head = sector / geometry->sectors;
      head_offset = head % geometry->heads;
      cylinder_offset = head / geometry->heads;
      
      if (cylinder_offset >= geometry->cylinders)
	return BIOSDISK_ERROR_GEOMETRY;

      err = biosdisk_standard (read + 0x02, drive,
			       cylinder_offset, head_offset, sector_offset,
			       nsec, segment);
    }

  return err;
}

/* Return the geometry of DRIVE in GEOMETRY. If an error occurs, return
   non-zero, otherwise zero.  */
int
get_diskinfo (int drive, struct geometry *geometry)
{
  int err;
  
  /* Clear GEOMETRY.  */
  memset (geometry, 0, sizeof (struct geometry));
  
  if (drive & 0x80)
    {
      /* hard disk */
      int version;
      unsigned long total_sectors = 0;
      
      version = check_int13_extensions (drive);
      if (version)
	{
	  struct drive_parameters
	  {
	    unsigned short size;
	    unsigned short flags;
	    unsigned long cylinders;
	    unsigned long heads;
	    unsigned long sectors;
	    unsigned long long total_sectors;
	    unsigned short bytes_per_sector;
	    /* ver 2.0 or higher */
	    unsigned long EDD_configuration_parameters;
	    /* ver 3.0 or higher */
	    unsigned short signature_dpi;
	    unsigned char length_dpi;
	    unsigned char reserved[3];
	    unsigned char name_of_host_bus[4];
	    unsigned char name_of_interface_type[8];
	    unsigned char interface_path[8];
	    unsigned char device_path[8];
	    unsigned char reserved2;
	    unsigned char checksum;
	  } drp;
	  
	  drp.size = sizeof (drp);
	  err = get_diskinfo_int13_extensions (drive, &drp);
	  if (! err)
	    {
	      geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION;
	      
	      /* Check if CHS information is valid.  */
	      if (drp.flags & 0x02)
		{
		  total_sectors = (geometry->cylinders
				   * geometry->heads
				   * geometry->sectors);
		}
	    }
	}

      /* Don't pass GEOMETRY directly, but pass each element instead,
	 so that we can change the structure easily.  */
      err = get_diskinfo_standard (drive,
				   &geometry->cylinders,
				   &geometry->heads,
				   &geometry->sectors);
      if (err)
	return err;

      if (! total_sectors)
	{
	  total_sectors = (geometry->cylinders
			   * geometry->heads
			   * geometry->sectors);
	}
      geometry->total_sectors = total_sectors;
    }
  else
    {
      /* floppy disk */
      err = get_diskinfo_floppy (drive,
				 &geometry->cylinders,
				 &geometry->heads,
				 &geometry->sectors);
      if (err)
	return err;

      geometry->total_sectors = (geometry->cylinders
				 * geometry->heads
				 * geometry->sectors);
    }

  return 0;
}
