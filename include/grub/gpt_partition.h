/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_GPT_PARTITION_HEADER
#define GRUB_GPT_PARTITION_HEADER	1

#include <grub/types.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>

struct grub_gpt_part_type
{
  grub_uint32_t data1;
  grub_uint16_t data2;
  grub_uint16_t data3;
  grub_uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct grub_gpt_part_type grub_gpt_part_type_t;

#define GRUB_GPT_PARTITION_TYPE_EMPTY \
  { 0x0, 0x0, 0x0, \
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } \
  }

#define GRUB_GPT_PARTITION_TYPE_BIOS_BOOT \
  { grub_cpu_to_le32_compile_time (0x21686148), \
      grub_cpu_to_le16_compile_time (0x6449), \
      grub_cpu_to_le16_compile_time (0x6e6f),	       \
    { 0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } \
  }

#define GRUB_GPT_PARTITION_TYPE_LDM \
  { grub_cpu_to_le32_compile_time (0x5808C8AAU),\
      grub_cpu_to_le16_compile_time (0x7E8F), \
      grub_cpu_to_le16_compile_time (0x42E0),	       \
	{ 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3 }	\
  }

#define GRUB_GPT_HEADER_MAGIC \
  { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 }

#define GRUB_GPT_HEADER_VERSION	\
  grub_cpu_to_le32_compile_time (0x00010000U)

struct grub_gpt_header
{
  grub_uint8_t magic[8];
  grub_uint32_t version;
  grub_uint32_t headersize;
  grub_uint32_t crc32;
  grub_uint32_t unused1;
  grub_uint64_t header_lba;
  grub_uint64_t alternate_lba;
  grub_uint64_t start;
  grub_uint64_t end;
  grub_uint8_t guid[16];
  grub_uint64_t partitions;
  grub_uint32_t maxpart;
  grub_uint32_t partentry_size;
  grub_uint32_t partentry_crc32;
} GRUB_PACKED;

struct grub_gpt_partentry
{
  grub_gpt_part_type_t type;
  grub_uint8_t guid[16];
  grub_uint64_t start;
  grub_uint64_t end;
  grub_uint64_t attrib;
  char name[72];
} GRUB_PACKED;

/* Basic GPT partmap module.  */
grub_err_t
grub_gpt_partition_map_iterate (grub_disk_t disk,
				grub_partition_iterate_hook_t hook,
				void *hook_data);

/* Advanced GPT library.  */
typedef enum grub_gpt_status
  {
    GRUB_GPT_PROTECTIVE_MBR         = 0x01,
    GRUB_GPT_HYBRID_MBR             = 0x02,
    GRUB_GPT_PRIMARY_HEADER_VALID   = 0x04,
    GRUB_GPT_PRIMARY_ENTRIES_VALID  = 0x08,
    GRUB_GPT_BACKUP_HEADER_VALID    = 0x10,
    GRUB_GPT_BACKUP_ENTRIES_VALID   = 0x20,
  } grub_gpt_status_t;

#define GRUB_GPT_MBR_VALID (GRUB_GPT_PROTECTIVE_MBR|GRUB_GPT_HYBRID_MBR)

/* UEFI requires the entries table to be at least 16384 bytes for a
 * total of 128 entries given the standard 128 byte entry size.  */
#define GRUB_GPT_DEFAULT_ENTRIES_LENGTH	128

struct grub_gpt
{
  /* Bit field indicating which structures on disk are valid.  */
  grub_gpt_status_t status;

  /* Protective or hybrid MBR.  */
  struct grub_msdos_partition_mbr mbr;

  /* Each of the two GPT headers.  */
  struct grub_gpt_header primary;
  struct grub_gpt_header backup;

  /* Only need one entries table, on disk both copies are identical.  */
  struct grub_gpt_partentry *entries;

  /* Logarithm of sector size, in case GPT and disk driver disagree.  */
  unsigned int log_sector_size;
};
typedef struct grub_gpt *grub_gpt_t;

/* Translate GPT sectors to GRUB's 512 byte block addresses.  */
static inline grub_disk_addr_t
grub_gpt_sector_to_addr (grub_gpt_t gpt, grub_uint64_t sector)
{
  return (sector << (gpt->log_sector_size - GRUB_DISK_SECTOR_BITS));
}

/* Allocates and fills new grub_gpt structure, free with grub_gpt_free.  */
grub_gpt_t grub_gpt_read (grub_disk_t disk);

void grub_gpt_free (grub_gpt_t gpt);

grub_err_t grub_gpt_pmbr_check (struct grub_msdos_partition_mbr *mbr);
grub_err_t grub_gpt_header_check (struct grub_gpt_header *gpt,
				  unsigned int log_sector_size);

#endif /* ! GRUB_GPT_PARTITION_HEADER */
