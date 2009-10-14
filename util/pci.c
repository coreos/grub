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

#include <grub/pci.h>
#include <grub/dl.h>

struct pci_access *acc = 0;

grub_pci_address_t
grub_pci_make_address (grub_pci_device_t dev, int reg)
{
  grub_pci_address_t ret;
  ret.dev = dev;
  ret.pos = reg << 2;
  return ret;
}

void
grub_pci_close (grub_pci_device_t dev)
{
  pci_free_dev (dev);
}

void
grub_pci_iterate (grub_pci_iteratefunc_t hook)
{
  grub_pci_device_t cur;
  for (cur = acc->devices; cur; cur = cur->next)
    hook (cur, cur->vendor_id|(cur->device_id << 16));
}

GRUB_MOD_INIT (pci)
{
  acc = pci_alloc ();
  pci_init (acc);
}

GRUB_MOD_FINI (pci)
{
  pci_cleanup (acc);
}
