/*
 *  GRUB  --  GRand Unified Bootloader
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

#include <grub/command.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/misc.h>
#include <grub/err.h>
#include <grub/gpt_partition.h>
#include <grub/msdos_partition.h>
#include <grub/lib/hexdump.h>
#include <grub/test.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

/* from gnulib */
#include <verify.h>


/* GPT section sizes.  */
#define HEADER_SIZE   (sizeof (struct grub_gpt_header))
#define HEADER_PAD    (GRUB_DISK_SECTOR_SIZE - HEADER_SIZE)
#define ENTRY_SIZE    (sizeof (struct grub_gpt_partentry))
#define TABLE_ENTRIES 0x80
#define TABLE_SIZE    (TABLE_ENTRIES * ENTRY_SIZE)
#define TABLE_SECTORS (TABLE_SIZE / GRUB_DISK_SECTOR_SIZE)

/* Double check that the table size calculation was valid.  */
verify (TABLE_SECTORS * GRUB_DISK_SECTOR_SIZE == TABLE_SIZE);

/* GPT section locations for a 1MiB disk.  */
#define DISK_SECTORS	      0x800
#define DISK_SIZE	      (GRUB_DISK_SECTOR_SIZE * DISK_SECTORS)
#define PRIMARY_HEADER_SECTOR 0x1
#define PRIMARY_TABLE_SECTOR  0x2
#define BACKUP_HEADER_SECTOR  (DISK_SECTORS - 0x1)
#define BACKUP_TABLE_SECTOR   (BACKUP_HEADER_SECTOR - TABLE_SECTORS)

#define DATA_START_SECTOR     (PRIMARY_TABLE_SECTOR + TABLE_SECTORS)
#define DATA_END_SECTOR	      (BACKUP_TABLE_SECTOR - 0x1)
#define DATA_SECTORS	      (BACKUP_TABLE_SECTOR - DATA_START_SECTOR)
#define DATA_SIZE	      (GRUB_DISK_SECTOR_SIZE * DATA_SECTORS)

struct test_disk
{
  struct grub_msdos_partition_mbr mbr;

  struct grub_gpt_header primary_header;
  grub_uint8_t primary_header_pad[HEADER_PAD];
  struct grub_gpt_partentry primary_entries[TABLE_ENTRIES];

  grub_uint8_t data[DATA_SIZE];

  struct grub_gpt_partentry backup_entries[TABLE_ENTRIES];
  struct grub_gpt_header backup_header;
  grub_uint8_t backup_header_pad[HEADER_PAD];
} GRUB_PACKED;

/* Sanity check that all the above ugly math was correct.  */
verify (sizeof (struct test_disk) == DISK_SIZE);

struct test_data
{
  int fd;
  grub_device_t dev;
  struct test_disk *raw;
};


/* Sample primary GPT header for an empty 1MB disk.  */
static const struct grub_gpt_header example_primary = {
  .magic = GRUB_GPT_HEADER_MAGIC,
  .version = GRUB_GPT_HEADER_VERSION,
  .headersize = sizeof (struct grub_gpt_header),
  .crc32 = grub_cpu_to_le32_compile_time (0x7cd8642c),
  .header_lba = grub_cpu_to_le64_compile_time (PRIMARY_HEADER_SECTOR),
  .alternate_lba = grub_cpu_to_le64_compile_time (BACKUP_HEADER_SECTOR),
  .start = grub_cpu_to_le64_compile_time (DATA_START_SECTOR),
  .end = grub_cpu_to_le64_compile_time (DATA_END_SECTOR),
  .guid = {0xad, 0x31, 0xc1, 0x69, 0xd6, 0x67, 0xc6, 0x46,
	   0x93, 0xc4, 0x12, 0x4c, 0x75, 0x52, 0x56, 0xac},
  .partitions = grub_cpu_to_le64_compile_time (PRIMARY_TABLE_SECTOR),
  .maxpart = grub_cpu_to_le32_compile_time (TABLE_ENTRIES),
  .partentry_size = grub_cpu_to_le32_compile_time (ENTRY_SIZE),
  .partentry_crc32 = grub_cpu_to_le32_compile_time (0xab54d286),
};

