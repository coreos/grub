/* ubootdisk.c - disk subsystem support for U-Boot platforms */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/disk.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/uboot/disk.h>
#include <grub/uboot/uboot.h>

static struct ubootdisk_data *fd_devices;
static struct ubootdisk_data *hd_devices;
static struct ubootdisk_data *cd_devices;

/*
 * grub_ubootdisk_register():
 *   Called for each disk device enumerated as part of U-Boot initialization
 *   code.
 */
grub_err_t
grub_ubootdisk_register (struct device_info *newdev, int handle)
{
  struct ubootdisk_data *d;
  enum disktype type;

#define STOR_TYPE(x) ((x) & 0x0ff0)
  switch (STOR_TYPE (newdev->type))
    {
    case DT_STOR_IDE:
    case DT_STOR_SATA:
      /* hd */
      type = hd;
      break;
    case DT_STOR_MMC:
    case DT_STOR_USB:
      /* fd */
      type = fd;
      break;
    default:
      return GRUB_ERR_BAD_DEVICE;
      break;
    }

  d = (struct ubootdisk_data *) grub_malloc (sizeof (struct ubootdisk_data));
  if (!d)
    return GRUB_ERR_OUT_OF_MEMORY;
  d->handle = handle;
  d->cookie = newdev->cookie;
  d->opencount = 0;

  switch (type)
    {
    case cd:
      grub_dprintf ("ubootdisk", "registering cd device\n");
      d->next = cd_devices;
      cd_devices = d;

      break;
    case fd:
      grub_dprintf ("ubootdisk", "registering fd device\n");
      d->next = fd_devices;
      fd_devices = d;

      break;
    case hd:
      grub_dprintf ("ubootdisk", "registering hd device\n");
      d->next = hd_devices;
      hd_devices = d;

      break;
    default:
      grub_free (d);
      return GRUB_ERR_BAD_DEVICE;
    }

  return 0;
}

/*
 * uboot_disk_iterate():
 *   Itarator over enumerated disk devices.
 */
static int
uboot_disk_iterate (grub_disk_dev_iterate_hook_t hook, void *hook_data,
		    grub_disk_pull_t pull)
{
  struct ubootdisk_data *d;
  char buf[16];
  int count;

  switch (pull)
    {
    case GRUB_DISK_PULL_NONE:
      /* "hd" - built-in mass-storage */
      for (d = hd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf) - 1, "hd%d", count);
	  grub_dprintf ("ubootdisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}
      break;
    case GRUB_DISK_PULL_REMOVABLE:
      /* "floppy" - removable mass storage */
      for (d = fd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf) - 1, "fd%d", count);
	  grub_dprintf ("ubootdisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}

      /* "cdrom" - removeable read-only storage */
      for (d = cd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf) - 1, "cd%d", count);
	  grub_dprintf ("ubootdisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}
      break;
    default:
      return 0;
    }

  return 0;
}

/* Helper function for uboot_disk_open. */
static struct ubootdisk_data *
get_device (struct ubootdisk_data *devices, int num)
{
  struct ubootdisk_data *d;

  for (d = devices; d && num; d = d->next, num--)
    ;

  if (num == 0)
    return d;

  return NULL;
}

/*
 * uboot_disk_open():
 *   Opens a disk device already enumerated.
 */
