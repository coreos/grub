/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002, 2004  Free Software Foundation, Inc.
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

#ifndef PUPA_TYPES_CPU_HEADER
#define PUPA_TYPES_CPU_HEADER	1

/* The size of void *.  */
#define PUPA_HOST_SIZEOF_VOID_P	4

/* The size of long.  */
#define PUPA_HOST_SIZEOF_LONG	4

/* powerpc is little-endian.  */
#undef PUPA_HOST_WORDS_LITTLEENDIAN

#endif /* ! PUPA_TYPES_CPU_HEADER */
