/* device.c - device manager */
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

#include <pupa/device.h>
#include <pupa/disk.h>
#include <pupa/net.h>
#include <pupa/fs.h>
#include <pupa/mm.h>
#include <pupa/misc.h>

static char *pupa_device_root;

pupa_err_t
pupa_device_set_root (const char *name)
{
  pupa_free (pupa_device_root);
  pupa_device_root = pupa_strdup (name);
  return pupa_errno;
}

const char *
pupa_device_get_root (void)
{
  if (! pupa_device_root)
    pupa_error (PUPA_ERR_BAD_DEVICE, "no root device");
  
  return pupa_device_root;
}

pupa_device_t
pupa_device_open (const char *name)
{
  pupa_disk_t disk = 0;
  pupa_device_t dev = 0;

  if (! name)
    {
      if (! pupa_device_root)
	{
	  pupa_error (PUPA_ERR_BAD_DEVICE, "no device is set");
	  goto fail;
	}

      name = pupa_device_root;
    }
    
  dev = pupa_malloc (sizeof (*dev));
  if (! dev)
    goto fail;
  
  /* Try to open a disk.  */
  disk = pupa_disk_open (name);
  if (! disk)
    {
      pupa_error (PUPA_ERR_BAD_DEVICE, "unknown device");
      goto fail;
    }

  dev->disk = disk;
  dev->net = 0;	/* FIXME */

  return dev;

 fail:
  if (disk)
    pupa_disk_close (disk);
  
  pupa_free (dev);

  return 0;
}

pupa_err_t
pupa_device_close (pupa_device_t device)
{
  if (device->disk)
    pupa_disk_close (device->disk);

  pupa_free (device);

  return pupa_errno;
}
