/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_PARTITION_HEADER
#define GRUB_PARTITION_HEADER	1

#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/err.h>

#define GRUB_APPLE_PART_MAGIC	0x504D

struct grub_apple_part
{
  /* The magic number to idenify this as a partition, it should have
     the value `0x504D'.  */
  grub_uint16_t magic;

  /* Reserved.  */
  grub_uint16_t reserved;

  /* The size of the partition map in blocks.  */
  grub_uint32_t partmap_size;

  /* The first physical block of the partition.  */
  grub_uint32_t first_phys_block;

  /* The amount of blocks.  */
  grub_uint32_t blockcnt;

  /* The partition name.  */
  char partname[32];

  /* The partition type.  */
  char parttype[32];

  /* The first datablock of the partition.  */
  grub_uint32_t datablocks_first;

  /* The amount datablocks.  */
  grub_uint32_t datablocks_count;

  /* The status of the partition. (???)  */
  grub_uint32_t status;

  /* The first block on which the bootcode can be found.  */
  grub_uint32_t bootcode_pos;

  /* The size of the bootcode in bytes.  */
  grub_uint32_t bootcode_size;

  /* The load address of the bootcode.  */
  grub_uint32_t bootcode_loadaddr;

  /* Reserved.  */
  grub_uint32_t reserved2;
  
  /* The entrypoint of the bootcode.  */
  grub_uint32_t bootcode_entrypoint;

  /* Reserved.  */
  grub_uint32_t reserved3;

  /* A checksum of the bootcode.  */
  grub_uint32_t bootcode_checksum;

  /* The processor type.  */
  char processor[16];

  /* Padding.  */
  grub_uint16_t pad[187];
};

/* Partition description.  */
struct grub_partition
{
  /* The start sector.  */
  unsigned long start;

  /* The length in sector units.  */
  unsigned long len;

  /* The offset of the partition table.  */
  unsigned long offset;

  /* The index of this partition in the partition table.  */
  int index;
  
  /* The DOS partition number.  */
  int dos_part;

  /* The BSD partition number (a == 0).  */
  int bsd_part;

  /* The DOS partition type.  */
  int dos_type;

  /* The BSD partition type.  */
  int bsd_type;
};
typedef struct grub_partition *grub_partition_t;

struct grub_disk;

grub_partition_t EXPORT_FUNC(grub_partition_probe) (struct grub_disk *disk,
						    const char *str);
grub_err_t EXPORT_FUNC(grub_partition_iterate) (struct grub_disk *disk,
						int (*hook) (const grub_partition_t partition));
char *EXPORT_FUNC(grub_partition_get_name) (const grub_partition_t partition);


static inline unsigned long
grub_partition_get_start (const grub_partition_t p)
{
  return p->start;
}

static inline unsigned long
grub_partition_get_len (const grub_partition_t p)
{
  return p->len;
}

#endif /* ! GRUB_PARTITION_HEADER */
