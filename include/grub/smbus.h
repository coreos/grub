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

#ifndef GRUB_SMBUS_HEADER
#define GRUB_SMBUS_HEADER 1

#define GRUB_SMB_RAM_START_ADDR 0x50
#define GRUB_SMB_RAM_NUM_MAX 0x08

struct grub_smbus_spd
{
  grub_uint8_t written_size;
  grub_uint8_t log_total_flash_size;
#define GRUB_SMBUS_SPD_MEMORY_TYPE_DDR2 8
  grub_uint8_t memory_type;
  union
  {
    grub_uint8_t unknown[253];
    struct {
      grub_uint8_t unused1[70];
      grub_uint8_t part_number[18];
      grub_uint8_t unused2[165];
    } ddr2;
  };
};

#endif
