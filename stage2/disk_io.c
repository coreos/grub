/* disk_io.c - implement abstract BIOS disk input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "shared.h"

#include "filesys.h"

#ifndef STAGE1_5
/* instrumentation variables */
void (*disk_read_hook) (int) = NULL;
void (*disk_read_func) (int) = NULL;

int print_possibilities;

static int do_completion;
static int unique;
static char *unique_string;

#endif

int fsmax;
struct fsys_entry fsys_table[NUM_FSYS + 1] =
{
  /* TFTP should come first because others don't handle net device.  */
# ifdef FSYS_TFTP
  {"tftp", tftp_mount, tftp_read, tftp_dir, tftp_close},
# endif
# ifdef FSYS_FAT
  {"fat", fat_mount, 0, fat_dir, 0},
# endif
# ifdef FSYS_EXT2FS
  {"ext2fs", ext2fs_mount, ext2fs_read, ext2fs_dir, 0},
# endif
# ifdef FSYS_MINIX
  {"minix", minix_mount, minix_read, minix_dir, 0},
# endif
  /* XX FFS should come last as it's superblock is commonly crossing tracks
     on floppies from track 1 to 2, while others only use 1.  */
# ifdef FSYS_FFS
  {"ffs", ffs_mount, ffs_read, ffs_dir, 0},
# endif
  {0, 0, 0, 0, 0}
};


/* These have the same format as "boot_drive" and "install_partition", but
   are meant to be working values. */
unsigned long current_drive = 0xFF;
unsigned long current_partition;

#ifndef STAGE1_5
/* The register ESI should contain the address of the partition to be
   used for loading a chain-loader when chain-loading the loader.  */
unsigned long boot_part_addr;
unsigned long boot_part_offset;
#endif

/*
 *  Global variables describing details of the filesystem
 */

/* FIXME: BSD evil hack */
#include "freebsd.h"
int bsd_evil_hack;

/* filesystem type */
int fsys_type = NUM_FSYS;
#ifndef NO_BLOCK_FILES
int block_file = 0;
#endif /* NO_BLOCK_FILES */

/* these are the translated numbers for the open partition */
long part_start;
long part_length;

int current_slice;

/* disk buffer parameters */
int buf_drive = -1;
int buf_track;
struct geometry buf_geom;

/* filesystem common variables */
int filepos;
int filemax;


int
rawread (int drive, int sector, int byte_offset, int byte_len, char *buf)
{
  int slen = (byte_offset + byte_len + SECTOR_SIZE - 1) / SECTOR_SIZE;

  if (byte_len <= 0)
    return 1;

  while (byte_len > 0 && !errnum)
    {
      int soff, num_sect, bufaddr, track, size = byte_len;

      /*
       *  Check track buffer.  If it isn't valid or it is from the
       *  wrong disk, then reset the disk geometry.
       */
      if (buf_drive != drive)
	{
	  if (get_diskinfo (drive, &buf_geom))
	    {
	      errnum = ERR_NO_DISK;
	      return 0;
	    }
	  buf_drive = drive;
	  buf_track = -1;
	}

      /*  Get first sector of track  */
      soff = sector % buf_geom.sectors;
      track = sector - soff;
      num_sect = buf_geom.sectors - soff;
      bufaddr = BUFFERADDR + (soff * SECTOR_SIZE) + byte_offset;

      if (track != buf_track)
	{
	  int bios_err, read_start = track, read_len = buf_geom.sectors;

	  /*
	   *  If there's more than one read in this entire loop, then
	   *  only make the earlier reads for the portion needed.  This
	   *  saves filling the buffer with data that won't be used!
	   */
	  if (slen > num_sect)
	    {
	      read_start = sector;
	      read_len = num_sect;
	      bufaddr = BUFFERADDR + byte_offset;
	    }

	  bios_err = biosdisk (BIOSDISK_READ, drive, &buf_geom,
			       read_start, read_len, BUFFERSEG);
	  if (bios_err)
	    {
	      buf_track = -1;

	      if (bios_err == BIOSDISK_ERROR_GEOMETRY)
		errnum = ERR_GEOM;
	      else
		{
		  /*
		   *  If there was an error, try to load only the
		   *  required sector(s) rather than failing completely.
		   */
		  if (slen > num_sect
		      || biosdisk (BIOSDISK_READ, drive, &buf_geom,
				   sector, slen, BUFFERSEG))
		    errnum = ERR_READ;

		  bufaddr = BUFFERSEG + byte_offset;
		}
	    }
	  else
	    buf_track = track;
	}

#ifndef STAGE1_5
      /*
       *  Instrumentation to tell which sectors were read and used.
       */
      if (disk_read_hook && disk_read_func)
	{
	  int sector_end = sector + ((num_sect < slen) ? num_sect : slen);
	  int sector_num = sector;

	  while (sector_num < sector_end)
	    (*disk_read_func) (sector_num++);
	}
#endif /* STAGE1_5 */

      if (size > ((num_sect * SECTOR_SIZE) - byte_offset))
	size = (num_sect * SECTOR_SIZE) - byte_offset;

      memmove (buf, (char *) bufaddr, size);

      buf += size;
      byte_len -= size;
      sector += num_sect;
      slen -= num_sect;
      byte_offset = 0;
    }

  return (!errnum);
}


