/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef PUPA_FILE_HEADER
#define PUPA_FILE_HEADER	1

#include <pupa/types.h>
#include <pupa/err.h>
#include <pupa/device.h>
#include <pupa/fs.h>

/* File description.  */
struct pupa_file
{
  /* The underlying device.  */
  pupa_device_t device;

  /* The underlying filesystem.  */
  pupa_fs_t fs;

  /* The current offset.  */
  pupa_ssize_t offset;

  /* The file size.  */
  pupa_ssize_t size;

  /* Filesystem-specific data.  */
  void *data;

  /* This is called when a sector is read. Used only for a disk device.  */
  void (*read_hook) (unsigned long sector, unsigned offset, unsigned length);
};
typedef struct pupa_file *pupa_file_t;

/* Get a device name from NAME.  */
char *EXPORT_FUNC(pupa_file_get_device_name) (const char *name);

pupa_file_t EXPORT_FUNC(pupa_file_open) (const char *name);
pupa_ssize_t EXPORT_FUNC(pupa_file_read) (pupa_file_t file, char *buf,
					  pupa_ssize_t len);
pupa_ssize_t EXPORT_FUNC(pupa_file_seek) (pupa_file_t file,
					  pupa_ssize_t offset);
pupa_err_t EXPORT_FUNC(pupa_file_close) (pupa_file_t file);

static inline pupa_ssize_t
pupa_file_size (const pupa_file_t file)
{
  return file->size;
}

static inline pupa_ssize_t
pupa_file_tell (const pupa_file_t file)
{
  return file->offset;
}

#endif /* ! PUPA_FILE_HEADER */
