/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2006,2007,2008,2009,2010,2011,2012,2013  Free Software Foundation, Inc.
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

#include <config-util.h>

#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <windows.h>
#include <winioctl.h>
#include "dirname.h"

grub_int64_t
grub_util_get_fd_size_os (int fd __attribute__ ((unused)),
			  const char *name_in,
			  unsigned *log_secsize)
{
  HANDLE hd;
  grub_int64_t size = -1LL;
  int log_sector_size = 9;
  char *name = xstrdup (name_in);

  if (log_secsize)
    *log_secsize = log_sector_size;

  strip_trailing_slashes(name);
  hd = CreateFile (name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   0, OPEN_EXISTING, 0, 0);

  if (hd == INVALID_HANDLE_VALUE)
    {
      free (name);
      return size;
    }

  if (((name[0] == '/') || (name[0] == '\\')) &&
      ((name[1] == '/') || (name[1] == '\\')) &&
      (name[2] == '.') &&
      ((name[3] == '/') || (name[3] == '\\')) &&
      (! strncasecmp (name + 4, "PHYSICALDRIVE", 13)))
    {
      DWORD nr;
      DISK_GEOMETRY g;

      if (! DeviceIoControl (hd, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             0, 0, &g, sizeof (g), &nr, 0))
        goto fail;

      size = g.Cylinders.QuadPart;
      size *= g.TracksPerCylinder * g.SectorsPerTrack * g.BytesPerSector;

      for (log_sector_size = 0;
	   (1 << log_sector_size) < g.BytesPerSector;
	   log_sector_size++);
    }
  else
    {
      ULARGE_INTEGER s;

      s.LowPart = GetFileSize (hd, &s.HighPart);
      size = s.QuadPart;
    }

 fail:

  if (log_secsize)
    *log_secsize = log_sector_size;

  free (name);

  CloseHandle (hd);

  return size;
}

void
grub_hostdisk_configure_device_driver (int fd __attribute__ ((unused)))
{
}

void
grub_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}