int
devread (int sector, int byte_offset, int byte_len, char *buf)
{
  /*
   *  Check partition boundaries
   */
  if (sector < 0
      || (sector + ((byte_offset + byte_len - 1) / SECTOR_SIZE)) >= part_length)
    {
      errnum = ERR_OUTSIDE_PART;
      return 0;
    }

  /*
   *  Get the read to the beginning of a partition.
   */
  while (byte_offset >= SECTOR_SIZE)
    {
      byte_offset -= SECTOR_SIZE;
      sector++;
    }

#if !defined(STAGE1_5)
  if (disk_read_hook && debug)
    printf ("<%d, %d, %d>", sector, byte_offset, byte_len);
#endif /* !STAGE1_5 */

  /*
   *  Call RAWREAD, which is very similar, but:
   *
   *    --  It takes an extra parameter, the drive number.
   *    --  It requires that "sector" is relative to the beginning
   *            of the disk.
   *    --  It doesn't handle offsets of more than 511 bytes into the
   *            sector.
   */
  return rawread (current_drive, part_start + sector, byte_offset,
		  byte_len, buf);
}

#ifndef STAGE1_5
static int
sane_partition (void)
{
  /* network drive */
  if (current_drive == NETWORK_DRIVE)
    return 1;
  
  if (!(current_partition & 0xFF000000uL)
      && (current_drive & 0xFFFFFF7F) < 8
      && (current_partition & 0xFF) == 0xFF
      && ((current_partition & 0xFF00) == 0xFF00
	  || (current_partition & 0xFF00) < 0x800)
      && ((current_partition >> 16) == 0xFF
	  || (current_drive & 0x80)))
    return 1;

  errnum = ERR_DEV_VALUES;
  return 0;
}
#endif /* ! STAGE1_5 */

static void
attempt_mount (void)
{
#ifndef STAGE1_5
  for (fsys_type = 0; fsys_type < NUM_FSYS
       && (*(fsys_table[fsys_type].mount_func)) () != 1; fsys_type++);

  if (fsys_type == NUM_FSYS && errnum == ERR_NONE)
    errnum = ERR_FSYS_MOUNT;
#else
  fsys_type = 0;
  if ((*(fsys_table[fsys_type].mount_func)) () != 1)
    {
      fsys_type = NUM_FSYS;
      errnum = ERR_FSYS_MOUNT;
    }
#endif
}


#ifndef STAGE1_5
int
make_saved_active (void)
{
  if (saved_drive & 0x80)
    {
      int part = saved_partition >> 16;

      if (part > 3)
	{
	  errnum = ERR_NO_PART;
	  return 0;
	}

      if (!rawread (saved_drive, 0, 0, SECTOR_SIZE, (char *) SCRATCHADDR))
	return 0;

      if (PC_SLICE_FLAG (SCRATCHADDR, part) != PC_SLICE_FLAG_BOOTABLE)
	{
	  int i;

	  for (i = 0; i < 4; i++)
	    PC_SLICE_FLAG (SCRATCHADDR, i) = 0;

	  PC_SLICE_FLAG (SCRATCHADDR, part) = PC_SLICE_FLAG_BOOTABLE;

	  buf_track = -1;

	  if (biosdisk (BIOSDISK_WRITE, saved_drive, &buf_geom,
			0, 1, SCRATCHSEG))
	    {
	      errnum = ERR_WRITE;
	      return 0;
	    }
	}
    }

  return 1;
}

int
set_partition_hidden_flag (int hidden)
{
  if (saved_drive)
    {
      int part = saved_partition >> 16;

      if (part > 3)
	{
          errnum = ERR_NO_PART;
          return 0;
        }

      if (! rawread (saved_drive, 0, 0, SECTOR_SIZE, (char *) SCRATCHADDR))
        return 0;

      if (hidden)
	PC_SLICE_TYPE (SCRATCHADDR, part) |= PC_SLICE_TYPE_HIDDEN_FLAG;
      else
	PC_SLICE_TYPE (SCRATCHADDR, part) &= ~PC_SLICE_TYPE_HIDDEN_FLAG;
      
      buf_track = -1;
      if (biosdisk (BIOSDISK_WRITE, saved_drive, &buf_geom,
		    0, 1, SCRATCHSEG))
	{
	  errnum = ERR_WRITE;
	  return 0;
	}
    }

  return 1;
}


