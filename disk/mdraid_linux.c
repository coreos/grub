/* mdraid_linux.c - module to handle Linux Software RAID.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/raid.h>

/* Linux RAID on disk structures and constants,
   copied from include/linux/raid/md_p.h.  */

#define RESERVED_BYTES			(64 * 1024)
#define RESERVED_SECTORS		(RESERVED_BYTES / 512)

#define NEW_SIZE_SECTORS(x)		((x & ~(RESERVED_SECTORS - 1)) \
					- RESERVED_SECTORS)

#define SB_BYTES			4096
#define SB_WORDS			(SB_BYTES / 4)
#define SB_SECTORS			(SB_BYTES / 512)

/*
 * The following are counted in 32-bit words
 */
#define	SB_GENERIC_OFFSET		0

#define SB_PERSONALITY_OFFSET		64
#define SB_DISKS_OFFSET			128
#define SB_DESCRIPTOR_OFFSET		992

#define SB_GENERIC_CONSTANT_WORDS	32
#define SB_GENERIC_STATE_WORDS		32
#define SB_GENERIC_WORDS		(SB_GENERIC_CONSTANT_WORDS + \
                                         SB_GENERIC_STATE_WORDS)

#define SB_PERSONALITY_WORDS		64
#define SB_DESCRIPTOR_WORDS		32
#define SB_DISKS			27
#define SB_DISKS_WORDS			(SB_DISKS * SB_DESCRIPTOR_WORDS)

#define SB_RESERVED_WORDS		(1024 \
                                         - SB_GENERIC_WORDS \
                                         - SB_PERSONALITY_WORDS \
                                         - SB_DISKS_WORDS \
                                         - SB_DESCRIPTOR_WORDS)

#define SB_EQUAL_WORDS			(SB_GENERIC_WORDS \
                                         + SB_PERSONALITY_WORDS \
                                         + SB_DISKS_WORDS)

/*
 * Device "operational" state bits
 */
#define DISK_FAULTY			0
#define DISK_ACTIVE			1
#define DISK_SYNC			2
#define DISK_REMOVED			3

#define	DISK_WRITEMOSTLY		9

#define SB_MAGIC			0xa92b4efc

/*
 * Superblock state bits
 */
#define SB_CLEAN			0
#define SB_ERRORS			1

#define	SB_BITMAP_PRESENT		8

struct grub_raid_disk_09
{
  grub_uint32_t number;		/* Device number in the entire set.  */
  grub_uint32_t major;		/* Device major number.  */
  grub_uint32_t minor;		/* Device minor number.  */
  grub_uint32_t raid_disk;	/* The role of the device in the raid set.  */
  grub_uint32_t state;		/* Operational state.  */
  grub_uint32_t reserved[SB_DESCRIPTOR_WORDS - 5];
};

struct grub_raid_super_09
{
  /*
   * Constant generic information
   */
  grub_uint32_t md_magic;	/* MD identifier.  */
  grub_uint32_t major_version;	/* Major version.  */
  grub_uint32_t minor_version;	/* Minor version.  */
  grub_uint32_t patch_version;	/* Patchlevel version.  */
  grub_uint32_t gvalid_words;	/* Number of used words in this section.  */
  grub_uint32_t set_uuid0;	/* Raid set identifier.  */
  grub_uint32_t ctime;		/* Creation time.  */
  grub_uint32_t level;		/* Raid personality.  */
  grub_uint32_t size;		/* Apparent size of each individual disk.  */
  grub_uint32_t nr_disks;	/* Total disks in the raid set.  */
  grub_uint32_t raid_disks;	/* Disks in a fully functional raid set.  */
  grub_uint32_t md_minor;	/* Preferred MD minor device number.  */
  grub_uint32_t not_persistent;	/* Does it have a persistent superblock.  */
  grub_uint32_t set_uuid1;	/* Raid set identifier #2.  */
  grub_uint32_t set_uuid2;	/* Raid set identifier #3.  */
  grub_uint32_t set_uuid3;	/* Raid set identifier #4.  */
  grub_uint32_t gstate_creserved[SB_GENERIC_CONSTANT_WORDS - 16];

