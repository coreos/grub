/* gpt.c - Read/Verify/Write GUID Partition Tables (GPT).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007,2008  Free Software Foundation, Inc.
 *  Copyright (C) 2014 CoreOS, Inc.
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

#include <grub/crypto.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/msdos_partition.h>
#include <grub/gpt_partition.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_uint8_t grub_gpt_magic[] = GRUB_GPT_HEADER_MAGIC;


static grub_err_t
grub_gpt_header_crc32 (struct grub_gpt_header *gpt, grub_uint32_t *crc)
{
  grub_uint8_t *crc32_context;
  grub_uint32_t old;

  crc32_context = grub_zalloc (GRUB_MD_CRC32->contextsize);
  if (!crc32_context)
    return grub_errno;

  /* crc32 must be computed with the field cleared.  */
  old = gpt->crc32;
  gpt->crc32 = 0;
  GRUB_MD_CRC32->init (crc32_context);
  GRUB_MD_CRC32->write (crc32_context, gpt, sizeof (*gpt));
  GRUB_MD_CRC32->final (crc32_context);
  gpt->crc32 = old;

  /* GRUB_MD_CRC32 always uses big endian, gpt is always little.  */
  *crc = grub_swap_bytes32 (*(grub_uint32_t *)
			    GRUB_MD_CRC32->read (crc32_context));

  grub_free (crc32_context);

  return GRUB_ERR_NONE;
}

/* Make sure the MBR is a protective MBR and not a normal MBR.  */
grub_err_t
grub_gpt_pmbr_check (struct grub_msdos_partition_mbr *mbr)
{
  unsigned int i;

  if (mbr->signature !=
      grub_cpu_to_le16_compile_time (GRUB_PC_PARTITION_SIGNATURE))
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid MBR signature");

  for (i = 0; i < sizeof (mbr->entries); i++)
    if (mbr->entries[i].type == GRUB_PC_PARTITION_TYPE_GPT_DISK)
      return GRUB_ERR_NONE;

  return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid protective MBR");
}

grub_err_t
grub_gpt_header_check (struct grub_gpt_header *gpt,
		       unsigned int log_sector_size)
{
  grub_uint32_t crc = 0, size;

  if (grub_memcmp (gpt->magic, grub_gpt_magic, sizeof (grub_gpt_magic)) != 0)
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT signature");

  if (gpt->version != GRUB_GPT_HEADER_VERSION)
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "unknown GPT version");

  if (grub_gpt_header_crc32 (gpt, &crc))
    return grub_errno;

  if (gpt->crc32 != crc)
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT header crc32");

  /* The header size must be between 92 and the sector size.  */
  size = grub_le_to_cpu32 (gpt->headersize);
  if (size < 92U || size > (1U << log_sector_size))
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT header size");

  /* The partition entry size must be a multiple of 128.  */
  size = grub_le_to_cpu32 (gpt->partentry_size);
  if (size < 128 || size % 128)
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT entry size");

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_gpt_read_primary (grub_disk_t disk, grub_gpt_t gpt)
{
  grub_disk_addr_t addr;

  /* TODO: The gpt partmap module searches for the primary header instead
   * of relying on the disk's sector size. For now trust the disk driver
   * but eventually this code should match the existing behavior.  */
  gpt->log_sector_size = disk->log_sector_size;

  addr = grub_gpt_sector_to_addr (gpt, 1);
  if (grub_disk_read (disk, addr, 0, sizeof (gpt->primary), &gpt->primary))
    return grub_errno;

  if (grub_gpt_header_check (&gpt->primary, gpt->log_sector_size))
    return grub_errno;

  gpt->status |= GRUB_GPT_PRIMARY_HEADER_VALID;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_gpt_read_backup (grub_disk_t disk, grub_gpt_t gpt)
{
  grub_uint64_t sector;
  grub_disk_addr_t addr;

  /* Assumes gpt->log_sector_size == disk->log_sector_size  */
  if (disk->total_sectors != GRUB_DISK_SIZE_UNKNOWN)
    sector = disk->total_sectors - 1;
  else if (gpt->status & GRUB_GPT_PRIMARY_HEADER_VALID)
    sector = grub_le_to_cpu64 (gpt->primary.backup);
  else
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "Unable to locate backup GPT");

  addr = grub_gpt_sector_to_addr (gpt, sector);
  if (grub_disk_read (disk, addr, 0, sizeof (gpt->backup), &gpt->backup))
    return grub_errno;

  if (grub_gpt_header_check (&gpt->backup, gpt->log_sector_size))
    return grub_errno;

  gpt->status |= GRUB_GPT_BACKUP_HEADER_VALID;
  return GRUB_ERR_NONE;
}

