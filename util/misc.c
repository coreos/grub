/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <config.h>

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/cache.h>
#include <grub/util/misc.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/time.h>

#include "progname.h"

/* Include malloc.h, only if memalign is available. It is known that
   memalign is declared in malloc.h in all systems, if present.  */
#ifdef HAVE_MEMALIGN
# include <malloc.h>
#endif

#ifdef __MINGW32__
#include <windows.h>
#include <winioctl.h>
#endif

int verbosity = 0;

void
grub_util_warn (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: warn: ", program_name);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
}

void
grub_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;

      fprintf (stderr, "%s: info: ", program_name);
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fputc ('\n', stderr);
      fflush (stderr);
    }
}

void
grub_util_error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: error: ", program_name);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

int
grub_err_printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = vfprintf (stderr, fmt, ap);
  va_end (ap);

  return ret;
}

void *
xmalloc (size_t size)
{
  void *p;

  p = malloc (size);
  if (! p)
    grub_util_error ("out of memory");

  return p;
}

void *
xrealloc (void *ptr, size_t size)
{
  ptr = realloc (ptr, size);
  if (! ptr)
    grub_util_error ("out of memory");

  return ptr;
}

char *
xstrdup (const char *str)
{
  size_t len;
  char *dup;

  len = strlen (str);
  dup = (char *) xmalloc (len + 1);
  memcpy (dup, str, len + 1);

  return dup;
}

char *
grub_util_get_path (const char *dir, const char *file)
{
  char *path;

  path = (char *) xmalloc (strlen (dir) + 1 + strlen (file) + 1);
  sprintf (path, "%s/%s", dir, file);
  return path;
}

size_t
grub_util_get_fp_size (FILE *fp)
{
  struct stat st;

  if (fflush (fp) == EOF)
    grub_util_error ("fflush failed");

  if (fstat (fileno (fp), &st) == -1)
    grub_util_error ("fstat failed");

  return st.st_size;
}

size_t
grub_util_get_image_size (const char *path)
{
  struct stat st;

  grub_util_info ("getting the size of %s", path);

  if (stat (path, &st) == -1)
    grub_util_error ("cannot stat %s", path);

  return st.st_size;
}

void
grub_util_read_at (void *img, size_t size, off_t offset, FILE *fp)
{
  if (fseeko (fp, offset, SEEK_SET) == -1)
    grub_util_error ("seek failed");

  if (fread (img, 1, size, fp) != size)
    grub_util_error ("read failed");
}

char *
grub_util_read_image (const char *path)
{
  char *img;
  FILE *fp;
  size_t size;

  grub_util_info ("reading %s", path);

  size = grub_util_get_image_size (path);
  img = (char *) xmalloc (size);

  fp = fopen (path, "rb");
  if (! fp)
    grub_util_error ("cannot open %s", path);

  grub_util_read_at (img, size, 0, fp);

  fclose (fp);

  return img;
}

void
grub_util_load_image (const char *path, char *buf)
{
  FILE *fp;
  size_t size;

  grub_util_info ("reading %s", path);

  size = grub_util_get_image_size (path);

  fp = fopen (path, "rb");
  if (! fp)
    grub_util_error ("cannot open %s", path);

  if (fread (buf, 1, size, fp) != size)
    grub_util_error ("cannot read %s", path);

  fclose (fp);
}

void
grub_util_write_image_at (const void *img, size_t size, off_t offset, FILE *out)
{
  grub_util_info ("writing 0x%x bytes at offset 0x%x", size, offset);
  if (fseeko (out, offset, SEEK_SET) == -1)
    grub_util_error ("seek failed");
  if (fwrite (img, 1, size, out) != size)
    grub_util_error ("write failed");
}

void
grub_util_write_image (const char *img, size_t size, FILE *out)
{
  grub_util_info ("writing 0x%x bytes", size);
  if (fwrite (img, 1, size, out) != size)
    grub_util_error ("write failed");
}

void *
grub_malloc (grub_size_t size)
{
  return xmalloc (size);
}

void *
grub_zalloc (grub_size_t size)
{
  void *ret;

  ret = xmalloc (size);
  memset (ret, 0, size);
  return ret;
}

void
grub_free (void *ptr)
{
  free (ptr);
}

void *
grub_realloc (void *ptr, grub_size_t size)
{
  return xrealloc (ptr, size);
}

void *
grub_memalign (grub_size_t align, grub_size_t size)
{
  void *p;

#if defined(HAVE_POSIX_MEMALIGN)
  if (posix_memalign (&p, align, size) != 0)
    p = 0;
#elif defined(HAVE_MEMALIGN)
  p = memalign (align, size);
#else
  (void) align;
  (void) size;
  grub_util_error ("grub_memalign is not supported");
#endif

  if (! p)
    grub_util_error ("out of memory");

  return p;
}