static void
check_and_print_mount (void)
{
  attempt_mount ();
  if (errnum == ERR_FSYS_MOUNT)
    errnum = ERR_NONE;
  if (!errnum)
    print_fsys_type ();
  print_error ();
}
#endif /* STAGE1_5 */


static int
check_BSD_parts (int flags)
{
  char label_buf[SECTOR_SIZE];
  int part_no, got_part = 0;

  if (part_length < (BSD_LABEL_SECTOR + 1))
    {
      errnum = ERR_BAD_PART_TABLE;
      return 0;
    }

  if (!rawread (current_drive, part_start + BSD_LABEL_SECTOR,
		0, SECTOR_SIZE, label_buf))
    return 0;

  if (BSD_LABEL_CHECK_MAG (label_buf))
    {
      for (part_no = 0; part_no < BSD_LABEL_NPARTS (label_buf); part_no++)
	{
	  if (BSD_PART_TYPE (label_buf, part_no))
	    {
	      /* FIXME: should do BAD144 sector remapping setup here */

	      /* XXX We cannot determine which variant of BSD owns
		 this slice, so set it to FreeBSD paritition type.
		 That should work fine for now.  */
	      current_slice = ((BSD_PART_TYPE (label_buf, part_no) << 8)
			       | PC_SLICE_TYPE_FREEBSD);
	      part_start = BSD_PART_START (label_buf, part_no);
	      part_length = BSD_PART_LENGTH (label_buf, part_no);

#ifndef STAGE1_5
	      if (flags)
		{
		  if (!got_part)
		    {
		      if (! do_completion)
			printf ("[BSD sub-partitions immediately follow]\n");
		      got_part = 1;
		    }

		  if (! do_completion)
		    printf ("     BSD Partition num: \'%c\', ", part_no + 'a');
		  else
		    {
		      char str[16];

		      grub_sprintf (str, "%d,%c)",
				    (current_partition >> 16) & 0xFF,
				    part_no + 'a');
		      print_a_completion (str);
		    }
		  
		  check_and_print_mount ();
		}
	      else
#endif /* STAGE1_5 */
	      if (part_no == ((current_partition >> 8) & 0xFF))
		break;
	    }
	}

      if (part_no >= BSD_LABEL_NPARTS (label_buf) && !got_part)
	{
	  errnum = ERR_NO_PART;
	  return 0;
	}

      if ((current_drive & 0x80)
	  && BSD_LABEL_DTYPE (label_buf) == DTYPE_SCSI)
	bsd_evil_hack = 4;

      return 1;
    }

  errnum = ERR_BAD_PART_TABLE;
  return 0;
}

#ifndef STAGE1_5
static unsigned long cur_part_offset;
static unsigned long cur_part_addr;
#endif

