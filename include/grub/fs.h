/* fs.h - filesystem manager */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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

#ifndef PUPA_FS_HEADER
#define PUPA_FS_HEADER	1

#include <pupa/device.h>
#include <pupa/symbol.h>
#include <pupa/types.h>

/* Forward declaration is required, because of mutual reference.  */
struct pupa_file;

/* Filesystem descriptor.  */
struct pupa_fs
{
  /* My name.  */
  const char *name;

  /* Call HOOK with each file under DIR.  */
  pupa_err_t (*dir) (pupa_device_t device, const char *path,
		     int (*hook) (const char *filename, int dir));
  
  /* Open a file named NAME and initialize FILE.  */
  pupa_err_t (*open) (struct pupa_file *file, const char *name);
  
  /* Read LEN bytes data from FILE into BUF.  */
  pupa_ssize_t (*read) (struct pupa_file *file, char *buf, pupa_ssize_t len);
  
  /* Close the file FILE.  */
  pupa_err_t (*close) (struct pupa_file *file);
  
  /* Return the label of the device DEVICE in LABEL.  The label is
     returned in a pupa_malloc'ed buffer and should be freed by the
     caller.  */
  pupa_err_t (*label) (pupa_device_t device, char **label);

  /* The next filesystem.  */
  struct pupa_fs *next;
};
typedef struct pupa_fs *pupa_fs_t;

/* This is special, because block lists are not files in usual sense.  */
extern struct pupa_fs pupa_fs_blocklist;

void EXPORT_FUNC(pupa_fs_register) (pupa_fs_t fs);
void EXPORT_FUNC(pupa_fs_unregister) (pupa_fs_t fs);
void EXPORT_FUNC(pupa_fs_iterate) (int (*hook) (const pupa_fs_t fs));
pupa_fs_t EXPORT_FUNC(pupa_fs_probe) (pupa_device_t device);

#ifdef PUPA_UTIL
void pupa_fat_init (void);
void pupa_fat_fini (void);
void pupa_ext2_init (void);
void pupa_ext2_fini (void);
#endif /* PUPA_UTIL */

#endif /* ! PUPA_FS_HEADER */
