/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002, 2003  Free Software Foundation, Inc.
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

#ifndef PUPA_DISK_HEADER
#define PUPA_DISK_HEADER	1

#include <pupa/symbol.h>
#include <pupa/err.h>
#include <pupa/types.h>
#include <pupa/device.h>

struct pupa_disk;

/* Disk device.  */
struct pupa_disk_dev
{
  /* The device name.  */
  const char *name;

  /* Call HOOK with each device name, until HOOK returns non-zero.  */
  int (*iterate) (int (*hook) (const char *name));

  /* Open the device named NAME, and set up DISK.  */
  pupa_err_t (*open) (const char *name, struct pupa_disk *disk);

  /* Close the disk DISK.  */
  void (*close) (struct pupa_disk *disk);

  /* Read SIZE sectors from the sector SECTOR of the disk DISK into BUF.  */
  pupa_err_t (*read) (struct pupa_disk *disk, unsigned long sector,
		      unsigned long size, char *buf);

  /* Write SIZE sectors from BUF into the sector SECTOR of the disk DISK.  */
  pupa_err_t (*write) (struct pupa_disk *disk, unsigned long sector,
		       unsigned long size, const char *buf);

  /* The next disk device.  */
  struct pupa_disk_dev *next;
};
typedef struct pupa_disk_dev *pupa_disk_dev_t;

struct pupa_partition;

/* Disk.  */
struct pupa_disk
{
  /* The disk name.  */
  const char *name;

  /* The underlying disk device.  */
  pupa_disk_dev_t dev;

  /* The total number of sectors.  */
  unsigned long total_sectors;

  /* If partitions can be stored.  */
  int has_partitions;

  /* The id used by the disk cache manager.  */
  unsigned long id;
  
  /* The partition information. This is machine-specific.  */
  struct pupa_partition *partition;

  /* Called when a sector was read.  */
  void (*read_hook) (unsigned long sector, unsigned offset, unsigned length);

  /* Device-specific data.  */
  void *data;
};
typedef struct pupa_disk *pupa_disk_t;

/* The sector size.  */
#define PUPA_DISK_SECTOR_SIZE	0x200
#define PUPA_DISK_SECTOR_BITS	9

/* The maximum number of disk caches.  */
#define PUPA_DISK_CACHE_NUM	1021

/* The size of a disk cache in sector units.  */
#define PUPA_DISK_CACHE_SIZE	8
#define PUPA_DISK_CACHE_BITS	3

/* This is called from the memory manager.  */
void pupa_disk_cache_invalidate_all (void);

void EXPORT_FUNC(pupa_disk_dev_register) (pupa_disk_dev_t dev);
void EXPORT_FUNC(pupa_disk_dev_unregister) (pupa_disk_dev_t dev);
void EXPORT_FUNC(pupa_disk_dev_iterate) (int (*hook) (const char *name));

pupa_disk_t EXPORT_FUNC(pupa_disk_open) (const char *name);
void EXPORT_FUNC(pupa_disk_close) (pupa_disk_t disk);
pupa_err_t EXPORT_FUNC(pupa_disk_read) (pupa_disk_t disk,
					unsigned long sector,
					unsigned long offset,
					unsigned long size,
					char *buf);
pupa_err_t EXPORT_FUNC(pupa_disk_write) (pupa_disk_t disk,
					 unsigned long sector,
					 unsigned long offset,
					 unsigned long size,
					 const char *buf);

pupa_err_t EXPORT_FUNC(pupa_print_partinfo) (pupa_device_t disk,
					     char *partname);

#endif /* ! PUPA_DISK_HEADER */
