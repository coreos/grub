/* device.h - device manager */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
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

#ifndef PUPA_DEVICE_HEADER
#define PUPA_DEVICE_HEADER	1

#include <pupa/symbol.h>
#include <pupa/err.h>

struct pupa_disk;
struct pupa_net;
struct pupa_fs;

struct pupa_device
{
  struct pupa_disk *disk;
  struct pupa_net *net;
};
typedef struct pupa_device *pupa_device_t;

pupa_device_t EXPORT_FUNC(pupa_device_open) (const char *name);
pupa_err_t EXPORT_FUNC(pupa_device_close) (pupa_device_t device);

pupa_err_t EXPORT_FUNC(pupa_device_set_root) (const char *name);
const char *EXPORT_FUNC(pupa_device_get_root) (void);

#endif /* ! PUPA_DEVICE_HEADER */
