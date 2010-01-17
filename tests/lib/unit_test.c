/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <grub/list.h>
#include <grub/test.h>
#include <grub/handler.h>

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
    status = grub_test_run (test) ? : status;
    return 0;
  }

  grub_unit_test_init ();
  grub_list_iterate (GRUB_AS_LIST (grub_test_list),
		     (grub_list_hook_t) run_test);
  grub_unit_test_fini ();

  exit (status);
}

/* Other misc. functions necessary for successful linking.  */

void
grub_free (void *ptr)
{
  free (ptr);
}

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
