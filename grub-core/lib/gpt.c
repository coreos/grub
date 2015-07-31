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

#include <grub/charset.h>
#include <grub/crypto.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/msdos_partition.h>
#include <grub/gpt_partition.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_uint8_t grub_gpt_magic[] = GRUB_GPT_HEADER_MAGIC;


char *
grub_gpt_guid_to_str (grub_gpt_guid_t *guid)
{
  return grub_xasprintf ("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			 grub_le_to_cpu32 (guid->data1),
			 grub_le_to_cpu16 (guid->data2),
			 grub_le_to_cpu16 (guid->data3),
			 guid->data4[0], guid->data4[1],
			 guid->data4[2], guid->data4[3],
			 guid->data4[4], guid->data4[5],
			 guid->data4[6], guid->data4[7]);
}

static grub_err_t
grub_gpt_device_partentry (grub_device_t device,
			   struct grub_gpt_partentry *entry)
{
  grub_disk_t disk = device->disk;
  grub_partition_t p;
  grub_err_t err;

  if (!disk || !disk->partition)
    return grub_error (GRUB_ERR_BUG, "not a partition");

  if (grub_strcmp (disk->partition->partmap->name, "gpt"))
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not a GPT partition");

  p = disk->partition;
  disk->partition = p->parent;
  err = grub_disk_read (disk, p->offset, p->index, sizeof (*entry), entry);
  disk->partition = p;

  return err;
}

grub_err_t
grub_gpt_part_label (grub_device_t device, char **label)
{
  struct grub_gpt_partentry entry;
  const grub_size_t name_len = ARRAY_SIZE (entry.name);
  const grub_size_t label_len = name_len * GRUB_MAX_UTF8_PER_UTF16 + 1;
  grub_size_t i;
  grub_uint8_t *end;

  if (grub_gpt_device_partentry (device, &entry))
    return grub_errno;

  *label = grub_malloc (label_len);
  if (!*label)
    return grub_errno;

  for (i = 0; i < name_len; i++)
    entry.name[i] = grub_le_to_cpu16 (entry.name[i]);

  end = grub_utf16_to_utf8 ((grub_uint8_t *) *label, entry.name, name_len);
  *end = '\0';

  return GRUB_ERR_NONE;
}

grub_err_t
grub_gpt_part_uuid (grub_device_t device, char **uuid)
{
  struct grub_gpt_partentry entry;

  if (grub_gpt_device_partentry (device, &entry))
    return grub_errno;

  *uuid = grub_gpt_guid_to_str (&entry.guid);
  if (!*uuid)
    return grub_errno;

  return GRUB_ERR_NONE;
}

static grub_uint64_t
grub_gpt_size_to_sectors (grub_gpt_t gpt, grub_size_t size)
{
  unsigned int sector_size;
  grub_uint64_t sectors;

  sector_size = 1U << gpt->log_sector_size;
  sectors = size / sector_size;
  if (size % sector_size)
    sectors++;

  return sectors;
}

static void
grub_gpt_lecrc32 (grub_uint32_t *crc, const void *data, grub_size_t len)
{
  grub_uint32_t crc32_val;

  grub_crypto_hash (GRUB_MD_CRC32, &crc32_val, data, len);

  /* GRUB_MD_CRC32 always uses big endian, gpt is always little.  */
  *crc = grub_swap_bytes32 (crc32_val);
}