int
real_open_partition (int flags)
{
  int i, part_no, slice_no, ext = 0;
  char mbr_buf[SECTOR_SIZE];
  
#ifndef STAGE1_5
  /* network drive */
  if (current_drive == NETWORK_DRIVE)
    return 1;
  
  if (! sane_partition ())
    return 0;
#endif

  /*
   *  The "rawread" is probably unnecessary here, but it is good to
   *  know it works.
   */
  if (! rawread (current_drive, 0, 0, SECTOR_SIZE, mbr_buf))
    return 0;

  bsd_evil_hack = 0;
  current_slice = 0;
  part_start = 0;
  part_length = buf_geom.total_sectors;

  if (current_drive & 0x80)
    {
      /*
       *  We're looking at a hard disk
       */

      int ext_offset = 0, part_offset = 0;
      part_no = (current_partition >> 16);
      slice_no = 0;

      /* if this is the whole disk, return here */
      if (! flags && current_partition == 0xFFFFFFuL)
	return 1;

      /*
       *  Load the current MBR-style PC partition table (4 entries)
       */
      while (slice_no < 255 && ext >= 0
	     && (part_no == 0xFF || slice_no <= part_no)
	     && rawread (current_drive, part_offset,
			 0, SECTOR_SIZE, mbr_buf))
	{
	  /*
	   *  If the table isn't valid, we can't continue
	   */
	  if (! PC_MBR_CHECK_SIG (mbr_buf))
	    {
	      errnum = ERR_BAD_PART_TABLE;
	      return 0;
	    }

	  ext = -1;

	  /*
	   *  Scan table for partitions
	   */
	  for (i = 0; i < PC_SLICE_MAX; i++)
	    {
	      current_partition = ((slice_no << 16)
				   | (current_partition & 0xFFFF));
	      current_slice = PC_SLICE_TYPE (mbr_buf, i);
	      part_start = part_offset + PC_SLICE_START (mbr_buf, i);
	      part_length = PC_SLICE_LENGTH (mbr_buf, i);
#ifndef STAGE1_5
	      cur_part_offset = part_offset;
	      cur_part_addr = BOOTSEC_LOCATION + PC_SLICE_OFFSET + (i << 4);
#endif

	      /*
	       *  Is this PC partition entry valid?
	       */
	      if (current_slice)
		{
#ifndef STAGE1_5
		  /*
		   *  Display partition information
		   */
		  if (flags && ! IS_PC_SLICE_TYPE_EXTENDED (current_slice))
		    {
		      current_partition |= 0xFFFF;
		      if (! do_completion)
			{
			  printf ("   Partition num: %d, ", slice_no);
		      
			  if (! IS_PC_SLICE_TYPE_BSD (current_slice))
			    check_and_print_mount ();
			  else
			    check_BSD_parts (1);
			}
		      else
			{
			  if (! IS_PC_SLICE_TYPE_BSD (current_slice))
			    {
			      char str[8];

			      grub_sprintf (str, "%d)", slice_no);
			      print_a_completion (str);
			    }
			  else
			    check_BSD_parts (1);
			}
		      
		      errnum = ERR_NONE;
		    }
#endif /* ! STAGE1_5 */
		  
		  /*
		   *  If we've found the right partition, we're done
		   */
		  if (! flags
		      && (slice_no < PC_SLICE_MAX
			  || ! IS_PC_SLICE_TYPE_EXTENDED (current_slice))
		      && (part_no == slice_no
			  || (part_no == 0xFF
			      && IS_PC_SLICE_TYPE_BSD (current_slice))))
		    {
		      if ((current_partition & 0xFF00) != 0xFF00)
			{
			  if (IS_PC_SLICE_TYPE_BSD (current_slice))
			    check_BSD_parts (0);
			  else
			    errnum = ERR_NO_PART;
			}

		      ext = -2;
		      break;
		    }

		  /*
		   *  Is this an extended partition?
		   */
		  if (IS_PC_SLICE_TYPE_EXTENDED (current_slice))
		    {
		      if (ext == -1)
			ext = i;
		    }
		}

	      /*
	       *  If we're beyond the end of the standard PC partition
	       *  range, change the numbering from one per table entry
	       *  to one per valid entry.
	       */
	      if (slice_no < PC_SLICE_MAX
		  || (! IS_PC_SLICE_TYPE_EXTENDED (current_slice)
		      && current_slice != PC_SLICE_TYPE_NONE))
		slice_no++;
	    }

	  part_offset = ext_offset + PC_SLICE_START (mbr_buf, ext);
	  if (! ext_offset)
	    ext_offset = part_offset;
	}
    }
  else
    {
      /*
       *  We're looking at a floppy disk
       */
      ext = -1;
      if ((flags || (current_partition & 0xFF00) != 0xFF00)
	  && check_BSD_parts (flags))
	ext = -2;
      else
	{
	  errnum = 0;
	  if (!flags)
	    {
	      if (current_partition == 0xFFFFFF
		  || current_partition == 0xFF00FF)
		{
		  current_partition = 0xFFFFFF;
		  ext = -2;
		}
	    }
#ifndef STAGE1_5
	  else
	    {
	      current_partition = 0xFFFFFF;
	      check_and_print_mount ();
	      errnum = 0;
	    }
#endif /* STAGE1_5 */
	}
    }

  if (!flags && (ext != -2) && (errnum == ERR_NONE))
    errnum = ERR_NO_PART;

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}


int
open_partition (void)
{
  return real_open_partition (0);
}


#ifndef STAGE1_5
/* XX used for device completion in 'set_device' and 'print_completions' */
static int incomplete, disk_choice;
static enum
{
  PART_UNSPECIFIED = 0,
  PART_DISK,
  PART_CHOSEN,
}
part_choice;
#endif /* ! STAGE1_5 */

