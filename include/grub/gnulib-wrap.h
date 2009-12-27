/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_GNULIB_WRAP_H
#define GRUB_GNULIB_WRAP_H	1

#include <grub/mm.h>
#include <grub/misc.h>

typedef grub_size_t size_t;
typedef int bool;
static const bool true = 1;
static const bool false = 0;

#define assert(x) assert_real(__FILE__, __LINE__, x)

static inline void
assert_real (const char *file, int line, int cond)
{
  if (!cond)
    grub_fatal ("Assertion failed at %s:%d\n", file, line);
}

static inline char *
locale_charset (void)
{
  return "UTF-8";
}

#define MB_CUR_MAX 6

static inline void *
realloc (void *ptr, grub_size_t size)
{
  return grub_realloc (ptr, size);
}

static inline int
toupper (int c)
{
  return grub_toupper (c);
}

static inline int 
isspace (int c)
{
  return grub_isspace (c);
}

static inline int 
isdigit (int c)
{
  return grub_isdigit (c);
}

static inline int
islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
isxdigit (int c)
{
  return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')
    || (c >= '0' && c <= '9');
}

static inline int 
isprint (int c)
{
  return grub_isprint (c);
}

static inline int 
iscntrl (int c)
{
  return !grub_isprint (c);
}

static inline int 
isgraph (int c)
{
  return grub_isprint (c) && !grub_isspace (c);
}

static inline int
isalnum (int c)
{
  return grub_isalpha (c) || grub_isdigit (c);
}

static inline int 
ispunct (int c)
{
  return grub_isprint (c) && !grub_isspace (c) && !isalnum (c);
}

static inline int 
isalpha (int c)
{
  return grub_isalpha (c);
}

static inline int
tolower (int c)
{
  return grub_tolower (c);
}

static inline grub_size_t
strlen (const char *s)
{
  return grub_strlen (s);
}

static inline int 
strcmp (const char *s1, const char *s2)
{
  return grub_strcmp (s1, s2);
}

static inline void
abort (void)
{
  grub_abort ();
}

static inline void 
free (void *ptr)
{
  grub_free (ptr);
}

static inline void *
malloc (grub_size_t size)
{
  return grub_malloc (size);
}

static inline void *
calloc (grub_size_t size, grub_size_t nelem)
{
  return grub_zalloc (size * nelem);
}

#define ULONG_MAX GRUB_ULONG_MAX
#define UCHAR_MAX 0xff

/* UCS-4.  */
typedef grub_uint32_t wchar_t;

#undef __libc_lock_init
#define __libc_lock_init(x)
#undef __libc_lock_lock
#define __libc_lock_lock(x)
#undef __libc_lock_unlock
#define __libc_lock_unlock(x)

#endif
