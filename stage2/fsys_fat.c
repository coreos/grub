/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
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

#ifdef FSYS_FAT

#include "shared.h"

#include "filesys.h"

#include "fat.h"

static int num_clust;
static int mapblock;
static int data_offset;
static int fat_size;
static int root_dir;

/* pointer(s) into filesystem info buffer for DOS stuff */
#define BPB     ( FSYS_BUF + 32256 )	/* 512 bytes long */
#define FAT_BUF ( FSYS_BUF + 30208 )	/* 4 sector FAT buffer */

int
fat_mount (void)
{
  int retval = 1;

  if ((((current_drive & 0x80) || (current_slice != 0))
       && ! IS_PC_SLICE_TYPE_FAT (current_slice)
       && (! IS_PC_SLICE_TYPE_BSD_WITH_FS (current_slice, FS_MSDOS)))
      || !devread (0, 0, SECTOR_SIZE, (char *) BPB)
      || FAT_BPB_BYTES_PER_SECTOR (BPB) != SECTOR_SIZE
      || FAT_BPB_SECT_PER_CLUST (BPB) < 1
      || (FAT_BPB_SECT_PER_CLUST (BPB) & (FAT_BPB_SECT_PER_CLUST (BPB) - 1))
      || !((current_drive & 0x80)
	   || FAT_BPB_FLOPPY_NUM_SECTORS (BPB)))
    retval = 0;
  else
    {
      mapblock = -4096;
      data_offset = FAT_BPB_DATA_OFFSET (BPB);
      num_clust = FAT_BPB_NUM_CLUST (BPB) + 2;
      root_dir = -1;
      if (FAT_BPB_IS_FAT32 (BPB))
	{
	  fat_size = 8;
	  root_dir = FAT_BPB_ROOT_DIR_CLUSTER (BPB);
	} 
      else if (num_clust > FAT_MAX_12BIT_CLUST)
	fat_size = 4;
      else
	fat_size = 3;
    }

  return retval;
}


static int
fat_create_blocklist (int first_fat_entry)
{
  BLK_CUR_FILEPOS = 0;
  BLK_CUR_BLKNUM = 0;
  BLK_CUR_BLKLIST = BLK_BLKLIST_START;
  block_file = 1;
  filepos = 0;

  if (first_fat_entry < 0)
    {
      /* root directory */

      BLK_BLKSTART (BLK_BLKLIST_START) = FAT_BPB_ROOT_DIR_START (BPB);
      fsmax = filemax = SECTOR_SIZE * (BLK_BLKLENGTH (BLK_BLKLIST_START)
				       = FAT_BPB_ROOT_DIR_LENGTH (BPB));
      return 1;
    }
  else
    /* any real directory/file */
    {
      int blk_cur_blklist = BLK_BLKLIST_START, blk_cur_blknum;
      int last_fat_entry, new_mapblock;

      fsmax = 0;

      do
	{
	  BLK_BLKSTART (blk_cur_blklist)
	    = (first_fat_entry - 2) * FAT_BPB_SECT_PER_CLUST (BPB) + data_offset;
	  blk_cur_blknum = 0;

	  do
	    {
	      blk_cur_blknum += FAT_BPB_SECT_PER_CLUST (BPB);
	      last_fat_entry = first_fat_entry;

	      /*
	       *  Do FAT table translation here!
	       */

	      new_mapblock = (last_fat_entry * fat_size) >> 1;
	      if (new_mapblock > (mapblock + SECTOR_SIZE * 4 - 3)
		  || new_mapblock < (mapblock + 3))
		{
		  mapblock = ((new_mapblock < 6) ? 0 :
			      ((new_mapblock - 6) & ~0x1FF));
		  if (!devread ((mapblock >> 9) + FAT_BPB_FAT_START (BPB),
				0, SECTOR_SIZE * 4, (char *) FAT_BUF))
		    return 0;
		}

	      first_fat_entry
		= *((unsigned long *) (FAT_BUF + (new_mapblock - mapblock)));

	      if (fat_size == 3)
		{
		  if (last_fat_entry & 1)
		    first_fat_entry >>= 4;

		  first_fat_entry &= 0xFFF;
		}
	      else if (fat_size == 4)
		first_fat_entry &= 0xFFFF;

	      if (first_fat_entry < 2)
		{
		  errnum = ERR_FSYS_CORRUPT;
		  return 0;
		}
	    }
	  while (first_fat_entry == (last_fat_entry + 1)
		 && first_fat_entry < num_clust);

	  BLK_BLKLENGTH (blk_cur_blklist) = blk_cur_blknum;
	  fsmax += blk_cur_blknum * SECTOR_SIZE;
	  blk_cur_blklist += BLK_BLKLIST_INC_VAL;
	}
      while (first_fat_entry < num_clust && blk_cur_blklist < (FAT_BUF - 7));
    }

  return first_fat_entry >= num_clust;
}


/* XX FAT filesystem uses the block-list filesystem read function,
   so none is defined here.  */


int
fat_dir (char *dirname)
{
  char *rest, ch, filename[13], dir_buf[FAT_DIRENTRY_LENGTH];
  int attrib = FAT_ATTRIB_DIR, map = root_dir;

/* main loop to find desired directory entry */
loop:

  if (!fat_create_blocklist (map))
    return 0;

  /* if we have a real file (and we're not just printing possibilities),
     then this is where we want to exit */

  if (!*dirname || isspace (*dirname))
    {
      if (attrib & FAT_ATTRIB_DIR)
	{
	  errnum = ERR_BAD_FILETYPE;
	  return 0;
	}

      return 1;
    }

  /* continue with the file/directory name interpretation */

  while (*dirname == '/')
    dirname++;

  filemax = fsmax;

  if (!filemax || !(attrib & FAT_ATTRIB_DIR))
    {
      errnum = ERR_BAD_FILETYPE;
      return 0;
    }

  for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);

  *rest = 0;

  do
    {
      if (grub_read (dir_buf, FAT_DIRENTRY_LENGTH) != FAT_DIRENTRY_LENGTH)
	{
	  if (!errnum)
	    {
	      if (print_possibilities < 0)
		{
#if 0
		  putchar ('\n');
#endif
		  return 1;
		}

	      errnum = ERR_FILE_NOT_FOUND;
	      *rest = ch;
	    }

	  return 0;
	}

      if (!FAT_DIRENTRY_VALID (dir_buf))
	continue;

      /* XXX convert to 8.3 filename format here */
      {
	int i, j, c;

	for (i = 0; i < 8 && (c = filename[i] = tolower (dir_buf[i]))
	     && !isspace (c); i++);

	filename[i++] = '.';

	for (j = 0; j < 3 && (c = filename[i + j] = tolower (dir_buf[8 + j]))
	     && !isspace (c); j++);

	if (j == 0)
	  i--;

	filename[i + j] = 0;
      }

# ifndef STAGE1_5
      if (print_possibilities && ch != '/'
	  && (!*dirname || substring (dirname, filename) <= 0))
	{
	  if (print_possibilities > 0)
	    print_possibilities = -print_possibilities;
	  print_a_completion (filename);
	}
# endif /* STAGE1_5 */
    }
  while (substring (dirname, filename) != 0 ||
	 (print_possibilities && ch != '/'));

  *(dirname = rest) = ch;

  attrib = FAT_DIRENTRY_ATTRIB (dir_buf);
  filemax = FAT_DIRENTRY_FILELENGTH (dir_buf);
  map = FAT_DIRENTRY_FIRST_CLUSTER (dir_buf);

  /* go back to main loop at top of function */
  goto loop;
}

#endif /* FSYS_FAT */