char *
set_device (char *device)
{
#ifdef STAGE1_5
    /* In Stage 1.5, the first 4 bytes of FILENAME has a device number.  */
  unsigned long dev = *((unsigned long *) device);
  int drive = (dev >> 24) & 0xFF;
  int partition = dev & 0xFFFFFF;

  /* If DRIVE is disabled (0xFF), use SAVED_DRIVE instead.  */
  if (drive == 0xFF)
    current_drive = saved_drive;
  else
    current_drive = drive;

  /* The `partition' part must always have a valid number.  */
  current_partition = partition;
  
  return device + sizeof (unsigned long);
  
#else /* ! STAGE1_5 */
  
  int result = 0;

  incomplete = 0;
  disk_choice = 1;
  part_choice = PART_UNSPECIFIED;
  current_drive = saved_drive;
  current_partition = 0xFFFFFF;

  if (*device == '(' && !*(device + 1))
    /* user has given '(' only, let disk_choice handle what disks we have */
    return device + 1;

  if (*device == '(' && *(++device))
    {
      if (*device != ',' && *device != ')')
	{
	  char ch = *device;

	  if (*device == 'f' || *device == 'h')
	    {
	      /* user has given '([fh]', check for resp. add 'd' and
		 let disk_choice handle what disks we have */
	      if (!*(device + 1))
		{
		  device++;
		  *device++ = 'd';
		  *device = '\0';
		  return device;
		}
	      else if (*(device + 1) == 'd' && !*(device + 2))
		return device + 2;
	    }

	  if ((*device == 'f' || *device == 'h' || *device == 'n')
	      && (device += 2, (*(device - 1) != 'd')))
	    errnum = ERR_NUMBER_PARSING;

	  if (ch == 'n')
	    current_drive = NETWORK_DRIVE;
	  else
	    {
	      safe_parse_maxint (&device, (int *) &current_drive);
	      
	      disk_choice = 0;
	      if (ch == 'h')
		current_drive += 0x80;
	    }
	}

      if (errnum)
	return 0;

      if (*device == ')')
	{
	  part_choice = PART_CHOSEN;
	  result = 1;
	}
      else if (*device == ',')
	{
	  /* Either an absolute PC or BSD partition. */
	  disk_choice = 0;
	  part_choice ++;
	  device++;

	  if (*device >= '0' && *device <= '9')
	    {
	      part_choice ++;
	      current_partition = 0;

	      if (!(current_drive & 0x80)
		  || !safe_parse_maxint (&device, (int *) &current_partition)
		  || current_partition > 254)
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      current_partition = (current_partition << 16) + 0xFFFF;

	      if (*device == ','
		  && *(device + 1) >= 'a' && *(device + 1) <= 'h')
		{
		  device++;
		  current_partition = (((*(device++) - 'a') << 8)
				       | (current_partition & 0xFF00FF));
		}
	    }
	  else if (*device >= 'a' && *device <= 'h')
	    {
	      part_choice ++;
	      current_partition = ((*(device++) - 'a') << 8) | 0xFF00FF;
	    }

	  if (*device == ')')
	    {
	      if (part_choice == PART_DISK)
		{
		  current_partition = saved_partition;
		  part_choice ++;
		}

	      result = 1;
	    }
	}
    }

  if (! sane_partition ())
    return 0;
  
  if (result)
    return device + 1;
  else
    {
      if (!*device)
	incomplete = 1;
      errnum = ERR_DEV_FORMAT;
    }

  return 0;
  
#endif /* ! STAGE1_5 */
}

/*
 *  This performs a "mount" on the current device, both drive and partition
 *  number.
 */

int
open_device (void)
{
  if (open_partition ())
    attempt_mount ();

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}


#ifndef STAGE1_5
int
set_bootdev (int hdbias)
{
  int i, j;

  /* Save the boot partition for chain-loading.  */
  boot_part_offset = cur_part_offset;
  boot_part_addr = cur_part_addr;
  
  /*
   *  Set BSD boot device.
   */
  i = (saved_partition >> 16) + 2;
  if (saved_partition == 0xFFFFFF)
    i = 1;
  else if ((saved_partition >> 16) == 0xFF)
    i = 0;

  /* FIXME: extremely evil hack!!! */
  j = 2;
  if (saved_drive & 0x80)
    j = bsd_evil_hack;

  return MAKEBOOTDEV (j, (i >> 4), (i & 0xF),
		      ((saved_drive - hdbias) & 0x7F),
		      ((saved_partition >> 8) & 0xFF));
}
#endif /* STAGE1_5 */


