/* biosdisk.h - emulate biosdisk */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef PUPA_BIOSDISK_MACHINE_UTIL_HEADER
#define PUPA_BIOSDISK_MACHINE_UTIL_HEADER	1

void pupa_util_biosdisk_init (const char *dev_map);
void pupa_util_biosdisk_fini (void);
char *pupa_util_biosdisk_get_pupa_dev (const char *os_dev);

#endif /* ! PUPA_BIOSDISK_MACHINE_UTIL_HEADER */
