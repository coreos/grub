/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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

#ifndef PUPA_UTIL_MISC_HEADER
#define PUPA_UTIL_MISC_HEADER	1

#include <stdlib.h>
#include <stdio.h>

extern char *progname;
extern int verbosity;

void pupa_util_info (const char *fmt, ...);
void pupa_util_error (const char *fmt, ...) __attribute__ ((noreturn));

void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstrdup (const char *str);

char *pupa_util_get_path (const char *dir, const char *file);
size_t pupa_util_get_image_size (const char *path);
char *pupa_util_read_image (const char *path);
void pupa_util_load_image (const char *path, char *buf);
void pupa_util_write_image (const char *img, size_t size, FILE *out);

#endif /* ! PUPA_UTIL_MISC_HEADER */
