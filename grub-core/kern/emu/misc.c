#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <grub/mm.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/time.h>
#include <grub/emu/misc.h>

int verbosity;

void
grub_util_warn (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: warn:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  fflush (stderr);
}

void
grub_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;

      fprintf (stderr, _("%s: info:"), program_name);
      fprintf (stderr, " ");
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fprintf (stderr, ".\n");
      fflush (stderr);
    }
}

void
grub_util_error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: error:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  exit (1);
}

void *
grub_malloc (grub_size_t size)
{
  return malloc (size);
}

void *
grub_zalloc (grub_size_t size)
{
  void *ret;

  ret = malloc (size);
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
  return realloc (ptr, size);
}

void *
grub_memalign (grub_size_t align, grub_size_t size)
{
  void *p;

#if defined(HAVE_POSIX_MEMALIGN)
  if (align < sizeof (void *))
    align = sizeof (void *);

  else if (align % sizeof (void *))
    grub_fatal ("bad alignment");

  if (posix_memalign (&p, align, size) != 0)
    p = 0;
#elif defined(HAVE_MEMALIGN)
  p = memalign (align, size);
#else
  (void) align;
  (void) size;
  grub_fatal ("grub_memalign is not supported");
#endif

  if (! p)
    grub_fatal ("out of memory");

  return p;
}

void *
xmalloc (grub_size_t size)
{
  void *p;

  p = grub_malloc (size);
  if (! p)
    grub_fatal ("out of memory");

  return p;
}

void *
xrealloc (void *ptr, grub_size_t size)
{
  ptr = grub_realloc (ptr, size);
  if (! ptr)
    grub_fatal ("out of memory");

  return ptr;
}

char *
xstrdup (const char *str)
{
  size_t len;
  char *newstr;

  len = grub_strlen (str);
  newstr = (char *) xmalloc (len + 1);
  grub_memcpy (newstr, str, len + 1);

  return newstr;
}

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

void
grub_exit (void)
{
  exit (1);
}

grub_uint64_t
grub_get_time_ms (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
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
