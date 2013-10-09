/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2011,2012,2013  Free Software Foundation, Inc.
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

#ifndef GRUB_EMU_HOSTFILE_H
#define GRUB_EMU_HOSTFILE_H 1

#include <config.h>
#include <stdarg.h>

#include <grub/symbol.h>
#include <grub/types.h>

#if defined (__NetBSD__)
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/"GRUB_DIR_NAME
#else
# define DEFAULT_DIRECTORY	"/"GRUB_BOOT_DIR_NAME"/"GRUB_DIR_NAME
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

typedef int grub_util_fd_t;
#define GRUB_UTIL_FD_INVALID -1
#define GRUB_UTIL_FD_IS_VALID(x) ((x) >= 0)
#define GRUB_UTIL_FD_STAT_IS_FUNCTIONAL 1

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__) || defined (__sun__) || defined(__OpenBSD__)
#define GRUB_DISK_DEVS_ARE_CHAR 1
#else
#define GRUB_DISK_DEVS_ARE_CHAR 0
#endif

#endif
