/* ofdisk.c - Open Firmware disk access.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2004  Free Software Foundation, Inc.
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

#include <pupa/misc.h>
#include <pupa/disk.h>
#include <pupa/mm.h>
#include <pupa/machine/ieee1275.h>

static int
pupa_ofdisk_iterate (int (*hook) (const char *name))
{
  int dev_iterate (struct pupa_ieee1275_devalias *alias)
    {
      if (! pupa_strcmp (alias->type, "block"))
	hook (alias->name);
      return 0;
    }

  pupa_devalias_iterate (dev_iterate);
  return 0;
}

static pupa_err_t
pupa_ofdisk_open (const char *name, pupa_disk_t disk)
{
  pupa_ieee1275_phandle_t devalias;
  pupa_ieee1275_phandle_t dev;
  pupa_ieee1275_ihandle_t dev_ihandle = 0;
  char *devpath = 0;
  /* XXX: This should be large enough for any possible case.  */
  char prop[64];
  pupa_size_t pathlen;
  int actual;

  if (pupa_ieee1275_finddevice ("/aliases", &devalias))
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Can't read the aliases");
  
  pupa_ieee1275_get_property_length (devalias, name, &pathlen);
  devpath = pupa_malloc (pathlen);
  if (! devpath)
    return pupa_errno;

  if (pupa_ieee1275_get_property (devalias, name, devpath, pathlen, &actual))
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "No such device alias");

  /* To access the complete disk add `:0'.  */
  pupa_strcat (devpath, ":0");
  pupa_ieee1275_open (devpath, &dev_ihandle);
  if (! dev_ihandle)
    return pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Can't open device");
  
  if (pupa_ieee1275_finddevice (devpath, &dev))
    {
      pupa_error (PUPA_ERR_UNKNOWN_DEVICE, "Can't read device properties");
      goto fail;
    }

  if (pupa_ieee1275_get_property (dev, "device_type", prop, sizeof (prop),
				  &actual))
    {
      pupa_error (PUPA_ERR_BAD_DEVICE, "Can't read the device type");
      goto fail;
    }

  if (pupa_strcmp (prop, "block"))
    {
      pupa_error (PUPA_ERR_BAD_DEVICE, "Not a block device");
      goto fail;
    }

  /* XXX: There is no property to read the number of blocks.  There
     should be a property `#blocks', but it is not there.  Perhaps it
     is possible to use seek for this.  */
  disk->total_sectors = 0xFFFFFFFFUL;

  /* XXX: Is it ok to use this?  Perhaps it is better to use the path
     or some property.  */
  disk->id = dev;

  /* XXX: Read this, somehow.  */
  disk->has_partitions = 1;
  disk->data = (void *) dev_ihandle;

 fail:
  if (pupa_errno)
    pupa_ieee1275_close (dev_ihandle);
  pupa_free (devpath);
  return pupa_errno;
}

static void
pupa_ofdisk_close (pupa_disk_t disk)
{
  pupa_ieee1275_close ((pupa_ieee1275_ihandle_t) disk->data);
}

static pupa_err_t
pupa_ofdisk_read (pupa_disk_t disk, unsigned long sector,
		  unsigned long size, char *buf)
{
  int status;
  int actual;
  unsigned long long pos;

  pos = (unsigned long long) sector * 512UL;

  pupa_ieee1275_seek ((pupa_ieee1275_ihandle_t) disk->data, (int) (pos >> 32),
		      (int) pos & 0xFFFFFFFFUL, &status);
  if (status != 0)
    return pupa_error (PUPA_ERR_READ_ERROR,
		       "Seek error, can't seek block %d", sector);
  pupa_ieee1275_read ((pupa_ieee1275_ihandle_t) disk->data, buf,
		      size * 512UL, &actual);
  if (actual != actual)
    return pupa_error (PUPA_ERR_READ_ERROR, "Read error on block: %d", sector);
    
  return 0;
}

static pupa_err_t
pupa_ofdisk_write (pupa_disk_t disk __attribute ((unused)),
		   unsigned long sector __attribute ((unused)),
		   unsigned long size __attribute ((unused)),
		   const char *buf __attribute ((unused)))
{
  return PUPA_ERR_NOT_IMPLEMENTED_YET;
}

static struct pupa_disk_dev pupa_ofdisk_dev =
  {
    .name = "ofdisk",
    .iterate = pupa_ofdisk_iterate,
    .open = pupa_ofdisk_open,
    .close = pupa_ofdisk_close,
    .read = pupa_ofdisk_read,
    .write = pupa_ofdisk_write,
    .next = 0
  };

void
pupa_ofdisk_init (void)
{
  pupa_disk_dev_register (&pupa_ofdisk_dev);
}
