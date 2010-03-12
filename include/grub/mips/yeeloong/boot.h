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

#ifndef GRUB_BOOT_MACHINE_HEADER
#define GRUB_BOOT_MACHINE_HEADER	1

#define GRUB_MACHINE_FLASH_START            0xbfc00000
#define GRUB_MACHINE_FLASH_TLB_REFILL       0xbfc00200
#define GRUB_MACHINE_FLASH_CACHE_ERROR      0xbfc00300
#define GRUB_MACHINE_FLASH_OTHER_EXCEPTION  0xbfc00380

#define GRUB_MACHINE_DDR2_BASE              0xaffffe00
#define GRUB_MACHINE_DDR2_REG1_HI_8BANKS    0x00000001
#define GRUB_MACHINE_DDR2_REG_SIZE          0x8
#define GRUB_MACHINE_DDR2_REG_STEP          0x10

#endif
