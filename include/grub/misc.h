/* misc.h - prototypes for misc functions */
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

#ifndef PUPA_MISC_HEADER
#define PUPA_MISC_HEADER	1

#include <stdarg.h>
#include <pupa/types.h>
#include <pupa/symbol.h>

/* XXX: If pupa_memmove is too slow, we must implement pupa_memcpy.  */
#define pupa_memcpy(d,s,n)	pupa_memmove ((d), (s), (n))

void *EXPORT_FUNC(pupa_memmove) (void *dest, const void *src, pupa_size_t n);
char *EXPORT_FUNC(pupa_strcpy) (char *dest, const char *src);
int EXPORT_FUNC(pupa_memcmp) (const void *s1, const void *s2, pupa_size_t n);
int EXPORT_FUNC(pupa_strcmp) (const char *s1, const char *s2);
char *EXPORT_FUNC(pupa_strchr) (const char *s, int c);
char *EXPORT_FUNC(pupa_strrchr) (const char *s, int c);
int EXPORT_FUNC(pupa_isspace) (int c);
int EXPORT_FUNC(pupa_isprint) (int c);
int EXPORT_FUNC(pupa_isalpha) (int c);
int EXPORT_FUNC(pupa_tolower) (int c);
unsigned long EXPORT_FUNC(pupa_strtoul) (const char *str, char **end, int base);
char *EXPORT_FUNC(pupa_strdup) (const char *s);
void *EXPORT_FUNC(pupa_memset) (void *s, int c, pupa_size_t n);
pupa_size_t EXPORT_FUNC(pupa_strlen) (const char *s);
int EXPORT_FUNC(pupa_printf) (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
int EXPORT_FUNC(pupa_vprintf) (const char *fmt, va_list args);
int EXPORT_FUNC(pupa_sprintf) (char *str, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int EXPORT_FUNC(pupa_vsprintf) (char *str, const char *fmt, va_list args);
void EXPORT_FUNC(pupa_stop) (void) __attribute__ ((noreturn));

#endif /* ! PUPA_MISC_HEADER */
