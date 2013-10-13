/* hostdisk.c - emulate biosdisk */
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

#if !defined (__CYGWIN__) && !defined (__MINGW32__) && !defined (__AROS__)

#ifdef __linux__
# include <sys/ioctl.h>         /* ioctl */
# include <sys/mount.h>
# if !defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1)))
/* Maybe libc doesn't have large file support.  */
#  include <linux/unistd.h>     /* _llseek */
# endif /* (GLIBC < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR < 1)) */
#endif /* __linux__ */

grub_uint64_t
grub_util_get_fd_size (grub_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  struct stat st;
  grub_int64_t ret = -1;

  if (fstat (fd, &st) < 0)
    /* TRANSLATORS: "stat" comes from the name of POSIX function.  */
    grub_util_error (_("cannot stat `%s': %s"), name, strerror (errno));
#if GRUB_DISK_DEVS_ARE_CHAR
  if (S_ISCHR (st.st_mode))
#else
  if (S_ISBLK (st.st_mode))
#endif
    ret = grub_util_get_fd_size_os (fd, name, log_secsize);
  if (ret != -1LL)
    return ret;

  if (log_secsize)
    *log_secsize = 9;

  return st.st_size;
}

#if defined(__linux__) && (!defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1))))
  /* Maybe libc doesn't have large file support.  */
grub_err_t
grub_util_fd_seek (grub_util_fd_t fd, const char *name, grub_uint64_t off)
{
  loff_t offset, result;
  static int _llseek (uint filedes, ulong hi, ulong lo,
		      loff_t *res, uint wh);
  _syscall5 (int, _llseek, uint, filedes, ulong, hi, ulong, lo,
	     loff_t *, res, uint, wh);

  offset = (loff_t) off;
  if (_llseek (fd, offset >> 32, offset & 0xffffffff, &result, SEEK_SET))
    return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
		       name, strerror (errno));
  return GRUB_ERR_NONE;
}
#else
grub_err_t
grub_util_fd_seek (grub_util_fd_t fd, const char *name, grub_uint64_t off)
{
  off_t offset = (off_t) off;

  if (lseek (fd, offset, SEEK_SET) != offset)
    return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
		       name, strerror (errno));
  return 0;
}
#endif


/* Read LEN bytes from FD in BUF. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
ssize_t
grub_util_fd_read (grub_util_fd_t fd, char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = read (fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
    }

  return size;
}

/* Write LEN bytes from BUF to FD. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
ssize_t
grub_util_fd_write (grub_util_fd_t fd, const char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = write (fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
    }

  return size;
}

#if !defined (__NetBSD__) && !defined (__APPLE__) && !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
grub_util_fd_t
grub_util_fd_open (const char *os_dev, int flags)
{
#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

  return open (os_dev, flags);
}
#endif

const char *
grub_util_fd_strerror (void)
{
  return strerror (errno);
}

void
grub_util_fd_sync (grub_util_fd_t fd)
{
  fsync (fd);
}

void
grub_util_fd_close (grub_util_fd_t fd)
{
  close (fd);
}

char *
canonicalize_file_name (const char *path)
{
#if defined (PATH_MAX)
  char *ret;

  ret = xmalloc (PATH_MAX);
  if (!realpath (path, ret))
    return NULL;
  return ret;
#else
  return realpath (path, NULL);
#endif
}

FILE *
grub_util_fopen (const char *path, const char *mode)
{
  return fopen (path, mode);
}

#endif