static char *
setup_part (char *filename)
{
#ifdef STAGE1_5

  if (! (filename = set_device (filename)))
    {
      current_drive = 0xFF;
      return 0;
    }
  
# ifndef NO_BLOCK_FILES
  if (*filename != '/')
    open_partition ();
  else
# endif /* ! NO_BLOCK_FILES */
    open_device ();
  
#else /* ! STAGE1_5 */
  
  if (*filename == '(')
    {
      if ((filename = set_device (filename)) == 0)
	{
	  current_drive = 0xFF;
	  return 0;
	}
# ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
# endif /* ! NO_BLOCK_FILES */
	open_device ();
    }
  else if (saved_drive != current_drive
	   || saved_partition != current_partition
	   || (*filename == '/' && fsys_type == NUM_FSYS)
	   || buf_drive == -1)
    {
      current_drive = saved_drive;
      current_partition = saved_partition;
      /* allow for the error case of "no filesystem" after the partition
         is found.  This makes block files work fine on no filesystem */
# ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
# endif /* ! NO_BLOCK_FILES */
	open_device ();
    }
  
#endif /* ! STAGE1_5 */
  
  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;
  else
    errnum = 0;

#ifndef STAGE1_5
  if (!sane_partition ())
    return 0;
#endif

  return filename;
}


#ifndef STAGE1_5
/*
 *  This prints the filesystem type or gives relevant information.
 */

void
print_fsys_type (void)
{
  if (! do_completion)
    {
      printf (" Filesystem type ");
      
      if (fsys_type != NUM_FSYS)
	printf ("is %s, ", fsys_table[fsys_type].name);
      else
	printf ("unknown, ");
      
      if (current_partition == 0xFFFFFF)
	printf ("using whole disk\n");
      else
	printf ("partition type 0x%x\n", current_slice);
    }
}
#endif /* STAGE1_5 */

#ifndef STAGE1_5
/* If DO_COMPLETION is true, just print NAME. Otherwise save the unique
   part into UNIQUE_STRING.  */
void
print_a_completion (char *name)
{
  /* If NAME is "." or "..", do not count it.  */
  if (grub_strcmp (name, ".") == 0 || grub_strcmp (name, "..") == 0)
    return;
  
  if (do_completion)
    {
      char *buf = unique_string;
      
      if (! unique)
	while ((*buf++ = *name++))
	  ;
      else
	{
	  while (*buf && (*buf == *name))
	    {
	      buf++;
	      name++;
	    }
	  /* mismatch, strip it.  */
	  *buf = '\0';
	}
    }
  else
    grub_printf (" %s", name);
  
  unique++;
}

/*
 *  This lists the possible completions of a device string, filename, or
 *  any sane combination of the two.
 */