static void
grub_gpt_header_lecrc32 (grub_uint32_t *crc, struct grub_gpt_header *header)
{
  grub_uint32_t old, new;

  /* crc32 must be computed with the field cleared.  */
  old = header->crc32;
  header->crc32 = 0;
  grub_gpt_lecrc32 (&new, header, sizeof (*header));
  header->crc32 = old;

  *crc = new;
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

  grub_gpt_header_lecrc32 (&crc, gpt);
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
    sector = grub_le_to_cpu64 (gpt->primary.alternate_lba);
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

static grub_err_t
grub_gpt_read_entries (grub_disk_t disk, grub_gpt_t gpt,
		       struct grub_gpt_header *header)
{
  struct grub_gpt_partentry *entries = NULL;
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

  /* Double check that the header was validated properly.  */
  if (entries_size < GRUB_GPT_DEFAULT_ENTRIES_SIZE)
    return grub_error (GRUB_ERR_BUG, "invalid GPT entries table size");

  entries = grub_malloc (entries_size);
  if (!entries)
    goto fail;

  addr = grub_gpt_sector_to_addr (gpt, grub_le_to_cpu64 (header->partitions));
  if (grub_disk_read (disk, addr, 0, entries_size, entries))
    goto fail;

  grub_gpt_lecrc32 (&crc, entries, entries_size);
  if (crc != header->partentry_crc32)
    {
      grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid GPT entry crc32");
      goto fail;
    }

  grub_free (gpt->entries);
  gpt->entries = entries;
  gpt->entries_size = entries_size;
  return GRUB_ERR_NONE;

fail:
  grub_free (entries);
  return grub_errno;
}

grub_gpt_t
grub_gpt_read (grub_disk_t disk)
{
  grub_gpt_t gpt;

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

  /* Similarly, favor the value or error from the primary table.  */
  if (gpt->status & GRUB_GPT_BACKUP_HEADER_VALID &&
      !grub_gpt_read_entries (disk, gpt, &gpt->backup))
    gpt->status |= GRUB_GPT_BACKUP_ENTRIES_VALID;

  grub_errno = GRUB_ERR_NONE;
  if (gpt->status & GRUB_GPT_PRIMARY_HEADER_VALID &&
      !grub_gpt_read_entries (disk, gpt, &gpt->primary))
    gpt->status |= GRUB_GPT_PRIMARY_ENTRIES_VALID;

  if (gpt->status & GRUB_GPT_PRIMARY_ENTRIES_VALID ||
      gpt->status & GRUB_GPT_BACKUP_ENTRIES_VALID)
    grub_errno = GRUB_ERR_NONE;
  else
    goto fail;

  return gpt;

fail:
  grub_gpt_free (gpt);
  return NULL;
}

grub_err_t
grub_gpt_repair (grub_disk_t disk, grub_gpt_t gpt)
{
  grub_uint64_t backup_header, backup_entries;

  if (disk->log_sector_size != gpt->log_sector_size)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "GPT sector size must match disk sector size");

  if (!(gpt->status & GRUB_GPT_PRIMARY_ENTRIES_VALID ||
        gpt->status & GRUB_GPT_BACKUP_ENTRIES_VALID))
    return grub_error (GRUB_ERR_BUG, "No valid GPT entries");

  if (gpt->status & GRUB_GPT_PRIMARY_HEADER_VALID)
    {
      backup_header = grub_le_to_cpu64 (gpt->primary.alternate_lba);
      grub_memcpy (&gpt->backup, &gpt->primary, sizeof (gpt->backup));
    }
  else if (gpt->status & GRUB_GPT_BACKUP_HEADER_VALID)
    {
      backup_header = grub_le_to_cpu64 (gpt->backup.header_lba);
      grub_memcpy (&gpt->primary, &gpt->backup, sizeof (gpt->primary));
    }
  else
    return grub_error (GRUB_ERR_BUG, "No valid GPT header");

  /* Relocate backup to end if disk whenever possible.  */
  if (disk->total_sectors != GRUB_DISK_SIZE_UNKNOWN)
    backup_header = disk->total_sectors - 1;

  backup_entries = backup_header -
    grub_gpt_size_to_sectors (gpt, gpt->entries_size);

  /* Update/fixup header and partition table locations.  */
  gpt->primary.header_lba = grub_cpu_to_le64_compile_time (1);
  gpt->primary.alternate_lba = grub_cpu_to_le64 (backup_header);
  gpt->primary.partitions = grub_cpu_to_le64_compile_time (2);
  gpt->backup.header_lba = gpt->primary.alternate_lba;
  gpt->backup.alternate_lba = gpt->primary.header_lba;
  gpt->backup.partitions = grub_cpu_to_le64 (backup_entries);

  /* Recompute checksums.  */
  if (grub_gpt_update_checksums (gpt))
    return grub_errno;

  /* Sanity check.  */
  if (grub_gpt_header_check (&gpt->primary, gpt->log_sector_size))
    return grub_error (GRUB_ERR_BUG, "Generated invalid GPT primary header");

  if (grub_gpt_header_check (&gpt->backup, gpt->log_sector_size))
    return grub_error (GRUB_ERR_BUG, "Generated invalid GPT backup header");

  gpt->status |= GRUB_GPT_BOTH_VALID;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_gpt_update_checksums (grub_gpt_t gpt)
{
  grub_uint32_t crc;

  /* Writing headers larger than our header structure are unsupported.  */
  gpt->primary.headersize =
    grub_cpu_to_le32_compile_time (sizeof (gpt->primary));
  gpt->backup.headersize =
    grub_cpu_to_le32_compile_time (sizeof (gpt->backup));

  grub_gpt_lecrc32 (&crc, gpt->entries, gpt->entries_size);
  gpt->primary.partentry_crc32 = crc;
  gpt->backup.partentry_crc32 = crc;

  grub_gpt_header_lecrc32 (&gpt->primary.crc32, &gpt->primary);
  grub_gpt_header_lecrc32 (&gpt->backup.crc32, &gpt->backup);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_gpt_write_table (grub_disk_t disk, grub_gpt_t gpt,
		      struct grub_gpt_header *header)
{
  grub_disk_addr_t addr;

  if (grub_le_to_cpu32 (header->headersize) != sizeof (*header))
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "Header size is %u, must be %u",
		       grub_le_to_cpu32 (header->headersize),
		       sizeof (*header));

  addr = grub_gpt_sector_to_addr (gpt, grub_le_to_cpu64 (header->header_lba));
  if (grub_disk_write (disk, addr, 0, sizeof (*header), header))
    return grub_errno;

  addr = grub_gpt_sector_to_addr (gpt, grub_le_to_cpu64 (header->partitions));
  if (grub_disk_write (disk, addr, 0, gpt->entries_size, gpt->entries))
    return grub_errno;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_gpt_write (grub_disk_t disk, grub_gpt_t gpt)
{
  /* TODO: update/repair protective MBRs too.  */

  if (!(gpt->status & GRUB_GPT_BOTH_VALID))
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "Invalid GPT data");

  if (grub_gpt_write_table (disk, gpt, &gpt->primary))
    return grub_errno;

  if (grub_gpt_write_table (disk, gpt, &gpt->backup))
    return grub_errno;

  return GRUB_ERR_NONE;
}

void
grub_gpt_free (grub_gpt_t gpt)
{
  if (!gpt)
    return;

  grub_free (gpt->entries);
  grub_free (gpt);
}
