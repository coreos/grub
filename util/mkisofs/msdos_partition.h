/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004,2007  Free Software Foundation, Inc.
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

#ifndef MSDOS_PARTITION_H
#define MSDOS_PARTITION_H	1

#include <stdint.h>

/* The signature.  */
#define MSDOS_PARTITION_SIGNATURE		((0xaa << 8) | 0x55)

/* This is not a flag actually, but used as if it were a flag.  */
#define MSDOS_PARTITION_TYPE_HIDDEN_FLAG	0x10

/* The partition entry.  */
struct msdos_partition_entry
{
  /* If active, 0x80, otherwise, 0x00.  */
  uint8_t flag;

  /* The head of the start.  */
  uint8_t start_head;

  /* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
     is the cylinder of the start. Note that S is counted from one.  */
  uint8_t start_sector;

  /* (C & 0xFF) where C is the cylinder of the start.  */
  uint8_t start_cylinder;

  /* The partition type.  */
  uint8_t type;

  /* The end versions of start_head, start_sector and start_cylinder,
     respectively.  */
  uint8_t end_head;
  uint8_t end_sector;
  uint8_t end_cylinder;

  /* The start sector. Note that this is counted from zero.  */
  uint32_t start;

  /* The length in sector units.  */
  uint32_t length;
} __attribute__ ((packed));

/* The structure of MBR.  */
struct msdos_partition_mbr
{
  /* The code area (actually, including BPB).  */
  uint8_t code[446];

  /* Four partition entries.  */
  struct msdos_partition_entry entries[4];

  /* The signature 0xaa55.  */
  uint16_t signature;
} __attribute__ ((packed));

#endif