/* And the backup header.  */
static const struct grub_gpt_header example_backup = {
  .magic = GRUB_GPT_HEADER_MAGIC,
  .version = GRUB_GPT_HEADER_VERSION,
  .headersize = sizeof (struct grub_gpt_header),
  .crc32 = grub_cpu_to_le32_compile_time (0xcfaa4a27),
  .header_lba = grub_cpu_to_le64_compile_time (BACKUP_HEADER_SECTOR),
  .alternate_lba = grub_cpu_to_le64_compile_time (PRIMARY_HEADER_SECTOR),
  .start = grub_cpu_to_le64_compile_time (DATA_START_SECTOR),
  .end = grub_cpu_to_le64_compile_time (DATA_END_SECTOR),
  .guid = {0xad, 0x31, 0xc1, 0x69, 0xd6, 0x67, 0xc6, 0x46,
	   0x93, 0xc4, 0x12, 0x4c, 0x75, 0x52, 0x56, 0xac},
  .partitions = grub_cpu_to_le64_compile_time (BACKUP_TABLE_SECTOR),
  .maxpart = grub_cpu_to_le32_compile_time (TABLE_ENTRIES),
  .partentry_size = grub_cpu_to_le32_compile_time (ENTRY_SIZE),
  .partentry_crc32 = grub_cpu_to_le32_compile_time (0xab54d286),
};

/* Sample protective MBR for the same 1MB disk. Note, this matches
 * parted and fdisk behavior. The UEFI spec uses different values.  */
static const struct grub_msdos_partition_mbr example_pmbr = {
  .entries = {{.flag = 0x00,
	       .start_head = 0x00,
	       .start_sector = 0x01,
	       .start_cylinder = 0x00,
	       .type = 0xee,
	       .end_head = 0xfe,
	       .end_sector = 0xff,
	       .end_cylinder = 0xff,
	       .start = grub_cpu_to_le32_compile_time (0x1),
	       .length = grub_cpu_to_le32_compile_time (DISK_SECTORS - 0x1),
	       }},
  .signature = grub_cpu_to_le16_compile_time (GRUB_PC_PARTITION_SIGNATURE),
};

/* If errors are left in grub's error stack things can get confused.  */
static void
assert_error_stack_empty (void)
{
  do
    {
      grub_test_assert (grub_errno == GRUB_ERR_NONE,
			"error on stack: %s", grub_errmsg);
    }
  while (grub_error_pop ());
}

static grub_err_t
execute_command2 (const char *name, const char *arg1, const char *arg2)
{
  grub_command_t cmd;
  grub_err_t err;
  char *argv[2];

  cmd = grub_command_find (name);
  if (!cmd)
    grub_fatal ("can't find command %s", name);

  argv[0] = strdup (arg1);
  argv[1] = strdup (arg2);
  err = (cmd->func) (cmd, 2, argv);
  free (argv[0]);
  free (argv[1]);

  return err;
}

static void
sync_disk (struct test_data *data)
{
  if (msync (data->raw, DISK_SIZE, MS_SYNC | MS_INVALIDATE) < 0)
    grub_fatal ("Syncing disk failed: %s", strerror (errno));

  grub_disk_cache_invalidate_all ();
}

static void
reset_disk (struct test_data *data)
{
  memset (data->raw, 0, DISK_SIZE);

  /* Initialize image with valid example tables.  */
  memcpy (&data->raw->mbr, &example_pmbr, sizeof (data->raw->mbr));
  memcpy (&data->raw->primary_header, &example_primary,
	  sizeof (data->raw->primary_header));
  memcpy (&data->raw->backup_header, &example_backup,
	  sizeof (data->raw->backup_header));

  sync_disk (data);
}

static void
open_disk (struct test_data *data)
{
  const char *loop = "loop0";
  char template[] = "/tmp/grub_gpt_test.XXXXXX";
  char host[sizeof ("(host)") + sizeof (template)];

  data->fd = mkstemp (template);
  if (data->fd < 0)
    grub_fatal ("Creating %s failed: %s", template, strerror (errno));

  if (ftruncate (data->fd, DISK_SIZE) < 0)
    {
      int err = errno;
      unlink (template);
      grub_fatal ("Resizing %s failed: %s", template, strerror (err));
    }

  data->raw = mmap (NULL, DISK_SIZE, PROT_READ | PROT_WRITE,
		    MAP_SHARED, data->fd, 0);
  if (data->raw == MAP_FAILED)
    {
      int err = errno;
      unlink (template);
      grub_fatal ("Maping %s failed: %s", template, strerror (err));
    }

  snprintf (host, sizeof (host), "(host)%s", template);
  if (execute_command2 ("loopback", loop, host) != GRUB_ERR_NONE)
    {
      unlink (template);
      grub_fatal ("loopback %s %s failed: %s", loop, host, grub_errmsg);
    }

  if (unlink (template) < 0)
    grub_fatal ("Unlinking %s failed: %s", template, strerror (errno));

  reset_disk (data);

  data->dev = grub_device_open (loop);
  if (!data->dev)
    grub_fatal ("Opening %s failed: %s", loop, grub_errmsg);
}

