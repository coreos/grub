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


#define _DISK_IO_C

#include "shared.h"

#include "filesys.h"

/* XXX for evil hack */
#include "freebsd.h"

#ifndef STAGE1_5
/* instrumentation variables */
void (*debug_fs) (int) = NULL;
void (*debug_fs_func) (int) = NULL;
#endif /* STAGE1_5 */

/* These have the same format as "boot_drive" and "install_partition", but
   are meant to be working values. */
unsigned long current_drive = 0xFF;
unsigned long current_partition;

/*
 *  Global variables describing details of the filesystem
 */

/* XXX BSD evil hack */
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
int buf_geom;

/* filesystem common variables */
int filepos;
int filemax;


int
rawread (int drive, int sector, int byte_offset, int byte_len, int addr)
{
  int slen = (byte_offset + byte_len + 511) / SECTOR_SIZE;

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
	  buf_geom = get_diskinfo (drive);
	  buf_drive = drive;
	  buf_track = -1;
	}

      if (buf_geom == 0)
	{
	  errnum = ERR_NO_DISK;
	  return 0;
	}

      /*  Get first sector of track  */
      soff = sector % SECTORS (buf_geom);
      track = sector - soff;
      num_sect = SECTORS (buf_geom) - soff;
      bufaddr = BUFFERADDR + (soff * SECTOR_SIZE) + byte_offset;

      if (track != buf_track)
	{
	  int bios_err, read_start = track, read_len = SECTORS (buf_geom);

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

	  if (bios_err = biosdisk (BIOSDISK_READ, drive, buf_geom,
				   read_start, read_len, BUFFERSEG))
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
		      || biosdisk (BIOSDISK_READ, drive, buf_geom,
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
      if (debug_fs && debug_fs_func)
	{
	  int sector_end = sector + ((num_sect < slen) ? num_sect : slen);
	  int sector_num = sector;

	  while (sector_num < sector_end)
	    (*debug_fs_func) (sector_num++);
	}
#endif /* STAGE1_5 */

      if (size > ((num_sect * SECTOR_SIZE) - byte_offset))
	size = (num_sect * SECTOR_SIZE) - byte_offset;

      bcopy ((char *) bufaddr, (char *) addr, size);

      addr += size;
      byte_len -= size;
      sector += num_sect;
      slen -= num_sect;
      byte_offset = 0;
    }

  return (!errnum);
}


int
devread (int sector, int byte_offset, int byte_len, int addr)
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

#if !defined(STAGE1_5) && defined(DEBUG)
  if (debug_fs)
    printf ("<%d, %d, %d>", sector, byte_offset, byte_len);
#endif /* !STAGE1_5 && DEBUG */

  /*
   *  Call "rawread", which is very similar, but:
   *
   *    --  It takes an extra parameter, the drive number.
   *    --  It requires that "sector" is relative to the beginning
   *            of the disk.
   *    --  It doesn't handle offsets of more than 511 bytes into the
   *            sector.
   */
  return rawread (current_drive, part_start + sector, byte_offset,
		  byte_len, addr);
}


