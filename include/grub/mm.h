/* mm.h - prototypes and declarations for memory manager */
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

#ifndef PUPA_MM_H
#define PUPA_MM_H	1

#include <pupa/types.h>
#include <pupa/symbol.h>

void pupa_mm_init_region (void *addr, unsigned size);
void *EXPORT_FUNC(pupa_malloc) (unsigned size);
void EXPORT_FUNC(pupa_free) (void *ptr);
void *EXPORT_FUNC(pupa_realloc) (void *ptr, unsigned size);
void *EXPORT_FUNC(pupa_memalign) (unsigned align, unsigned size);

/* For debugging.  */
#define MM_DEBUG	1
#if MM_DEBUG
void pupa_mm_dump (unsigned lineno);
#endif

#endif /* ! PUPA_MM_H */
