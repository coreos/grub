/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
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

#ifndef PUPA_LOADER_MACHINE_HEADER
#define PUPA_LOADER_MACHINE_HEADER	1

#include <pupa/types.h>
#include <pupa/symbol.h>

/* This is an asm part of the chainloader.  */
void EXPORT_FUNC(pupa_chainloader_real_boot) (int drive, void *part_addr) __attribute__ ((noreturn));

#endif /* ! PUPA_LOADER_MACHINE_HEADER */