static struct grub_gpt_partentry *
grub_gpt_read_entries (grub_disk_t disk, grub_gpt_t gpt,
		       struct grub_gpt_header *header)
{
  struct grub_gpt_partentry *entries = NULL;
  grub_uint8_t *crc32_context = NULL;
  grub_uint32_t count, size, crc;
  grub_disk_addr_t addr;
  grub_size_t entries_size;

  /* Grub doesn't include calloc, hence the manual overflow check.  */
  count = grub_le_to_cpu32 (header->maxpart);
  size = grub_le_to_cpu32 (header->partentry_size);
  entries_size = count *size;
  if (size && entries_size / size != count)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  entries = grub_malloc (entries_size);
  if (!entries)
    goto fail;

  addr = grub_gpt_sector_to_addr (gpt, grub_le_to_cpu64 (header->partitions));
  if (grub_disk_read (disk, addr, 0, entries_size, entries))
    goto fail;

  crc32_context = grub_zalloc (GRUB_MD_CRC32->contextsize);
  if (!crc32_context)
    goto fail;

  GRUB_MD_CRC32->init (crc32_context);
  GRUB_MD_CRC32->write (crc32_context, entries, entries_size);
  GRUB_MD_CRC32->final (crc32_context);

  crc = *(grub_uint32_t *) GRUB_MD_CRC32->read (crc32_context);
  if (grub_swap_bytes32 (crc) != header->partentry_crc32)
    {
      grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT entry crc32");
      goto fail;
    }

  grub_free (crc32_context);
  return entries;

fail:
  grub_free (entries);
  grub_free (crc32_context);
  return NULL;
}

grub_gpt_t
grub_gpt_read (grub_disk_t disk)
{
  grub_gpt_t gpt;
  struct grub_gpt_partentry *backup_entries;

  gpt = grub_zalloc (sizeof (*gpt));
  if (!gpt)
    goto fail;

  if (grub_disk_read (disk, 0, 0, sizeof (gpt->mbr), &gpt->mbr))
    goto fail;

  /* Check the MBR but errors aren't reported beyond the status bit.  */
  if (grub_gpt_pmbr_check (&gpt->mbr))
    grub_errno = GRUB_ERR_NONE;
  else
    gpt->status |= GRUB_GPT_PROTECTIVE_MBR;

  /* If both the primary and backup fail report the primary's error.  */
  if (grub_gpt_read_primary (disk, gpt))
    {
      grub_error_push ();
      grub_gpt_read_backup (disk, gpt);
      grub_error_pop ();
    }
  else
    grub_gpt_read_backup (disk, gpt);

  /* If either succeeded clear any possible error from the other.  */
  if (gpt->status & GRUB_GPT_PRIMARY_HEADER_VALID ||
      gpt->status & GRUB_GPT_BACKUP_HEADER_VALID)
    grub_errno = GRUB_ERR_NONE;
  else
    goto fail;

  /* Same error handling scheme for the entry tables.  */
  gpt->entries = grub_gpt_read_entries (disk, gpt, &gpt->primary);
  if (!gpt->entries)
    {
      grub_error_push ();
      backup_entries = grub_gpt_read_entries (disk, gpt, &gpt->backup);
      grub_error_pop ();
    }
  else
    {
      gpt->status |= GRUB_GPT_PRIMARY_ENTRIES_VALID;
      backup_entries = grub_gpt_read_entries (disk, gpt, &gpt->backup);
    }

  if (backup_entries)
    {
      gpt->status |= GRUB_GPT_BACKUP_ENTRIES_VALID;

      if (gpt->status & GRUB_GPT_PRIMARY_ENTRIES_VALID)
	grub_free (backup_entries);
      else
	gpt->entries = backup_entries;
    }

  if (gpt->status & GRUB_GPT_PRIMARY_ENTRIES_VALID ||
      gpt->status & GRUB_GPT_BACKUP_ENTRIES_VALID)
    {
      grub_errno = GRUB_ERR_NONE;
      return gpt;
    }

fail:
  grub_gpt_free (gpt);
  return NULL;
}

void
grub_gpt_free (grub_gpt_t gpt)
{
  if (!gpt)
    return;

  grub_free (gpt->entries);
  grub_free (gpt);
}