static void
close_disk (struct test_data *data)
{
  char *loop;

  assert_error_stack_empty ();

  if (munmap (data->raw, DISK_SIZE) || close (data->fd))
    grub_fatal ("Closing disk image failed: %s", strerror (errno));

  loop = strdup (data->dev->disk->name);
  grub_test_assert (grub_device_close (data->dev) == GRUB_ERR_NONE,
		    "Closing disk device failed: %s", grub_errmsg);

  grub_test_assert (execute_command2 ("loopback", "-d", loop) ==
		    GRUB_ERR_NONE, "loopback -d %s failed: %s", loop,
		    grub_errmsg);

  free (loop);
}

static grub_gpt_t
read_disk (struct test_data *data)
{
  grub_gpt_t gpt;

  gpt = grub_gpt_read (data->dev->disk);
  if (gpt == NULL)
    {
      grub_print_error ();
      grub_fatal ("grub_gpt_read failed");
    }


  return gpt;
}

static void
pmbr_test (void)
{
  struct grub_msdos_partition_mbr mbr;

  memset (&mbr, 0, sizeof (mbr));

  /* Empty is invalid.  */
  grub_gpt_pmbr_check (&mbr);
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  /* A table without a protective partition is invalid.  */
  mbr.signature = grub_cpu_to_le16_compile_time (GRUB_PC_PARTITION_SIGNATURE);
  grub_gpt_pmbr_check (&mbr);
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  /* A table with a protective type is ok.  */
  memcpy (&mbr, &example_pmbr, sizeof (mbr));
  grub_gpt_pmbr_check (&mbr);
  grub_test_assert (grub_errno == GRUB_ERR_NONE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;
}

static void
header_test (void)
{
  struct grub_gpt_header primary, backup;

  /* Example headers should be valid.  */
  memcpy (&primary, &example_primary, sizeof (primary));
  grub_gpt_header_check (&primary, GRUB_DISK_SECTOR_BITS);
  grub_test_assert (grub_errno == GRUB_ERR_NONE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  memcpy (&backup, &example_backup, sizeof (backup));
  grub_gpt_header_check (&backup, GRUB_DISK_SECTOR_BITS);
  grub_test_assert (grub_errno == GRUB_ERR_NONE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  /* Twiddle the GUID to invalidate the CRC. */
  primary.guid[0] = 0;
  grub_gpt_header_check (&primary, GRUB_DISK_SECTOR_BITS);
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  backup.guid[0] = 0;
  grub_gpt_header_check (&backup, GRUB_DISK_SECTOR_BITS);
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;
}

static void
read_valid_test (void)
{
  struct test_data data;
  grub_gpt_t gpt;

  open_disk (&data);
  gpt = read_disk (&data);
  grub_test_assert (gpt->status == (GRUB_GPT_PROTECTIVE_MBR |
				    GRUB_GPT_PRIMARY_HEADER_VALID |
				    GRUB_GPT_PRIMARY_ENTRIES_VALID |
				    GRUB_GPT_BACKUP_HEADER_VALID |
				    GRUB_GPT_BACKUP_ENTRIES_VALID),
		    "unexpected status: 0x%02x", gpt->status);
  grub_gpt_free (gpt);
  close_disk (&data);
}

static void
read_invalid_entries_test (void)
{
  struct test_data data;
  grub_gpt_t gpt;

  open_disk (&data);

  /* Corrupt the first entry in both tables.  */
  memset (&data.raw->primary_entries[0], 0x55,
	  sizeof (data.raw->primary_entries[0]));
  memset (&data.raw->backup_entries[0], 0x55,
	  sizeof (data.raw->backup_entries[0]));
  sync_disk (&data);

  gpt = grub_gpt_read (data.dev->disk);
  grub_test_assert (gpt == NULL, "no error reported for corrupt entries");
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  close_disk (&data);
}

static void
read_fallback_test (void)
{
  struct test_data data;
  grub_gpt_t gpt;

  open_disk (&data);

  /* Corrupt the primary header.  */
  memset (&data.raw->primary_header.guid, 0x55,
	  sizeof (data.raw->primary_header.guid));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_test_assert ((gpt->status & GRUB_GPT_PRIMARY_HEADER_VALID) == 0,
		    "unreported corrupt primary header");
  grub_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the backup header.  */
  memset (&data.raw->backup_header.guid, 0x55,
	  sizeof (data.raw->backup_header.guid));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_test_assert ((gpt->status & GRUB_GPT_BACKUP_HEADER_VALID) == 0,
		    "unreported corrupt backup header");
  grub_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the primary entry table.  */
  memset (&data.raw->primary_entries[0], 0x55,
	  sizeof (data.raw->primary_entries[0]));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_test_assert ((gpt->status & GRUB_GPT_PRIMARY_ENTRIES_VALID) == 0,
		    "unreported corrupt primary entries table");
  grub_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the backup entry table.  */
  memset (&data.raw->backup_entries[0], 0x55,
	  sizeof (data.raw->backup_entries[0]));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_test_assert ((gpt->status & GRUB_GPT_BACKUP_ENTRIES_VALID) == 0,
		    "unreported corrupt backup entries table");
  grub_gpt_free (gpt);
  reset_disk (&data);

  /* If primary is corrupt and disk size is unknown fallback fails.  */
  memset (&data.raw->primary_header.guid, 0x55,
	  sizeof (data.raw->primary_header.guid));
  sync_disk (&data);
  data.dev->disk->total_sectors = GRUB_DISK_SIZE_UNKNOWN;
  gpt = grub_gpt_read (data.dev->disk);
  grub_test_assert (gpt == NULL, "no error reported");
  grub_test_assert (grub_errno == GRUB_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", grub_errmsg);
  grub_errno = GRUB_ERR_NONE;

  close_disk (&data);
}

static void
repair_test (void)
{
  struct test_data data;
  grub_gpt_t gpt;

  open_disk (&data);

  /* Erase/Repair primary.  */
  memset (&data.raw->primary_header, 0, sizeof (data.raw->primary_header));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_gpt_repair (data.dev->disk, gpt);
  grub_test_assert (grub_errno == GRUB_ERR_NONE,
		    "repair failed: %s", grub_errmsg);
  if (memcmp (&gpt->primary, &example_primary, sizeof (gpt->primary)))
    {
      printf ("Invalid restored primary header:\n");
      hexdump (16, (char*)&gpt->primary, sizeof (gpt->primary));
      printf ("Expected primary header:\n");
      hexdump (16, (char*)&example_primary, sizeof (example_primary));
      grub_test_assert (0, "repair did not restore primary header");
    }
  grub_gpt_free (gpt);
  reset_disk (&data);

  /* Erase/Repair backup.  */
  memset (&data.raw->backup_header, 0, sizeof (data.raw->backup_header));
  sync_disk (&data);
  gpt = read_disk (&data);
  grub_gpt_repair (data.dev->disk, gpt);
  grub_test_assert (grub_errno == GRUB_ERR_NONE,
		    "repair failed: %s", grub_errmsg);
  if (memcmp (&gpt->backup, &example_backup, sizeof (gpt->backup)))
    {
      printf ("Invalid restored backup header:\n");
      hexdump (16, (char*)&gpt->backup, sizeof (gpt->backup));
      printf ("Expected backup header:\n");
      hexdump (16, (char*)&example_backup, sizeof (example_backup));
      grub_test_assert (0, "repair did not restore backup header");
    }
  grub_gpt_free (gpt);
  reset_disk (&data);

  close_disk (&data);
}
void
grub_unit_test_init (void)
{
  grub_init_all ();
  grub_hostfs_init ();
  grub_host_init ();
  grub_test_register ("gpt_pmbr_test", pmbr_test);
  grub_test_register ("gpt_header_test", header_test);
  grub_test_register ("gpt_read_valid_test", read_valid_test);
  grub_test_register ("gpt_read_invalid_test", read_invalid_entries_test);
  grub_test_register ("gpt_read_fallback_test", read_fallback_test);
  grub_test_register ("gpt_repair_test", repair_test);
}

void
grub_unit_test_fini (void)
{
  grub_test_unregister ("gpt_pmbr_test");
  grub_test_unregister ("gpt_header_test");
  grub_test_unregister ("gpt_read_valid_test");
  grub_test_unregister ("gpt_read_invalid_test");
  grub_test_unregister ("gpt_read_fallback_test");
  grub_test_unregister ("gpt_repair_test");
  grub_fini_all ();
}
