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

#include <grub/util/windows.h>
#include <grub/charset.h>

#include <windows.h>
#include <winioctl.h>

#if SIZEOF_TCHAR == 1

LPTSTR
grub_util_utf8_to_tchar (const char *in)
{
  return xstrdup (in);
}

char *
grub_util_tchar_to_utf8 (LPCTSTR in)
{
  return xstrdup (in);
}

#elif SIZEOF_TCHAR == 2

LPTSTR
grub_util_utf8_to_tchar (const char *in)
{
  LPTSTR ret;
  size_t ssz = strlen (in);
  size_t tsz = 2 * (GRUB_MAX_UTF16_PER_UTF8 * ssz + 1);
  ret = xmalloc (tsz);
  tsz = grub_utf8_to_utf16 (ret, tsz,
			    (const grub_uint8_t *) in, ssz, NULL);
  ret[tsz] = 0;
  return ret;
}

char *
grub_util_tchar_to_utf8 (LPCTSTR in)
{
  size_t ssz;
  for (ssz = 0; in[ssz]; ssz++);

  size_t tsz = GRUB_MAX_UTF8_PER_UTF16 * ssz + 1;
  grub_uint8_t *ret = xmalloc (tsz);
  *grub_utf16_to_utf8 (ret, in, ssz) = '\0';
  return (char *) ret;
}

#else
#error "Unsupported TCHAR size"
#endif

#ifdef __CYGWIN__
LPTSTR
grub_util_get_windows_path (const char *path)
{
  LPTSTR winpath;
  winpath = xmalloc (sizeof (winpath[0]) * PATH_MAX);
  if (cygwin_conv_path ((sizeof (winpath[0]) == 1 ? CCP_POSIX_TO_WIN_A
			 : CCP_POSIX_TO_WIN_W) | CCP_ABSOLUTE, path, winpath,
			sizeof (winpath[0]) * PATH_MAX))
    grub_util_error ("%s", _("cygwin_conv_path() failed"));
  return winpath;
}
#else
LPTSTR
grub_util_get_windows_path (const char *path)
{
  LPTSTR fpa;
  LPTSTR tpath;

  tpath = grub_util_utf8_to_tchar (path);

  fpa = xmalloc (PATH_MAX * sizeof (fpa[0]));
  if (!_wfullpath (fpa, tpath, PATH_MAX))
    {
      free (fpa);
      return tpath;
    }

  free (tpath);
  return fpa;
}
#endif

grub_uint64_t
grub_util_get_fd_size (grub_util_fd_t hd, const char *name_in,
		       unsigned *log_secsize)
{
  grub_int64_t size = -1LL;
  int log_sector_size = 9;
  LPTSTR name = grub_util_get_windows_path (name_in);

  if (log_secsize)
    *log_secsize = log_sector_size;

  if (((name[0] == '/') || (name[0] == '\\')) &&
      ((name[1] == '/') || (name[1] == '\\')) &&
      ((name[2] == '.') || (name[2] == '?')) &&
      ((name[3] == '/') || (name[3] == '\\')))
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

  return size;
}

void
grub_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}

int
grub_util_fd_seek (grub_util_fd_t fd, grub_uint64_t off)
{
  LARGE_INTEGER offset;
  offset.QuadPart = off;

  if (!SetFilePointerEx (fd, offset, NULL, FILE_BEGIN))
    return -1;
  return 0;
}

grub_util_fd_t
grub_util_fd_open (const char *os_dev, int flags)
{
  DWORD flg = 0, crt;
  LPTSTR dev = grub_util_get_windows_path (os_dev);
  grub_util_fd_t ret;

  if (flags & GRUB_UTIL_FD_O_WRONLY)
    flg |= GENERIC_WRITE;
  if (flags & GRUB_UTIL_FD_O_RDONLY)
    flg |= GENERIC_READ;

  if (flags & GRUB_UTIL_FD_O_CREATTRUNC)
    crt = CREATE_ALWAYS;
  else
    crt = OPEN_EXISTING;

  ret = CreateFile (dev, flg, FILE_SHARE_READ | FILE_SHARE_WRITE,
		    0, crt, 0, 0);
  free (dev);
  return ret;
}

ssize_t
grub_util_fd_read (grub_util_fd_t fd, char *buf, size_t len)
{
  DWORD real_read;
  if (!ReadFile(fd, buf, len, &real_read, NULL))
    {
      grub_util_info ("read err %x", (int) GetLastError ());
      return -1;
    }
  grub_util_info ("successful read");
  return real_read;
}

ssize_t
grub_util_fd_write (grub_util_fd_t fd, const char *buf, size_t len)
{
  DWORD real_read;
  if (!WriteFile(fd, buf, len, &real_read, NULL))
    {
      grub_util_info ("write err %x", (int) GetLastError ());
      return -1;
    }

  grub_util_info ("successful write");
  return real_read;
}

void
grub_util_fd_sync (grub_util_fd_t fd)
{
  FlushFileBuffers (fd);
}

void
grub_util_fd_close (grub_util_fd_t fd)
{
  CloseHandle (fd);
}

const char *
grub_util_fd_strerror (void)
{
  DWORD err;
  static TCHAR tbuf[1024];
  err = GetLastError ();
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM |
		 FORMAT_MESSAGE_IGNORE_INSERTS,
		 NULL, err,
		 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		 tbuf, ARRAY_SIZE (tbuf), NULL);

#if SIZEOF_TCHAR == 1
  return (char *) tbuf;
#elif SIZEOF_TCHAR == 2
  static grub_uint8_t buf[ARRAY_SIZE (tbuf) * GRUB_MAX_UTF8_PER_UTF16 + 1];
  *grub_utf16_to_utf8 (buf, tbuf, ARRAY_SIZE (tbuf)) = '\0';
  return (char *) buf;
#else
#error "Unsupported TCHAR size"
#endif
}

char *
canonicalize_file_name (const char *path)
{
  char *ret;
  LPTSTR windows_path;
  ret = xmalloc (PATH_MAX);

  windows_path = grub_util_get_windows_path (path);
  if (!windows_path)
    return NULL;
  ret = grub_util_tchar_to_utf8 (windows_path);
  free (windows_path);
 
  return ret;
}


#ifdef __MINGW32__

FILE *
grub_util_fopen (const char *path, const char *mode)
{
  LPTSTR tpath;
  FILE *ret;
  tpath = grub_util_get_windows_path (path);
#if SIZEOF_TCHAR == 1
  ret = fopen (tpath, tmode);
#else
  LPTSTR tmode;
  tmode = grub_util_utf8_to_tchar (mode);
  ret = _wfopen (tpath, tmode);
  free (tmode);
#endif
  free (tpath);
  return ret;
}

int fsync (int fno __attribute__ ((unused)))
{
  return 0;
}

#else

FILE *
grub_util_fopen (const char *path, const char *mode)
{
  return fopen (path, mode);
}

#endif