int
print_completions (int is_filename, int is_completion)
{
  char *buf = (char *) COMPLETION_BUF;
  char *ptr = buf;

  unique_string = (char *) UNIQUE_BUF;
  *unique_string = 0;
  unique = 0;
  do_completion = is_completion;

  if (! is_filename)
    {
      /* Print the completions of builtin commands.  */
      struct builtin **builtin;

      if (! is_completion)
	grub_printf (" Possible commands are:");
      
      for (builtin = builtin_table; (*builtin); builtin++)
	{
	  /* If *BUILTIN cannot be run in the command-line, skip it.  */
	  if (! ((*builtin)->flags & BUILTIN_CMDLINE))
	    continue;

	  if (substring (buf, (*builtin)->name) <= 0)
	    print_a_completion ((*builtin)->name);
	}

      if (is_completion && *unique_string)
	{
	  if (unique == 1)
	    {
	      char *u = unique_string + grub_strlen (unique_string);

	      *u++ = ' ';
	      *u = 0;
	    }
	  
	  grub_strcpy (buf, unique_string);
	}

      if (! is_completion)
	grub_putchar ('\n');
      
      print_error ();
      do_completion = 0;
      if (errnum)
	return -1;
      else
	return unique - 1;
    }

  if (*buf == '/' || (ptr = set_device (buf)) || incomplete)
    {
      errnum = 0;

      if (*buf == '(' && (incomplete || ! *ptr))
	{
	  if (! part_choice)
	    {
	      /* disk completions */
	      int disk_no, i, j;
	      struct geometry geom;

	      if (! is_completion)
		grub_printf (" Possible disks are: ");

	      for (i = (ptr && (*(ptr-2) == 'h' && *(ptr-1) == 'd') ? 1 : 0);
		   i < (ptr && (*(ptr-2) == 'f' && *(ptr-1) == 'd') ? 1 : 2);
		   i++)
		{
		  for (j = 0; j < 8; j++)
		    {
		      disk_no = (i * 0x80) + j;
		      if ((disk_choice || disk_no == current_drive)
			  && ! get_diskinfo (disk_no, &geom))
			{
			  char dev_name[8];

			  grub_sprintf (dev_name, "%cd%d", i ? 'h' : 'f', j);
			  print_a_completion (dev_name);
			}
		    }
		}

	      if (is_completion && *unique_string)
		{
		  ptr = buf;
		  while (*ptr != '(')
		    ptr--;
		  ptr++;
		  grub_strcpy (ptr, unique_string);
		  if (unique == 1)
		    {
		      ptr += grub_strlen (ptr);
		      if (*unique_string == 'h')
			{
			  *ptr++ = ',';
			  *ptr = 0;
			}
		      else
			{
			  *ptr++ = ')';
			  *ptr = 0;
			}
		    }
		}

	      if (! is_completion)
		grub_putchar ('\n');
	    }
	  else
	    {
	      /* partition completions */
	      if (part_choice == PART_DISK)
		{
		  if (! is_completion)
		    grub_printf (" Possible partitions are:\n");
		  real_open_partition (1);
		  
		  if (is_completion && *unique_string)
		    {
		      ptr = buf;
		      while (*ptr++ != ',')
			;
		      grub_strcpy (ptr, unique_string);
		    }
		}
	      else
		{
		  if (open_partition ())
		    {
		      unique = 1;
		      ptr = buf + grub_strlen (buf);
		      if (*(ptr - 1) != ')')
			{
			  *ptr++ = ')';
			  *ptr = 0;
			}
		    }
		}
	    }
	}
      else if (ptr && *ptr == '/')
	{
	  /* filename completions */
	  if (! is_completion)
	    grub_printf (" Possible files are:");
	  
	  dir (buf);
	  
	  if (is_completion && *unique_string)
	    {
	      ptr += grub_strlen (ptr);
	      while (*ptr != '/')
		ptr--;
	      ptr++;
	      
	      grub_strcpy (ptr, unique_string);
	      
	      if (unique == 1)
		{
		  ptr += grub_strlen (unique_string);

		  /* Check if the file UNIQUE_STRING is a directory.  */
		  *ptr = '/';
		  *(ptr + 1) = 0;
		  
		  dir (buf);
		  
		  /* Restore the original unique value.  */
		  unique = 1;
		  
		  if (errnum)
		    {
		      /* Regular file */
		      errnum = 0;
		      *ptr = ' ';
		      *(ptr + 1) = 0;
		    }
		}
	    }
	  
	  if (! is_completion)
	    grub_putchar ('\n');
	}
      else
	errnum = ERR_BAD_FILENAME;
    }

  print_error ();
  do_completion = 0;
  if (errnum)
    return -1;
  else
    return unique - 1;
}
#endif /* STAGE1_5 */


/*
 *  This is the generic file open function.
 */

