/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PUPA_SETJMP_HEADER
#define PUPA_SETJMP_HEADER	1

#ifdef PUPA_UTIL
#include <setjmp.h>
typedef jmp_buf pupa_jmp_buf;
#define pupa_setjmp setjmp
#define pupa_longjmp longjmp
#else
/* This must define pupa_jmp_buf.  */
#include <pupa/cpu/setjmp.h>

int pupa_setjmp (pupa_jmp_buf env);
void pupa_longjmp (pupa_jmp_buf env, int val) __attribute__ ((noreturn));
#endif

#endif /* ! PUPA_SETJMP_HEADER */
