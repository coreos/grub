/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

#ifndef	GRUB_PCIUTILS_H
#define	GRUB_PCIUTILS_H	1

#include <pci/pci.h>

typedef struct pci_dev *grub_pci_device_t;

static inline int
grub_pci_get_bus (grub_pci_device_t dev)
{
  return dev->bus;
}

static inline int
grub_pci_get_device (grub_pci_device_t dev)
{
  return dev->dev;
}

static inline int
grub_pci_get_function (grub_pci_device_t dev)
{
  return dev->func;
}

struct grub_pci_address
{
  grub_pci_device_t dev;
  int pos;
};

typedef struct grub_pci_address grub_pci_address_t;

static inline grub_uint32_t
grub_pci_read (grub_pci_address_t addr)
{
  return pci_read_long (addr.dev, addr.pos);
}

static inline grub_uint16_t
grub_pci_read_word (grub_pci_address_t addr)
{
  return pci_read_word (addr.dev, addr.pos);
}

static inline grub_uint8_t
grub_pci_read_byte (grub_pci_address_t addr)
{
  return pci_read_byte (addr.dev, addr.pos);
}

static inline void
grub_pci_write (grub_pci_address_t addr, grub_uint32_t data)
{
  pci_write_long (addr.dev, addr.pos, data);
}

static inline void
grub_pci_write_word (grub_pci_address_t addr, grub_uint16_t data)
{
  pci_write_word (addr.dev, addr.pos, data);
}

static inline void
grub_pci_write_byte (grub_pci_address_t addr, grub_uint8_t data)
{
  pci_write_byte (addr.dev, addr.pos, data);
}

#endif /* GRUB_PCIUTILS_H */
