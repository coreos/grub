/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003 Yoshinori K. Okuji <okuji@enbug.org>
 *  Copyright (C) 2003 Marco Gerards <metgerards@student.han.nl>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>

#include <pupa/util/misc.h>
#include <pupa/mm.h>
#include <pupa/term.h>

char *progname = 0;
int verbosity = 0;

void
pupa_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;
      
      fprintf (stderr, "%s: info: ", progname);
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fputc ('\n', stderr);
    }
}

void
pupa_util_error (const char *fmt, ...)
{
  va_list ap;
  
  fprintf (stderr, "%s: error: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

void *
xmalloc (size_t size)
{
  void *p;
  
  p = malloc (size);
  if (! p)
    pupa_util_error ("out of memory");

  return p;
}

void *
xrealloc (void *ptr, size_t size)
{
  ptr = realloc (ptr, size);
  if (! ptr)
    pupa_util_error ("out of memory");

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
pupa_util_get_path (const char *dir, const char *file)
{
  char *path;
  
  path = (char *) xmalloc (strlen (dir) + 1 + strlen (file) + 1);
  sprintf (path, "%s/%s", dir, file);
  return path;
}

size_t
pupa_util_get_image_size (const char *path)
{
  struct stat st;
  
  pupa_util_info ("getting the size of %s", path);
  
  if (stat (path, &st) == -1)
    pupa_util_error ("cannot stat %s", path);
  
  return st.st_size;
}

char *
pupa_util_read_image (const char *path)
{
  char *img;
  FILE *fp;
  size_t size;
  
  pupa_util_info ("reading %s", path);

  size = pupa_util_get_image_size (path);
  img = (char *) xmalloc (size);

  fp = fopen (path, "rb");
  if (! fp)
    pupa_util_error ("cannot open %s", path);
  
  if (fread (img, 1, size, fp) != size)
    pupa_util_error ("cannot read %s", path);

  fclose (fp);
  
  return img;
}

void
pupa_util_load_image (const char *path, char *buf)
{
  FILE *fp;
  size_t size;
  
  pupa_util_info ("reading %s", path);

  size = pupa_util_get_image_size (path);
  
  fp = fopen (path, "rb");
  if (! fp)
    pupa_util_error ("cannot open %s", path);

  if (fread (buf, 1, size, fp) != size)
    pupa_util_error ("cannot read %s", path);

  fclose (fp);
}

void
pupa_util_write_image (const char *img, size_t size, FILE *out)
{
  pupa_util_info ("writing 0x%x bytes", size);
  if (fwrite (img, 1, size, out) != size)
    pupa_util_error ("write failed");
}

void *
pupa_malloc (unsigned size)
{
  return malloc (size);
}

void
pupa_free (void *ptr)
{
  free (ptr);
}

void *
pupa_realloc (void *ptr, unsigned size)
{
  return realloc (ptr, size);
}

void
pupa_stop (void)
{
  exit (1);
}

void
pupa_putchar (int c)
{
  putchar (c);
}

pupa_uint32_t
pupa_get_rtc (void)
{
  struct tms currtime;

  return times (&currtime);
}