int
sane_partition (void)
{
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


static void
attempt_mount (void)
{
  for (fsys_type = 0; fsys_type < NUM_FSYS
       && (*(fsys_table[fsys_type].mount_func)) () != 1; fsys_type++);

  if (fsys_type == NUM_FSYS && errnum == ERR_NONE)
    errnum = ERR_FSYS_MOUNT;
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

      if (!rawread (saved_drive, 0, 0, SECTOR_SIZE, SCRATCHADDR))
	return 0;

      if (PC_SLICE_FLAG (SCRATCHADDR, part) != PC_SLICE_FLAG_BOOTABLE)
	{
	  int i;

	  for (i = 0; i < 4; i++)
	    PC_SLICE_FLAG (SCRATCHADDR, i) = 0;

	  PC_SLICE_FLAG (SCRATCHADDR, part) = PC_SLICE_FLAG_BOOTABLE;

	  buf_track = -1;

	  if (biosdisk (BIOSDISK_WRITE, saved_drive, buf_geom,
			0, 1, SCRATCHSEG))
	    {
	      errnum = ERR_WRITE;
	      return 0;
	    }
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
		0, SECTOR_SIZE, (int) label_buf))
    return 0;

  if (BSD_LABEL_CHECK_MAG (label_buf))
    {
      for (part_no = 0; part_no < BSD_LABEL_NPARTS (label_buf); part_no++)
	{
	  if (BSD_PART_TYPE (label_buf, part_no))
	    {
	      /* XXX should do BAD144 sector remapping setup here */

	      current_slice = ((BSD_PART_TYPE (label_buf, part_no) << 8)
			       | PC_SLICE_TYPE_BSD);
	      part_start = BSD_PART_START (label_buf, part_no);
	      part_length = BSD_PART_LENGTH (label_buf, part_no);

#ifndef STAGE1_5
	      if (flags)
		{
		  if (!got_part)
		    {
		      printf ("[BSD sub-partitions immediately follow]\n");
		      got_part = 1;
		    }
		  printf ("     BSD Partition num: \'%c\', ", part_no + 'a');
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


#if !defined(STAGE1_5) || !defined(NO_BLOCK_FILES)
static char cur_part_desc[16];

static int
real_open_partition (int flags)
{
  char mbr_buf[SECTOR_SIZE];
  int i, part_no, slice_no, ext = 0, part_offset = 0;

  /*
   *  The "rawread" is probably unnecessary here, but it is good to
   *  know it works.
   */
  if (!sane_partition ()
      || !rawread (current_drive, 0, 0, SECTOR_SIZE, (int) mbr_buf))
    return 0;

  bsd_evil_hack = 0;
  current_slice = 0;
  part_start = 0;
  part_length = SECTORS (buf_geom) * HEADS (buf_geom) * CYLINDERS (buf_geom);

  if (current_drive & 0x80)
    {
      /*
       *  We're looking at a hard disk
       */

      int ext_offset = 0, part_offset = 0;
      part_no = (current_partition >> 16);
      slice_no = 0;

      /* if this is the whole disk, return here */
      if (!flags && current_partition == 0xFFFFFFuL)
	return 1;

      /*
       *  Load the current MBR-style PC partition table (4 entries)
       */
      while (slice_no < 255 && ext >= 0
	     && (part_no == 0xFF || slice_no <= part_no)
	     && rawread (current_drive, part_offset,
			 0, SECTOR_SIZE, (int) mbr_buf))
	{
	  /*
	   *  If the table isn't valid, we can't continue
	   */
	  if (!PC_MBR_CHECK_SIG (mbr_buf))
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
	      bcopy (mbr_buf + PC_SLICE_OFFSET + (i << 4), cur_part_desc, 16);

	      /*
	       *  Is this PC partition entry valid?
	       */
	      if (current_slice)
		{
		  /*
		   *  Is this an extended partition?
		   */
		  if (current_slice == PC_SLICE_TYPE_EXTENDED)
		    {
		      if (ext == -1)
			{
			  ext = i;
			}
		    }
# ifndef STAGE1_5
		  /*
		   *  Display partition information
		   */
		  else if (flags)
		    {
		      current_partition |= 0xFFFF;
		      printf ("   Partition num: %d, ", slice_no);
		      if (current_slice != PC_SLICE_TYPE_BSD)
			check_and_print_mount ();
		      else
			check_BSD_parts (1);
		      errnum = ERR_NONE;
		    }
# endif /* STAGE1_5 */
		  /*
		   *  If we've found the right partition, we're done
		   */
		  else if (part_no == slice_no
			   || (part_no == 0xFF
			       && current_slice == PC_SLICE_TYPE_BSD))
		    {
		      if ((current_partition & 0xFF00) != 0xFF00)
			{
			  if (current_slice == PC_SLICE_TYPE_BSD)
			    check_BSD_parts (0);
			  else
			    errnum = ERR_NO_PART;
			}

		      ext = -2;
		      break;
		    }
		}

	      /*
	       *  If we're beyond the end of the standard PC partition
	       *  range, change the numbering from one per table entry
	       *  to one per valid entry.
	       */
	      if (slice_no < PC_SLICE_MAX
		  || (current_slice != PC_SLICE_TYPE_EXTENDED
		      && current_slice != PC_SLICE_TYPE_NONE))
		slice_no++;
	    }

	  part_offset = ext_offset + PC_SLICE_START (mbr_buf, ext);
	  if (!ext_offset)
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
		  current_partition == 0xFFFFFF;
		  ext = -2;
		}
	    }
# ifndef STAGE1_5
	  else
	    {
	      current_partition = 0xFFFFFF;
	      check_and_print_mount ();
	      errnum = 0;
	    }
# endif /* STAGE1_5 */
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
#endif /* !defined(STAGE1_5) || !defined(NO_BLOCK_FILES) */


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


char *
set_device (char *device)
{
  /* The use of retval in this function is not really clean, but it works */
  char *retval = 0;

  incomplete = 0;
  disk_choice = 1;
  part_choice = PART_UNSPECIFIED;
  current_drive = saved_drive;
  current_partition = 0xFFFFFF;

  if (*device == '(' && *(++device))
    {
      if (*device != ',' && *device != ')')
	{
	  char ch = *device;

	  if ((*device == 'f' || *device == 'h')
	      && (device += 2, (*(device - 1) != 'd')))
	    errnum = ERR_NUMBER_PARSING;

	  safe_parse_maxint (&device, (int *) &current_drive);

	  disk_choice = 0;
	  if (ch == 'h')
	    current_drive += 0x80;
	}

      if (errnum)
	return 0;

      if (*device == ')')
	{
	  part_choice = PART_CHOSEN;
	  retval++;
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

	      retval++;
	    }
	}
    }
  else
    {
      char ch;

      /* A Mach-style absolute partition name. */
      ch = *device;
      if (*device != 'f' && *device != 'h' ||
	  (device += 2, (*(device - 1) != 'd')))
	errnum = ERR_DEV_FORMAT;
      else
	safe_parse_maxint (&device, (int *) &current_drive);

      disk_choice = 0;
      if (ch != 'f')
	current_drive += 0x80;

      if (errnum)
	return 0;

      if (*device == '/')
	{
	  part_choice = PART_CHOSEN;
	  retval ++;
	}
      else if (*device == 's')
	{
	  /* An absolute PC partition. */
	  disk_choice = 0;
	  part_choice ++;
	  device ++;

	  if (*device >= '0' && *device <= '9')
	    {
	      part_choice ++;
	      current_partition = 0;

	      if (!(current_drive & 0x80) ||
		  !safe_parse_maxint (&device, (int *) &current_partition) ||
		  (--current_partition) > 254)
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      current_partition = (current_partition << 16) + 0xFFFF;

	      if (*device >= 'a' && *device <= 'h')
		{
		  /* A BSD partition within the slice. */
		  current_partition = (((*(device ++) - 'a') << 8)
				       | (current_partition & 0xFF00FF));
		}
	    }
	}
      else if (*device >= 'a' && *device <= 'h')
	{
	  /* An absolute BSD partition. */
	  part_choice ++;
	  current_partition = ((*(device ++) - 'a') << 8) | 0xFF00FF;
	}

      if (*device == '/')
	{
	  if (part_choice == PART_DISK)
	    {
	      current_partition = saved_partition;
	      part_choice ++;
	    }

	  retval ++;
	}
    }

  if (retval)
    retval = device + 1;
  else
    {
      if (!*device)
	incomplete = 1;
      errnum = ERR_DEV_FORMAT;
    }

  return retval;
}
#endif /* STAGE1_5 */

#ifndef STAGE1_5
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
#endif /* STAGE1_5 */


#ifndef STAGE1_5
int
set_bootdev (int hdbias)
{
  int i, j;

  /*
   *  Set chainloader boot device.
   */
  bcopy (cur_part_desc, (char *) (BOOTSEC_LOCATION - 16), 16);

  /*
   *  Set BSD boot device.
   */
  i = (saved_partition >> 16) + 2;
  if (saved_partition == 0xFFFFFF)
    i = 1;
  else if ((saved_partition >> 16) == 0xFF)
    i = 0;

  /* XXX extremely evil hack!!! */
  j = 2;
  if (saved_drive & 0x80)
    j = bsd_evil_hack;

  return MAKEBOOTDEV (j, (i >> 4), (i & 0xF),
		      ((saved_drive - hdbias) & 0x79),
		      ((saved_partition >> 8) & 0xFF));
}
#endif /* STAGE1_5 */


#ifndef STAGE1_5
static char *
setup_part (char *filename)
{
  /* FIXME: decide on syntax for blocklist vs. old-style vs. /dev/hd0s1 */
  /* Strip any leading /dev. */
  if (substring ("/dev/", filename) < 1)
    filename += 5;

  if (*filename == '(')
    {
      if ((filename = set_device (filename)) == 0)
	{
	  current_drive = 0xFF;
	  return 0;
	}
#ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
#endif /* NO_BLOCK_FILES */
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
#ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
#endif /* NO_BLOCK_FILES */
	open_device ();
    }

  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;
  else
    errnum = 0;

  if (!sane_partition ())
    return 0;

  return filename;
}
#endif /* STAGE1_5 */


#ifndef STAGE1_5
/*
 *  This prints the filesystem type or gives relevant information.
 */

void
print_fsys_type (void)
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
#endif /* STAGE1_5 */

#ifndef STAGE1_5
/*
 *  This lists the possible completions of a device string, filename, or
 *  any sane combination of the two.
 */

void
print_completions (char *filename)
{
  char *ptr = filename;

  if (*filename == '/' || (ptr = set_device (filename)) || incomplete)
    {
      errnum = 0;

      if (*filename != '/' && (incomplete || !*ptr))
	{
	  if (!part_choice)
	    {
	      /* disk completions */
	      int disk_no, i, j;

	      printf (" Possible disks are: ");

	      for (i = 0; i < 2; i++)
		{
		  for (j = 0; j < 8; j++)
		    {
		      disk_no = (i * 0x80) + j;
		      if ((disk_choice || disk_no == current_drive)
			  && get_diskinfo (disk_no))
			printf (" %cd%d", (i ? 'h' : 'f'), j);
		    }
		}

	      putchar ('\n');
	    }
	  else
	    {
	      /* partition completions */
	      if (part_choice == PART_DISK)
		{
		  printf (" Possible partitions are:\n");
		  real_open_partition (1);
		}
	      else
		{
		  if (open_partition ())
		    check_and_print_mount ();
		}
	    }
	}
      else if (*ptr == '/')
	{
	  /* filename completions */
	  printf (" Possible files are:");
	  dir (filename);
	}
      else
	errnum = ERR_BAD_FILENAME;
    }

  print_error ();
}
#endif /* STAGE1_5 */


/*
 *  This is the generic file open function.
 */

int
open (char *filename)
{
#ifndef NO_DECOMPRESSION
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

  /* if any "dir" function uses/sets filepos, it must
     set it to zero before returning if opening a file! */
  filepos = 0;

#ifndef STAGE1_5
  if (!(filename = setup_part (filename)))
    return 0;
#endif /* STAGE1_5 */

#ifndef NO_BLOCK_FILES
  block_file = 0;
#endif /* NO_BLOCK_FILES */

  /* XXX to account for partial filesystem implementations! */
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
read (int addr, int len)
{
  /* Make sure "filepos" is a sane value */
  if ((filepos < 0) | (filepos > filemax))
    filepos = filemax;

  /* Make sure "len" is a sane value */
  if ((len < 0) | (len > (filemax - filepos)))
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
    return gunzip_read (addr, len);
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
	  debug_fs_func = debug_fs;
#endif /* STAGE1_5 */

	  /* read current block and put it in the right place in memory */
	  devread (BLK_BLKSTART (BLK_CUR_BLKLIST) + BLK_CUR_BLKNUM,
		   off, size, addr);

#ifndef STAGE1_5
	  debug_fs_func = NULL;
#endif /* STAGE1_5 */

	  len -= size;
	  filepos += size;
	  ret += size;
	  addr += size;
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

  return (*(fsys_table[fsys_type].read_func)) (addr, len);
}


#ifndef STAGE1_5
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
