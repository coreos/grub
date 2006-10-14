/* lvm.c - LVM support for GRUB utils.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* We only support LVM on Linux.  */
#ifdef __linux__

#include <grub/util/misc.h>

#include <string.h>
#include <sys/stat.h>

int
grub_util_lvm_isvolume (char *name)
{
  char *devname;
  struct stat st;
  int err;
  
  devname = xmalloc (strlen (name) + 13);

  strcpy (devname, "/dev/mapper/");
  strcpy (devname+12, name);

  err = stat (devname, &st);
  free (devname);

  if (err)
    return 0;
  else
    return 1;
}

#endif /* ! __linux__ */