  /*
   * Generic state information
   */
  grub_uint32_t utime;		/* Superblock update time.  */
  grub_uint32_t state;		/* State bits (clean, ...).  */
  grub_uint32_t active_disks;	/* Number of currently active disks.  */
  grub_uint32_t working_disks;	/* Number of working disks.  */
  grub_uint32_t failed_disks;	/* Number of failed disks.  */
  grub_uint32_t spare_disks;	/* Number of spare disks.  */
  grub_uint32_t sb_csum;	/* Checksum of the whole superblock.  */
  grub_uint64_t events;		/* Superblock update count.  */
  grub_uint64_t cp_events;	/* Checkpoint update count.  */
  grub_uint32_t recovery_cp;	/* Recovery checkpoint sector count.  */
  grub_uint32_t gstate_sreserved[SB_GENERIC_STATE_WORDS - 12];

  /*
   * Personality information
   */
  grub_uint32_t layout;		/* The array's physical layout.  */
  grub_uint32_t chunk_size;	/* Chunk size in bytes.  */
  grub_uint32_t root_pv;	/* LV root PV.  */
  grub_uint32_t root_block;	/* LV root block.  */
  grub_uint32_t pstate_reserved[SB_PERSONALITY_WORDS - 4];

  /*
   * Disks information
   */
  struct grub_raid_disk_09 disks[SB_DISKS];

  /*
   * Reserved
   */
  grub_uint32_t reserved[SB_RESERVED_WORDS];

  /*
   * Active descriptor
   */
  struct grub_raid_disk_09 this_disk;
} __attribute__ ((packed));

/*
 * The version-1 superblock :
 * All numeric fields are little-endian.
 *
 * Total size: 256 bytes plus 2 per device.
 * 1K allows 384 devices.
 */

struct grub_raid_super_1x
{
  /* Constant array information - 128 bytes.  */
  grub_uint32_t magic;		/* MD_SB_MAGIC: 0xa92b4efc - little endian.  */
  grub_uint32_t major_version;	/* 1.  */
  grub_uint32_t feature_map;	/* Bit 0 set if 'bitmap_offset' is meaningful.   */
  grub_uint32_t pad0;		/* Always set to 0 when writing.  */

  grub_uint8_t set_uuid[16];	/* User-space generated.  */
  char set_name[32];		/* Set and interpreted by user-space.  */

  grub_uint64_t ctime;		/* Lo 40 bits are seconds, top 24 are microseconds or 0.  */
  grub_uint32_t level;		/* -4 (multipath), -1 (linear), 0,1,4,5.  */
  grub_uint32_t layout;		/* only for raid5 and raid10 currently.  */
  grub_uint64_t size;		/* Used size of component devices, in 512byte sectors.  */

  grub_uint32_t chunksize;	/* In 512byte sectors.  */
  grub_uint32_t raid_disks;
  grub_uint32_t bitmap_offset;	/* Sectors after start of superblock that bitmap starts
				 * NOTE: signed, so bitmap can be before superblock
				 * only meaningful of feature_map[0] is set.
				 */

  /* These are only valid with feature bit '4'.  */
  grub_uint32_t new_level;	/* New level we are reshaping to.  */
  grub_uint64_t reshape_position;	/* Next address in array-space for reshape.  */
  grub_uint32_t delta_disks;	/* Change in number of raid_disks.  */
  grub_uint32_t new_layout;	/* New layout.  */
  grub_uint32_t new_chunk;	/* New chunk size (512byte sectors).  */
  grub_uint8_t pad1[128 - 124];	/* Set to 0 when written.  */

