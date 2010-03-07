/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_CS5536_HEADER
#define GRUB_CS5536_HEADER 1

#ifndef ASM_FILE
#include <grub/pci.h>
#include <grub/err.h>
#include <grub/smbus.h>
#endif

#define GRUB_CS5536_PCIID 0x208f1022
#define GRUB_CS5536_MSR_MAILBOX_ADDR  0xf4
#define GRUB_CS5536_MSR_MAILBOX_DATA0 0xf8
#define GRUB_CS5536_MSR_MAILBOX_DATA1 0xfc
#define GRUB_CS5536_MSR_SMB_BAR 0x8000000b
#define GRUB_CS5536_MSR_GPIO_BAR 0x8000000c
#define GRUB_CS5536_SMB_REG_DATA 0x0
#define GRUB_CS5536_SMB_REG_STATUS 0x1
#define GRUB_CS5536_SMB_REG_STATUS_SDAST (1 << 6)
#define GRUB_CS5536_SMB_REG_STATUS_BER (1 << 5)
#define GRUB_CS5536_SMB_REG_STATUS_NACK (1 << 4)
#define GRUB_CS5536_SMB_REG_CTRL1 0x3
#define GRUB_CS5536_SMB_REG_CTRL1_START 0x01
#define GRUB_CS5536_SMB_REG_CTRL1_STOP  0x02
#define GRUB_CS5536_SMB_REG_CTRL1_ACK   0x10
#define GRUB_CS5536_SMB_REG_ADDR 0x4
#define GRUB_CS5536_SMB_REG_ADDR_MASTER 0x0
#define GRUB_CS5536_SMB_REG_CTRL2 0x5
#define GRUB_CS5536_SMB_REG_CTRL2_ENABLE 0x1
#define GRUB_CS5536_SMB_REG_CTRL3 0x6

#define GRUB_CS5536_LBAR_ADDR_MASK 0x000000000000fff8ULL
#define GRUB_CS5536_LBAR_ENABLE 0x0000000100000000ULL

/* PMON-compatible LBARs.  */
#define GRUB_CS5536_LBAR_GPIO      0x0b000
#define GRUB_CS5536_LBAR_SMBUS     0x0b390

#define GRUB_GPIO_SMBUS_PINS ((1 << 14) | (1 << 15))
#define GRUB_GPIO_REG_OUT_EN 0x4
#define GRUB_GPIO_REG_OUT_AUX1 0x10
#define GRUB_GPIO_REG_IN_EN 0x20
#define GRUB_GPIO_REG_IN_AUX1 0x34

#ifndef ASM_FILE
int EXPORT_FUNC (grub_cs5536_find) (grub_pci_device_t *devp);

grub_uint64_t grub_cs5536_read_msr (grub_pci_device_t dev, grub_uint32_t addr);
void grub_cs5536_write_msr (grub_pci_device_t dev, grub_uint32_t addr,
			    grub_uint64_t val);
grub_err_t grub_cs5536_read_spd_byte (grub_port_t smbbase, grub_uint8_t dev,
				      grub_uint8_t addr, grub_uint8_t *res);
grub_err_t EXPORT_FUNC (grub_cs5536_read_spd) (grub_port_t smbbase,
					       grub_uint8_t dev,
					       struct grub_smbus_spd *res);
grub_err_t grub_cs5536_smbus_wait (grub_port_t smbbase);
grub_err_t EXPORT_FUNC (grub_cs5536_init_smbus) (grub_pci_device_t dev,
						 grub_uint16_t divisor,
						 grub_port_t *smbbase);
#endif

#endif
