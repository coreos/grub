/*  openfw.c -- Open firmware support funtions.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/ieee1275.h>

/* Iterate through all device aliasses.  Thisfunction can be used to
   find a device of a specific type.  */
grub_err_t
grub_devalias_iterate (int (*hook) (struct grub_ieee1275_devalias *alias))
{
  grub_ieee1275_phandle_t devalias;
  char aliasname[32];
  int actual;
  struct grub_ieee1275_devalias alias;

  if (grub_ieee1275_finddevice ("/aliases", &devalias))
    return -1;

  /* XXX: Is this the right way to find the first property?  */
  aliasname[0] = '\0';

  /* XXX: Are the while conditions correct?  */
  while (grub_ieee1275_next_property (devalias, aliasname, aliasname, &actual)
	 || actual)
    {
      grub_ieee1275_phandle_t dev;
      grub_size_t pathlen;
      char *devpath;
      /* XXX: This should be large enough for any possible case.  */
      char devtype[64];
  
      grub_ieee1275_get_property_length (devalias, aliasname, &pathlen);
      devpath = grub_malloc (pathlen);
      if (! devpath)
	return grub_errno;

      if (grub_ieee1275_get_property (devalias, aliasname, devpath, pathlen,
				      &actual))
	{
	  grub_free (devpath);
	  continue;
	}

      if (grub_ieee1275_finddevice (devpath, &dev))
	{
	  grub_free (devpath);
	  continue;
	}

      if (grub_ieee1275_get_property (dev, "device_type", devtype, sizeof devtype,
				      &actual))
	{
	  grub_free (devpath);
	  continue;
	}

      alias.name = aliasname;
      alias.path= devpath;
      alias.type = devtype;
      hook (&alias);
      
      grub_free (devpath);
    }

  return 0;
}
