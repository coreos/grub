/*  openfw.c -- Open firmware support funtions.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
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

#include <pupa/err.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/machine/ieee1275.h>

/* Iterate through all device aliasses.  Thisfunction can be used to
   find a device of a specific type.  */
pupa_err_t
pupa_devalias_iterate (int (*hook) (struct pupa_ieee1275_devalias *alias))
{
  pupa_ieee1275_phandle_t devalias;
  char aliasname[32];
  int actual;
  struct pupa_ieee1275_devalias alias;

  if (pupa_ieee1275_finddevice ("/aliases", &devalias))
    return -1;

  /* XXX: Is this the right way to find the first property?  */
  aliasname[0] = '\0';

  /* XXX: Are the while conditions correct?  */
  while (pupa_ieee1275_next_property (devalias, aliasname, aliasname, &actual)
	 || actual)
    {
      pupa_ieee1275_phandle_t dev;
      pupa_size_t pathlen;
      char *devpath;
      /* XXX: This should be large enough for any possible case.  */
      char devtype[64];
  
      pupa_ieee1275_get_property_length (devalias, aliasname, &pathlen);
      devpath = pupa_malloc (pathlen);
      if (! devpath)
	return pupa_errno;

      if (pupa_ieee1275_get_property (devalias, aliasname, devpath, pathlen,
				      &actual))
	{
	  pupa_free (devpath);
	  continue;
	}

      if (pupa_ieee1275_finddevice (devpath, &dev))
	{
	  pupa_free (devpath);
	  continue;
	}

      if (pupa_ieee1275_get_property (dev, "device_type", devtype, sizeof devtype,
				      &actual))
	{
	  pupa_free (devpath);
	  continue;
	}

      alias.name = aliasname;
      alias.path= devpath;
      alias.type = devtype;
      hook (&alias);
      
      pupa_free (devpath);
    }

  return 0;
}
