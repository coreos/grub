/* err.c - error handling routines */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/err.h>
#include <pupa/misc.h>
#include <stdarg.h>

#define PUPA_MAX_ERRMSG	256

pupa_err_t pupa_errno;
char pupa_errmsg[PUPA_MAX_ERRMSG];

pupa_err_t
pupa_error (pupa_err_t n, const char *fmt, ...)
{
  va_list ap;
  
  pupa_errno = n;

  va_start (ap, fmt);
  pupa_vsprintf (pupa_errmsg, fmt, ap);
  va_end (ap);

  return n;
}

void
pupa_fatal (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  pupa_vprintf (fmt, ap);
  va_end (ap);

  pupa_stop ();
}

void
pupa_print_error (void)
{
  if (pupa_errno != PUPA_ERR_NONE)
    pupa_printf ("error: %s\n", pupa_errmsg);
}
