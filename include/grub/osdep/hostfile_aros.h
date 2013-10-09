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

struct grub_util_fd
{
  enum { GRUB_UTIL_FD_FILE, GRUB_UTIL_FD_DISK } type;
  grub_uint64_t off;
  union
  {
    int fd;
    struct {
      struct IOExtTD *ioreq;
      struct MsgPort *mp;
      unsigned int is_floppy:1;
      unsigned int is_64:1;
    };
  };
};
typedef struct grub_util_fd *grub_util_fd_t;

#define GRUB_UTIL_FD_INVALID NULL
#define GRUB_UTIL_FD_IS_VALID(x) ((x) != GRUB_UTIL_FD_INVALID)
#define GRUB_UTIL_FD_STAT_IS_FUNCTIONAL 0

#define DEFAULT_DIRECTORY	"SYS:" GRUB_BOOT_DIR_NAME "/" GRUB_DIR_NAME
#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

#endif