  /* Constant this-device information - 64 bytes.  */
  grub_uint64_t data_offset;	/* Sector start of data, often 0.  */
  grub_uint64_t data_size;	/* Sectors in this device that can be used for data.  */
  grub_uint64_t super_offset;	/* Sector start of this superblock.  */
  grub_uint64_t recovery_offset;	/* Sectors before this offset (from data_offset) have been recovered.  */
  grub_uint32_t dev_number;	/* Permanent identifier of this  device - not role in raid.  */
  grub_uint32_t cnt_corrected_read;	/* Number of read errors that were corrected by re-writing.  */
  grub_uint8_t device_uuid[16];	/* User-space setable, ignored by kernel.  */
  grub_uint8_t devflags;	/* Per-device flags.  Only one defined...  */
  grub_uint8_t pad2[64 - 57];	/* Set to 0 when writing.  */

  /* Array state information - 64 bytes.  */
  grub_uint64_t utime;		/* 40 bits second, 24 btes microseconds.  */
  grub_uint64_t events;		/* Incremented when superblock updated.  */
  grub_uint64_t resync_offset;	/* Data before this offset (from data_offset) known to be in sync.  */
  grub_uint32_t sb_csum;	/* Checksum upto devs[max_dev].  */
  grub_uint32_t max_dev;	/* Size of devs[] array to consider.  */
  grub_uint8_t pad3[64 - 32];	/* Set to 0 when writing.  */

  /* Device state information. Indexed by dev_number.
   * 2 bytes per device.
   * Note there are no per-device state flags. State information is rolled
   * into the 'roles' value.  If a device is spare or faulty, then it doesn't
   * have a meaningful role.
   */
  grub_uint16_t dev_roles[0];	/* Role in array, or 0xffff for a spare, or 0xfffe for faulty.  */
};
/* Could be __attribute__ ((packed)), but since all members in this struct
   are already appropriately aligned, we can omit this and avoid suboptimal
   assembly in some cases.  */

#define WriteMostly1    1	/* Mask for writemostly flag in above devflags.  */

static grub_err_t
grub_mdraid_detect_09 (grub_disk_addr_t sector,
		       struct grub_raid_super_09 *sb,
		       struct grub_raid_array *array,
		       grub_disk_addr_t *start_sector)
{
  grub_uint32_t *uuid;

  if (sb->major_version != 0 || sb->minor_version != 90)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "unsupported RAID version: %d.%d",
		       sb->major_version, sb->minor_version);

  /* FIXME: Check the checksum.  */

  /* Multipath.  */
  if ((int) sb->level == -4)
    sb->level = 1;

  if (sb->level != 0 && sb->level != 1 && sb->level != 4 &&
      sb->level != 5 && sb->level != 6 && sb->level != 10)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "unsupported RAID level: %d", sb->level);

  array->name = NULL;
  array->number = sb->md_minor;
  array->level = sb->level;
  array->layout = sb->layout;
  array->total_devs = sb->raid_disks;
  array->disk_size = (sb->size) ? sb->size * 2 : sector;
  array->chunk_size = sb->chunk_size >> 9;
  array->index = sb->this_disk.number;
  array->uuid_len = 16;
  array->uuid = grub_malloc (16);
  if (!array->uuid)
      return grub_errno;

  uuid = (grub_uint32_t *) array->uuid;
  uuid[0] = sb->set_uuid0;
  uuid[1] = sb->set_uuid1;
  uuid[2] = sb->set_uuid2;
  uuid[3] = sb->set_uuid3;

  *start_sector = 0;

  return 0;
}

