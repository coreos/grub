/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PUPA_PARTITION_HEADER
#define PUPA_PARTITION_HEADER	1

#include <pupa/symbol.h>
#include <pupa/types.h>
#include <pupa/err.h>

#define PUPA_APPLE_PART_MAGIC	0x504D

struct pupa_apple_part
{
  /* The magic number to idenify this as a partition, it should have
     the value `0x504D'.  */
  pupa_uint16_t magic;

  /* Reserved.  */
  pupa_uint16_t reserved;

  /* The size of the partition map in blocks.  */
  pupa_uint32_t partmap_size;

  /* The first physical block of the partition.  */
  pupa_uint32_t first_phys_block;

  /* The amount of blocks.  */
  pupa_uint32_t blockcnt;

  /* The partition name.  */
  char partname[32];

  /* The partition type.  */
  char parttype[32];

  /* The first datablock of the partition.  */
  pupa_uint32_t datablocks_first;

  /* The amount datablocks.  */
  pupa_uint32_t datablocks_count;

  /* The status of the partition. (???)  */
  pupa_uint32_t status;

  /* The first block on which the bootcode can be found.  */
  pupa_uint32_t bootcode_pos;

  /* The size of the bootcode in bytes.  */
  pupa_uint32_t bootcode_size;

  /* The load address of the bootcode.  */
  pupa_uint32_t bootcode_loadaddr;

  /* Reserved.  */
  pupa_uint32_t reserved2;
  
  /* The entrypoint of the bootcode.  */
  pupa_uint32_t bootcode_entrypoint;

  /* Reserved.  */
  pupa_uint32_t reserved3;

  /* A checksum of the bootcode.  */
  pupa_uint32_t bootcode_checksum;

  /* The processor type.  */
  char processor[16];

  /* Padding.  */
  pupa_uint16_t pad[187];
};

/* Partition description.  */
struct pupa_partition
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
typedef struct pupa_partition *pupa_partition_t;

struct pupa_disk;

pupa_partition_t EXPORT_FUNC(pupa_partition_probe) (struct pupa_disk *disk,
						    const char *str);
pupa_err_t EXPORT_FUNC(pupa_partition_iterate) (struct pupa_disk *disk,
						int (*hook) (const pupa_partition_t partition));
char *EXPORT_FUNC(pupa_partition_get_name) (const pupa_partition_t partition);


static inline unsigned long
pupa_partition_get_start (const pupa_partition_t p)
{
  return p->start;
}

static inline unsigned long
pupa_partition_get_len (const pupa_partition_t p)
{
  return p->len;
}

#endif /* ! PUPA_PARTITION_HEADER */