static grub_err_t
uboot_disk_open (const char *name, struct grub_disk *disk)
{
  struct ubootdisk_data *d;
  struct device_info *devinfo;
  int num;
  int retval;

  grub_dprintf ("ubootdisk", "Opening '%s'\n", name);

  num = grub_strtoul (name + 2, 0, 10);
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_dprintf ("ubootdisk", "Opening '%s' failed, invalid number\n",
		    name);
      goto fail;
    }

  if (name[1] != 'd')
    {
      grub_dprintf ("ubootdisk", "Opening '%s' failed, invalid name\n", name);
      goto fail;
    }

  switch (name[0])
    {
    case 'f':
      d = get_device (fd_devices, num);
      break;
    case 'c':
      d = get_device (cd_devices, num);
      break;
    case 'h':
      d = get_device (hd_devices, num);
      break;
    default:
      goto fail;
    }

  if (!d)
    goto fail;

  /*
   * Subsystems may call open on the same device recursively - but U-Boot
   * does not deal with this. So simply keep track of number of calls and
   * return success if already open.
   */
  if (d->opencount > 0)
    {
      grub_dprintf ("ubootdisk", "(%s) already open\n", disk->name);
      d->opencount++;
      retval = 0;
    }
  else
    {
      retval = uboot_dev_open (d->handle);
      if (retval != 0)
	goto fail;
      d->opencount = 1;
    }

  grub_dprintf ("ubootdisk", "cookie: 0x%08x\n", (grub_addr_t) d->cookie);
  disk->id = (grub_addr_t) d->cookie;

  /* Device has previously been enumerated, so this should never fail */
  if ((devinfo = uboot_dev_get (d->handle)) == NULL)
    goto fail;

  d->block_size = devinfo->di_stor.block_size;
  if (d->block_size == 0)
    {
      grub_printf ("%s: no block size!\n", __FUNCTION__);
      return GRUB_ERR_IO;
    }

  for (disk->log_sector_size = 0;
       (1U << disk->log_sector_size) < d->block_size;
       disk->log_sector_size++);

  grub_dprintf ("ubootdisk", "(%s) blocksize=%d, log_sector_size=%d\n",
		disk->name, d->block_size, disk->log_sector_size);

  disk->total_sectors = GRUB_DISK_SIZE_UNKNOWN;
  disk->data = d;

  return GRUB_ERR_NONE;

fail:
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");
}

static void
uboot_disk_close (struct grub_disk *disk)
{
  struct ubootdisk_data *d;
  int retval;

  d = disk->data;

  /*
   * In mirror of open function, keep track of number of calls to close and
   * send on to U-Boot only when opencount would decrease to 0.
   */
  if (d->opencount > 1)
    {
      grub_dprintf ("ubootdisk", "Closed (%s)\n", disk->name);

      d->opencount--;
    }
  else if (d->opencount == 1)
    {
      retval = uboot_dev_close (d->handle);
      d->opencount--;
      grub_dprintf ("ubootdisk", "closed %s (%d)\n", disk->name, retval);
    }
  else
    {
      grub_dprintf ("ubootdisk", "device %s not open!\n", disk->name);
    }
}

/*
 * uboot_disk_read():
 *   Called from within disk subsystem to read a sequence of blocks into the
 *   disk cache. Maps directly on top of U-Boot API, only wrap in some error
 *   handling.
 */
static grub_err_t
uboot_disk_read (struct grub_disk *disk,
		 grub_disk_addr_t offset, grub_size_t numblocks, char *buf)
{
  struct ubootdisk_data *d;
  lbasize_t real_size;
  int retval;

  d = disk->data;

  retval = uboot_dev_read (d->handle, buf, numblocks, offset, &real_size);
  grub_dprintf ("ubootdisk",
		"retval=%d, numblocks=%d, real_size=%llu, sector=%llu\n",
		retval, numblocks, (grub_uint64_t) real_size,
		(grub_uint64_t) offset);
  if (retval != 0)
    return grub_error (GRUB_ERR_IO, "U-Boot disk read error");

  return GRUB_ERR_NONE;
}

static grub_err_t
uboot_disk_write (struct grub_disk *disk __attribute__ ((unused)),
		  grub_disk_addr_t sector __attribute__ ((unused)),
		  grub_size_t size __attribute__ ((unused)),
		  const char *buf __attribute__ ((unused)))
{
  grub_dprintf ("ubootdisk", "attempt to write\n");
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static struct grub_disk_dev grub_ubootdisk_dev = {
  .name = "ubootdisk",
  .id = GRUB_DISK_DEVICE_UBOOTDISK_ID,
  .iterate = uboot_disk_iterate,
  .open = uboot_disk_open,
  .close = uboot_disk_close,
  .read = uboot_disk_read,
  .write = uboot_disk_write,
  .next = 0
};

void
grub_ubootdisk_init (void)
{
  grub_disk_dev_register (&grub_ubootdisk_dev);
}

void
grub_ubootdisk_fini (void)
{
  grub_disk_dev_unregister (&grub_ubootdisk_dev);
}
