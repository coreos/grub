#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <grub/list.h>
#include <grub/test.h>
#include <grub/handler.h>

void *
grub_test_malloc (grub_size_t size)
{
  return malloc (size);
}

void
grub_test_free (void *ptr)
{
  free (ptr);
}

int
grub_test_vsprintf (char *str, const char *fmt, va_list args)
{
  return vsprintf (str, fmt, args);
}

char *
grub_test_strdup (const char *str)
{
  return strdup (str);
}

int
grub_test_printf (const char *fmt, ...)
{
  int r;
  va_list ap;

  va_start (ap, fmt);
  r = vprintf (fmt, ap);
  va_end (ap);

  return r;
}

int
main (int argc __attribute__ ((unused)),
      char *argv[] __attribute__ ((unused)))
{
  int status = 0;

  extern void grub_unit_test_init (void);
  extern void grub_unit_test_fini (void);

  auto int run_test (grub_test_t test);
  int run_test (grub_test_t test)
  {
    status = grub_test_run (test->name) ? : status;
    return 0;
  }

  grub_unit_test_init ();
  grub_list_iterate (GRUB_AS_LIST (grub_test_list),
		     (grub_list_hook_t) run_test);
  grub_unit_test_fini ();

  exit (status);
}

/* Other misc. functions necessary for successful linking.  */

char *
grub_env_get (const char *name __attribute__ ((unused)))
{
  return NULL;
}

grub_err_t
grub_error (grub_err_t n, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  return n;
}

void *
grub_malloc (grub_size_t size)
{
  return malloc (size);
}

void
grub_refresh (void)
{
  fflush (stdout);
}

void
grub_putchar (int c)
{
  putchar (c);
}

int
grub_getkey (void)
{
  return -1;
}

void
grub_exit (void)
{
  exit (1);
}

struct grub_handler_class grub_term_input_class;
struct grub_handler_class grub_term_output_class;
