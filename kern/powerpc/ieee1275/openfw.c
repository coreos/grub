/*  openfw.c -- Open firmware support funtions.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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

/* Walk children of 'devpath', calling hook for each.  */
grub_err_t
grub_children_iterate (char *devpath,
		  int (*hook) (struct grub_ieee1275_devalias *alias))
{
  grub_ieee1275_phandle_t dev;
  grub_ieee1275_phandle_t child;

  grub_ieee1275_finddevice (devpath, &dev);
  if (dev == -1)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Unknown device");

  grub_ieee1275_child (dev, &child);
  if (child == -1)
    return grub_error (GRUB_ERR_BAD_DEVICE, "Device has no children");

  do
    {
      /* XXX: Don't use hardcoded path lengths.  */
      char childtype[64];
      char childpath[64];
      char childname[64];
      char fullname[64];
      struct grub_ieee1275_devalias alias;
      int actual;

      grub_ieee1275_get_property (child, "device_type", &childtype,
				  sizeof childtype, &actual);
      if (actual == -1)
	continue;

      grub_ieee1275_package_to_path (child, childpath, sizeof childpath,
      				     &actual);
      if (actual == -1)
	continue;

      grub_ieee1275_get_property (child, "name", &childname,
				  sizeof childname, &actual);
      if (actual == -1)
	continue;

      grub_sprintf(fullname, "%s/%s", devpath, childname);

      alias.type = childtype;
      alias.path = childpath;
      alias.name = fullname;
      hook (&alias);
    }
  while (grub_ieee1275_peer (child, &child));

  return 0;
}

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

      /* The property `name' is a special case we should skip.  */
      if (!grub_strcmp (aliasname, "name"))
	  continue;
      
      devpath = grub_malloc (pathlen);
      if (! devpath)
	return grub_errno;

      if (grub_ieee1275_get_property (devalias, aliasname, devpath, pathlen,
				      &actual))
	{
	  grub_free (devpath);
	  continue;
	}
      
      if (grub_ieee1275_finddevice (devpath, &dev) || dev == -1)
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

/* Call the "map" method of /chosen/mmu.  */
static int
grub_map (grub_addr_t phys, grub_addr_t virt, grub_uint32_t size,
		   grub_uint8_t mode)
{
  struct map_args {
    struct grub_ieee1275_common_hdr common;
    char *method;
    grub_ieee1275_ihandle_t ihandle;
    grub_uint32_t mode;
    grub_uint32_t size;
    grub_uint32_t virt;
    grub_uint32_t phys;
    int catch_result;
  } args;
  grub_ieee1275_ihandle_t mmu;
  grub_ieee1275_ihandle_t chosen;
  int len;

  grub_ieee1275_finddevice ("/chosen", &chosen);
  if (chosen == 0)
    return -1;

  grub_ieee1275_get_property (chosen, "mmu", &mmu, sizeof mmu, &len);
  if (len != sizeof mmu)
    return -1;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = "map";
  args.ihandle = mmu;
  args.phys = phys;
  args.virt = virt;
  args.size = size;
  args.mode = mode; /* Format is WIMG0PP.  */

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return args.catch_result;
}

int
grub_claimmap (grub_addr_t addr, grub_size_t size)
{
  if (grub_ieee1275_claim (addr, size, 0, 0))
    return -1;

  if ((! grub_ieee1275_realmode) && grub_map (addr, addr, size, 0x00))
    {
      grub_printf ("map failed: address 0x%x, size 0x%x\n", addr, size);
      grub_ieee1275_release (addr, size);
      return -1;
    }

  return 0;
}
