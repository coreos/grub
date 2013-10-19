/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010,2011,2012,2013  Free Software Foundation, Inc.
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
#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <grub/types.h>

#include <grub/util/misc.h>

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/charset.h>
#include <grub/util/windows.h>
#include <windows.h>
#include <winioctl.h>

#if SIZEOF_TCHAR == 1
#define tcsnicmp strncasecmp
#elif SIZEOF_TCHAR == 2
#define tcsnicmp wcsnicmp
#endif

char **
grub_guess_root_devices (const char *dir)
{
  char **os_dev = NULL;
  TCHAR *dirwindows, *mntpointwindows;
  TCHAR *ptr;
  TCHAR volumename[100];
  size_t mntpointwindows_sz;

  dirwindows = grub_util_get_windows_path (dir);
  if (!dirwindows)
    return 0;

  mntpointwindows_sz = strlen (dir) * 2 + 1;
  mntpointwindows = xmalloc ((mntpointwindows_sz + 1) * sizeof (mntpointwindows[0]));

  if (!GetVolumePathName (dirwindows,
			  mntpointwindows,
			  mntpointwindows_sz))
    {
      free (dirwindows);
      free (mntpointwindows);
      return 0;      
    }

  if (!mntpointwindows[0])
    {
      free (dirwindows);
      free (mntpointwindows);
      return 0;      
    }
  
  for (ptr = mntpointwindows; *ptr; ptr++);
  if (*(ptr - 1) != '\\')
    {
      *ptr = '\\';
      *(ptr + 1) = '\0';
    }

  if (!GetVolumeNameForVolumeMountPoint (mntpointwindows,
					 volumename,
					 ARRAY_SIZE (volumename)))
    {
      free (dirwindows);
      free (mntpointwindows);
      return 0;
    }
  os_dev = xmalloc (2 * sizeof (os_dev[0]));

  for (ptr = volumename; *ptr; ptr++);
  while (ptr > volumename && *(ptr - 1) == '\\')
    *--ptr = '\0';

  os_dev[0] = grub_util_tchar_to_utf8 (volumename);
  free (dirwindows);
  free (mntpointwindows);

  if (!os_dev[0])
    {
      free (os_dev);
      return 0;
    }

  os_dev[1] = 0;

  return os_dev;
}

char *
grub_util_part_to_disk (const char *os_dev,
			struct stat *st __attribute__ ((unused)),
			int *is_part)
{
  HANDLE hd;
  LPTSTR name = grub_util_get_windows_path (os_dev);
  VOLUME_DISK_EXTENTS exts;
  DWORD extsbytes;
  char *ret;

  if (((name[0] == '/') || (name[0] == '\\')) &&
      ((name[1] == '/') || (name[1] == '\\')) &&
      ((name[2] == '.') || (name[2] == '?')) &&
      ((name[3] == '/') || (name[3] == '\\'))
      && (tcsnicmp (name + 4, TEXT("PhysicalDrive"), sizeof ("PhysicalDrive") - 1) == 0
	  || tcsnicmp (name + 4, TEXT("Harddisk"), sizeof ("Harddisk") - 1) == 0
	  || ((name[4] == 'A' || name[4] == 'a' || name[4] == 'B' || name[4] == 'b')
	      && name[5] == ':' && name[6] == '\0')))
    {
      grub_util_info ("Matches full disk pattern");
      ret = grub_util_tchar_to_utf8 (name);
      free (name);
      return ret;
    }

  hd = CreateFile (name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   0, OPEN_EXISTING, 0, 0);
  if (hd == INVALID_HANDLE_VALUE)
    {
      grub_util_info ("CreateFile failed");
      ret = grub_util_tchar_to_utf8 (name);
      free (name);
      return ret;
    }

  if (!DeviceIoControl(hd, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		       NULL, 0, &exts, sizeof (exts), &extsbytes, NULL))
    {
      grub_util_info ("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed");
      ret = grub_util_tchar_to_utf8 (name);
      CloseHandle (hd);
      free (name);
      return ret;
    }
  
  CloseHandle (hd);

  *is_part = 1;
  free (name);
  return xasprintf ("\\\\?\\PhysicalDrive%lu", (unsigned long) exts.Extents[0].DiskNumber);
}

enum grub_dev_abstraction_types
grub_util_get_dev_abstraction_os (const char *os_dev __attribute__((unused)))
{
  return GRUB_DEV_ABSTRACTION_NONE;
}

int
grub_util_pull_device_os (const char *os_dev __attribute__ ((unused)),
			  enum grub_dev_abstraction_types ab __attribute__ ((unused)))
{
  return 0;
}

char *
grub_util_get_grub_dev_os (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}


grub_disk_addr_t
grub_util_find_partition_start_os (const char *os_dev)
{
  HANDLE hd;
  LPTSTR name = grub_util_get_windows_path (os_dev);
  VOLUME_DISK_EXTENTS exts;
  DWORD extsbytes;
  char *ret;

  if (((name[0] == '/') || (name[0] == '\\')) &&
      ((name[1] == '/') || (name[1] == '\\')) &&
      ((name[2] == '.') || (name[2] == '?')) &&
      ((name[3] == '/') || (name[3] == '\\'))
      && (tcsnicmp (name + 4, TEXT("PhysicalDrive"), sizeof ("PhysicalDrive") - 1) == 0
	  || tcsnicmp (name + 4, TEXT("Harddisk"), sizeof ("Harddisk") - 1) == 0
	  || ((name[4] == 'A' || name[4] == 'a' || name[4] == 'B' || name[4] == 'b')
	      && name[5] == ':' && name[6] == '\0')))
    {
      ret = grub_util_tchar_to_utf8 (name);
      free (name);
      return 0;
    }

  hd = CreateFile (name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   0, OPEN_EXISTING, 0, 0);
  if (hd == INVALID_HANDLE_VALUE)
    {
      ret = grub_util_tchar_to_utf8 (name);
      free (name);
      return 0;
    }

  if (!DeviceIoControl(hd, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		       NULL, 0, &exts, sizeof (exts), &extsbytes, NULL))
    {
      ret = grub_util_tchar_to_utf8 (name);
      CloseHandle (hd);
      free (name);
      return 0;
    }
  
  CloseHandle (hd);
  free (name);
  return exts.Extents[0].StartingOffset.QuadPart / 512;
}

int
grub_util_biosdisk_is_floppy (grub_disk_t disk)
{
  int ret;
  const char *dname;
  LPTSTR name;

  dname = grub_util_biosdisk_get_osdev (disk);

  if (!dname)
    return 0;

  name = grub_util_get_windows_path (dname);

  ret = (((name[0] == '/') || (name[0] == '\\')) &&
	 ((name[1] == '/') || (name[1] == '\\')) &&
	 ((name[2] == '.') || (name[2] == '?')) &&
	 ((name[3] == '/') || (name[3] == '\\'))
	 && (name[4] == 'A' || name[4] == 'a' || name[4] == 'B' || name[4] == 'b')
	 && name[5] == ':' && name[6] == '\0');
  free (name);

  return ret;
}
