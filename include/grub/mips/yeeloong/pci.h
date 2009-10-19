/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#ifndef	GRUB_MACHINE_PCI_H
#define	GRUB_MACHINE_PCI_H	1

#include <grub/types.h>
#include <grub/cpu/io.h>

#define GRUB_MACHINE_PCI_IOSPACE        0xbfe80000
#define GRUB_MACHINE_PCI_CONTROL_REG    (*(grub_uint32_t *) 0x1fe00118)


static inline grub_uint32_t
grub_pci_read (grub_pci_address_t addr)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  return *(grub_uint32_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff));
}

static inline grub_uint16_t
grub_pci_read_word (grub_pci_address_t addr)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  return *(grub_uint16_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff));
}

static inline grub_uint8_t
grub_pci_read_byte (grub_pci_address_t addr)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  return *(grub_uint8_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff));
}

static inline void
grub_pci_write (grub_pci_address_t addr, grub_uint32_t data)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  *(grub_uint32_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff)) = data;
}

static inline void
grub_pci_write_word (grub_pci_address_t addr, grub_uint16_t data)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  *(grub_uint16_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff)) = data;
}

static inline void
grub_pci_write_byte (grub_pci_address_t addr, grub_uint8_t data)
{
  GRUB_MACHINE_PCI_CONTROL_REG = addr >> 16;
  *(grub_uint8_t *) (GRUB_PCI_IOSPACE | (addr & 0xffff)) = data;
}

#endif /* GRUB_MACHINE_PCI_H */