static grub_err_t
grub_mdraid_detect_1x (grub_disk_t disk, grub_disk_addr_t sector,
		       struct grub_raid_super_1x *sb,
		       struct grub_raid_array *array,
		       grub_disk_addr_t *start_sector)
{
  grub_uint64_t sb_size;
  struct grub_raid_super_1x *real_sb;

  if (sb->major_version != 1)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "Unsupported RAID version: %d",
		       sb->major_version);

  /* Multipath.  */
  if ((int) sb->level == -4)
    sb->level = 1;

  if (sb->level != 0 && sb->level != 1 && sb->level != 4 &&
      sb->level != 5 && sb->level != 6 && sb->level != 10)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "Unsupported RAID level: %d", sb->level);

  /* 1.x superblocks don't have a fixed size on disk.  So we have to
     read it again now that we now the max device count.  */
  sb_size = sizeof (struct grub_raid_super_1x) + 2 * grub_le_to_cpu32 (sb->max_dev);
  real_sb = grub_malloc (sb_size);
  if (! real_sb)
    return grub_errno;

  if (grub_disk_read (disk, sector, 0, sb_size, real_sb))
    {
      grub_free (real_sb);
      return grub_errno;
    }

  array->name = grub_strdup (real_sb->set_name);
  if (! array->name)
    {
      grub_free (real_sb);
      return grub_errno;
    }

  array->number = 0;
  array->level = grub_le_to_cpu32 (real_sb->level);
  array->layout = grub_le_to_cpu32 (real_sb->layout);
  array->total_devs = grub_le_to_cpu32 (real_sb->raid_disks);
  array->disk_size = grub_le_to_cpu64 (real_sb->size);
  array->chunk_size = grub_le_to_cpu32 (real_sb->chunksize);
  if (grub_le_to_cpu32 (real_sb->dev_number) <
      grub_le_to_cpu32 (real_sb->max_dev))
    array->index = grub_le_to_cpu16
      (real_sb->dev_roles[grub_le_to_cpu32 (real_sb->dev_number)]);
  else
    array->index = 0xffff;  /* disk will be later not used! */
  array->uuid_len = 16;
  array->uuid = grub_malloc (16);
  if (!array->uuid)
    {
      grub_free (real_sb);
      return grub_errno;
    }

  grub_memcpy (array->uuid, real_sb->set_uuid, 16);

  *start_sector = real_sb->data_offset;

  grub_free (real_sb);
  return 0;
}

static grub_err_t
grub_mdraid_detect (grub_disk_t disk, struct grub_raid_array *array,
		    grub_disk_addr_t *start_sector)
{
  grub_disk_addr_t sector;
  grub_uint64_t size;
  struct grub_raid_super_09 sb_09;
  struct grub_raid_super_1x sb_1x;
  grub_uint8_t minor_version;

  /* The sector where the mdraid 0.90 superblock is stored, if available.  */
  size = grub_disk_get_size (disk);
  sector = NEW_SIZE_SECTORS (size);

  if (grub_disk_read (disk, sector, 0, SB_BYTES, &sb_09))
    return grub_errno;

  /* Look whether there is a mdraid 0.90 superblock.  */
  if (sb_09.md_magic == SB_MAGIC)
    return grub_mdraid_detect_09 (sector, &sb_09, array, start_sector);

  /* Check for an 1.x superblock.
   * It's always aligned to a 4K boundary
   * and depending on the minor version it can be:
   * 0: At least 8K, but less than 12K, from end of device
   * 1: At start of device
   * 2: 4K from start of device.
   */

  for (minor_version = 0; minor_version < 3; ++minor_version)
    {
      switch (minor_version)
	{
	case 0:
	  sector = (size - 8 * 2) & ~(4 * 2 - 1);
	  break;
	case 1:
	  sector = 0;
	  break;
	case 2:
	  sector = 4 * 2;
	  break;
	}

      if (grub_disk_read (disk, sector, 0, sizeof (struct grub_raid_super_1x),
			  &sb_1x))
	return grub_errno;

      if (sb_1x.magic == SB_MAGIC)
	return grub_mdraid_detect_1x (disk, sector, &sb_1x, array,
				      start_sector);
    }

  /* Neither 0.90 nor 1.x.  */
  return grub_error (GRUB_ERR_OUT_OF_RANGE, "not raid");
}

static struct grub_raid grub_mdraid_dev = {
  .name = "mdraid",
  .detect = grub_mdraid_detect,
  .next = 0
};

GRUB_MOD_INIT (mdraid)
{
  grub_raid_register (&grub_mdraid_dev);
}

GRUB_MOD_FINI (mdraid)
{
  grub_raid_unregister (&grub_mdraid_dev);
}