int
grub_open (char *filename)
{
#ifndef NO_DECOMPRESSION
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

  /* if any "dir" function uses/sets filepos, it must
     set it to zero before returning if opening a file! */
  filepos = 0;

  if (!(filename = setup_part (filename)))
    return 0;

#ifndef NO_BLOCK_FILES
  block_file = 0;
#endif /* NO_BLOCK_FILES */

  /* This accounts for partial filesystem implementations. */
  fsmax = MAXINT;

  if (*filename != '/')
    {
#ifndef NO_BLOCK_FILES
      char *ptr = filename;
      int tmp, list_addr = BLK_BLKLIST_START;
      filemax = 0;

      while (list_addr < BLK_MAX_ADDR)
	{
	  tmp = 0;
	  safe_parse_maxint (&ptr, &tmp);
	  errnum = 0;

	  if (*ptr != '+')
	    {
	      if ((*ptr && *ptr != '/' && !isspace (*ptr))
		  || tmp == 0 || tmp > filemax)
		errnum = ERR_BAD_FILENAME;
	      else
		filemax = tmp;

	      break;
	    }

	  /* since we use the same filesystem buffer, mark it to
	     be remounted */
	  fsys_type = NUM_FSYS;

	  BLK_BLKSTART (list_addr) = tmp;
	  ptr++;

	  if (!safe_parse_maxint (&ptr, &tmp)
	      || tmp == 0
	      || (*ptr && *ptr != ',' && *ptr != '/' && !isspace (*ptr)))
	    {
	      errnum = ERR_BAD_FILENAME;
	      break;
	    }

	  BLK_BLKLENGTH (list_addr) = tmp;

	  filemax += (tmp * SECTOR_SIZE);
	  list_addr += BLK_BLKLIST_INC_VAL;

	  if (*ptr != ',')
	    break;

	  ptr++;
	}

      if (list_addr < BLK_MAX_ADDR && ptr != filename && !errnum)
	{
	  block_file = 1;
	  BLK_CUR_FILEPOS = 0;
	  BLK_CUR_BLKLIST = BLK_BLKLIST_START;
	  BLK_CUR_BLKNUM = 0;

#ifndef NO_DECOMPRESSION
	  return gunzip_test_header ();
#else /* NO_DECOMPRESSION */
	  return 1;
#endif /* NO_DECOMPRESSION */
	}
#else /* NO_BLOCK_FILES */
      errnum = ERR_BAD_FILENAME;
#endif /* NO_BLOCK_FILES */
    }

  if (!errnum && fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

# ifndef STAGE1_5
  /* set "dir" function to open a file */
  print_possibilities = 0;
# endif

  if (!errnum && (*(fsys_table[fsys_type].dir_func)) (filename))
    {
#ifndef NO_DECOMPRESSION
      return gunzip_test_header ();
#else /* NO_DECOMPRESSION */
      return 1;
#endif /* NO_DECOMPRESSION */
    }

  return 0;
}


int
grub_read (char *buf, int len)
{
  /* Make sure "filepos" is a sane value */
  if ((filepos < 0) || (filepos > filemax))
    filepos = filemax;

  /* Make sure "len" is a sane value */
  if ((len < 0) || (len > (filemax - filepos)))
    len = filemax - filepos;

  /* if target file position is past the end of
     the supported/configured filesize, then
     there is an error */
  if (filepos + len > fsmax)
    {
      errnum = ERR_FILELENGTH;
      return 0;
    }

#ifndef NO_DECOMPRESSION
  if (compressed_file)
    return gunzip_read (buf, len);
#endif /* NO_DECOMPRESSION */

#ifndef NO_BLOCK_FILES
  if (block_file)
    {
      int size, off, ret = 0;

      while (len && !errnum)
	{
	  /* we may need to look for the right block in the list(s) */
	  if (filepos < BLK_CUR_FILEPOS)
	    {
	      BLK_CUR_FILEPOS = 0;
	      BLK_CUR_BLKLIST = BLK_BLKLIST_START;
	      BLK_CUR_BLKNUM = 0;
	    }

	  /* run BLK_CUR_FILEPOS up to filepos */
	  while (filepos > BLK_CUR_FILEPOS)
	    {
	      if ((filepos - (BLK_CUR_FILEPOS & ~(SECTOR_SIZE - 1)))
		  >= SECTOR_SIZE)
		{
		  BLK_CUR_FILEPOS += SECTOR_SIZE;
		  BLK_CUR_BLKNUM++;

		  if (BLK_CUR_BLKNUM >= BLK_BLKLENGTH (BLK_CUR_BLKLIST))
		    {
		      BLK_CUR_BLKLIST += BLK_BLKLIST_INC_VAL;
		      BLK_CUR_BLKNUM = 0;
		    }
		}
	      else
		BLK_CUR_FILEPOS = filepos;
	    }

	  off = filepos & (SECTOR_SIZE - 1);
	  size = ((BLK_BLKLENGTH (BLK_CUR_BLKLIST) - BLK_CUR_BLKNUM)
		  * SECTOR_SIZE) - off;
	  if (size > len)
	    size = len;

#ifndef STAGE1_5
	  disk_read_func = disk_read_hook;
#endif /* STAGE1_5 */

	  /* read current block and put it in the right place in memory */
	  devread (BLK_BLKSTART (BLK_CUR_BLKLIST) + BLK_CUR_BLKNUM,
		   off, size, buf);

#ifndef STAGE1_5
	  disk_read_func = NULL;
#endif /* STAGE1_5 */

	  len -= size;
	  filepos += size;
	  ret += size;
	  buf += size;
	}

      if (errnum)
	ret = 0;

      return ret;
    }
#endif /* NO_BLOCK_FILES */

  if (fsys_type == NUM_FSYS)
    {
      errnum = ERR_FSYS_MOUNT;
      return 0;
    }

  return (*(fsys_table[fsys_type].read_func)) (buf, len);
}

#ifndef STAGE1_5
/* Reposition a file offset.  */
int
grub_seek (int offset)
{
  if (offset > filemax || offset < 0)
    return -1;

  filepos = offset;
  return offset;
}

int
dir (char *dirname)
{
#ifndef NO_DECOMPRESSION
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

  if (!(dirname = setup_part (dirname)))
    return 0;

  if (*dirname != '/')
    errnum = ERR_BAD_FILENAME;

  if (fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  if (errnum)
    return 0;

  /* set "dir" function to list completions */
  print_possibilities = 1;

  return (*(fsys_table[fsys_type].dir_func)) (dirname);
}
#endif /* STAGE1_5 */

void 
grub_close (void)
{
  if (fsys_type == NUM_FSYS)
    return;
  
  if (fsys_table[fsys_type].close_func == 0)
    return;
  
  (*(fsys_table[fsys_type].close_func)) ();
}
