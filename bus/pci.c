/* pci.c - Generic PCI interfaces.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/pci.h>

grub_pci_address_t
grub_pci_make_address (grub_pci_device_t dev, int reg)
{
  return (1 << 31) | (dev.bus << 16) | (dev.device << 11)
    | (dev.function << 8) | (reg << 2);
}

void
grub_pci_iterate (grub_pci_iteratefunc_t hook)
{
  grub_pci_device_t dev;
  grub_pci_address_t addr;
  grub_pci_id_t id;
  grub_uint32_t hdr;

  for (dev.bus = 0; dev.bus < GRUB_PCI_NUM_BUS; dev.bus++)
    {
      for (dev.device = 0; dev.device < GRUB_PCI_NUM_DEVICES; dev.device++)
	{
	  for (dev.function = 0; dev.function < 8; dev.function++)
	    {
	      addr = grub_pci_make_address (dev, 0);
	      id = grub_pci_read (addr);

	      /* Check if there is a device present.  */
	      if (id >> 16 == 0xFFFF)
		continue;

	      if (hook (dev, id))
		return;

	      /* Probe only func = 0 if the device if not multifunction */
	      if (dev.function == 0)
		{
		  addr = grub_pci_make_address (dev, 3);
		  hdr = grub_pci_read (addr);
		  if (!(hdr & 0x800000))
		    break;
		}
	    }
	}
    }
}
