/* device.c - device manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#include <grub/device.h>
#include <grub/disk.h>
#include <grub/net.h>
#include <grub/fs.h>
#include <grub/mm.h>
#include <grub/misc.h>

static char *grub_device_root;

grub_err_t
grub_device_set_root (const char *name)
{
  grub_free (grub_device_root);
  grub_device_root = grub_strdup (name);
  return grub_errno;
}

const char *
grub_device_get_root (void)
{
  if (! grub_device_root)
    grub_error (GRUB_ERR_BAD_DEVICE, "no root device");
  
  return grub_device_root;
}

grub_device_t
grub_device_open (const char *name)
{
  grub_disk_t disk = 0;
  grub_device_t dev = 0;

  if (! name)
    {
      if (! grub_device_root)
	{
	  grub_error (GRUB_ERR_BAD_DEVICE, "no device is set");
	  goto fail;
	}

      name = grub_device_root;
    }
    
  dev = grub_malloc (sizeof (*dev));
  if (! dev)
    goto fail;
  
  /* Try to open a disk.  */
  disk = grub_disk_open (name);
  if (! disk)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "unknown device");
      goto fail;
    }

  dev->disk = disk;
  dev->net = 0;	/* FIXME */

  return dev;

 fail:
  if (disk)
    grub_disk_close (disk);
  
  grub_free (dev);

  return 0;
}

grub_err_t
grub_device_close (grub_device_t device)
{
  if (device->disk)
    grub_disk_close (device->disk);

  grub_free (device);

  return grub_errno;
}