/* Some functions that we don't use.  */
void
grub_mm_init_region (void *addr __attribute__ ((unused)),
		     grub_size_t size __attribute__ ((unused)))
{
}

void
grub_register_exported_symbols (void)
{
}

void
grub_exit (void)
{
  exit (1);
}

grub_uint32_t
grub_get_rtc (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * GRUB_TICKS_PER_SECOND
	  + (((tv.tv_sec % GRUB_TICKS_PER_SECOND) * 1000000 + tv.tv_usec)
	     * GRUB_TICKS_PER_SECOND / 1000000));
}

grub_uint64_t
grub_get_time_ms (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

#ifdef __MINGW32__

void
grub_millisleep (grub_uint32_t ms)
{
  Sleep (ms);
}

#else

void
grub_millisleep (grub_uint32_t ms)
{
  struct timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep (&ts, NULL);
}

#endif

void
grub_arch_sync_caches (void *address __attribute__ ((unused)),
		       grub_size_t len __attribute__ ((unused)))
{
}

#ifndef HAVE_VASPRINTF

int
vasprintf (char **buf, const char *fmt, va_list ap)
{
  /* Should be large enough.  */
  *buf = xmalloc (512);

  return vsprintf (*buf, fmt, ap);
}

#endif

#ifndef  HAVE_ASPRINTF

int
asprintf (char **buf, const char *fmt, ...)
{
  int status;
  va_list ap;

  va_start (ap, fmt);
  status = vasprintf (*buf, fmt, ap);
  va_end (ap);

  return status;
}

#endif

char *
xasprintf (const char *fmt, ...)
{
  va_list ap;
  char *result;

  va_start (ap, fmt);
  if (vasprintf (&result, fmt, ap) < 0)
    {
      if (errno == ENOMEM)
	grub_util_error ("out of memory");
      return NULL;
    }

  return result;
}

#ifdef __MINGW32__

void sync (void)
{
}

int fsync (int fno __attribute__ ((unused)))
{
  return 0;
}

void sleep (int s)
{
  Sleep (s * 1000);
}

grub_int64_t
grub_util_get_disk_size (char *name)
{
  HANDLE hd;
  grub_int64_t size = -1LL;

  hd = CreateFile (name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   0, OPEN_EXISTING, 0, 0);

  if (hd == INVALID_HANDLE_VALUE)
    return size;

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
    }
  else
    {
      LARGE_INTEGER s;

      s.LowPart = GetFileSize (hd, &s.HighPart);
      size = s.QuadPart;
    }

fail:

  CloseHandle (hd);

  return size;
}

#endif /* __MINGW32__ */

/* This function never prints trailing slashes (so that its output
   can be appended a slash unconditionally).  */
char *
make_system_path_relative_to_its_root (const char *path)
{
  struct stat st;
  char *p, *buf, *buf2, *buf3;
  uintptr_t offset = 0;
  dev_t num;
  size_t len;

  /* canonicalize.  */
  p = realpath (path, NULL);

  if (p == NULL)
    {
      if (errno != EINVAL)
	grub_util_error ("failed to get realpath of %s", path);
      else
	grub_util_error ("realpath not supporting (path, NULL)");
    }
  len = strlen (p) + 1;
  buf = strdup (p);
  free (p);

  if (stat (buf, &st) < 0)
    grub_util_error ("can not stat %s: %s", buf, strerror (errno));

  buf2 = strdup (buf);
  num = st.st_dev;

  /* This loop sets offset to the number of chars of the root
     directory we're inspecting.  */
  while (1)
    {
      p = strrchr (buf, '/');
      if (p == NULL)
	/* This should never happen.  */
	grub_util_error ("FIXME: no / in buf. (make_system_path_relative_to_its_root)");
      if (p != buf)
	*p = 0;
      else
	*++p = 0;

      if (stat (buf, &st) < 0)
	grub_util_error ("can not stat %s: %s", buf, strerror (errno));

      /* buf is another filesystem; we found it.  */
      if (st.st_dev != num)
	{
	  /* offset == 0 means path given is the mount point.  */
	  if (offset == 0)
	    {
	      free (buf);
	      free (buf2);
	      return strdup ("/");
	    }
	  else
	    break;
	}

      offset = p - buf;
      /* offset == 1 means root directory.  */
      if (offset == 1)
	{
	  free (buf);
	  len = strlen (buf2);
	  while (buf2[len - 1] == '/' && len > 1)
	    {
	      buf2[len - 1] = '\0';
	      len--;
	    }
	  return buf2;
	}
    }
  free (buf);
  buf3 = strdup (buf2 + offset);
  free (buf2);

  len = strlen (buf3);
  while (buf3[len - 1] == '/' && len > 1)
    {
      buf3[len - 1] = '\0';
      len--;
    }

  return buf3;
}
