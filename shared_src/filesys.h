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

#include "pc_slice.h"

#define SECTORS(geom)    ( (geom) & 0xFF )
#define HEADS(geom)      ( ( ( (geom) >> 8 ) & 0xFF ) + 1 )
#define CYLINDERS(geom)  ( ( ( (geom) >> 16 ) & 0x3FF ) + 1 )

/*
 *  Default to all functioning filesystems enabled
 */

#if !( defined(FSYS_FFS) || defined(FSYS_FAT) || defined(FSYS_EXT2FS) )
#define FSYS_FFS
#define FSYS_FAT
#define FSYS_EXT2FS
#endif

#ifdef FSYS_FFS
#define FSYS_FFS_NUM 1
int ffs_mount(void);
int ffs_read(int addr, int len);
int ffs_dir(char *dirname);
#else
#define FSYS_FFS_NUM 0
#endif

#ifdef FSYS_FAT
#define FSYS_FAT_NUM 1
int fat_mount(void);
/* XX FAT filesystem uses block filesystem code for read! */
int fat_dir(char *dirname);
#ifdef NO_BLOCK_FILES
#undef NO_BLOCK_FILES
#endif /* NO_BLOCK_FILES */
#else
#define FSYS_FAT_NUM 0
#endif

#ifdef FSYS_EXT2FS
#define FSYS_EXT2FS_NUM 1
int ext2fs_mount(void);
int ext2fs_read(int addr, int len);
int ext2fs_dir(char *dirname);
#else
#define FSYS_EXT2FS_NUM 0
#endif

#ifndef NUM_FSYS
#define NUM_FSYS  ( FSYS_FFS_NUM + FSYS_FAT_NUM + FSYS_EXT2FS_NUM )
#endif

/* defines for the block filesystem info area */
#ifndef NO_BLOCK_FILES
#define BLK_CUR_FILEPOS      (*((int*)FSYS_BUF))
#define BLK_CUR_BLKLIST      (*((int*)(FSYS_BUF+4)))
#define BLK_CUR_BLKNUM       (*((int*)(FSYS_BUF+8)))
#define BLK_MAX_ADDR         (FSYS_BUF+0x7FF9)
#define BLK_BLKSTART(l)      (*((int*)l))
#define BLK_BLKLENGTH(l)     (*((int*)(l+4)))
#define BLK_BLKLIST_START    (FSYS_BUF+12)
#define BLK_BLKLIST_INC_VAL  8
#endif /* NO_BLOCK_FILES */

/* this next part is pretty ugly, but it keeps it in one place! */

struct fsys_entry
{
  char *name;
  int (*mount_func)(void);
  int (*read_func)(int addr, int len);
  int (*dir_func)(char *dirname);
};

#ifndef _DISK_IO_C

extern int fsmax;
extern int print_possibilities;
extern struct fsys_entry fsys_table[NUM_FSYS+1];

#else

int fsmax;
int print_possibilities;
struct fsys_entry fsys_table[NUM_FSYS+1] =
{
#ifdef FSYS_FAT
  { "fat", fat_mount, 0, fat_dir },
#endif
#ifdef FSYS_EXT2FS
  { "ext2fs", ext2fs_mount, ext2fs_read, ext2fs_dir },
#endif
  /* XX FFS should come last as it's superblock is commonly crossing tracks
     on floppies from track 1 to 2, while others only use 1.  */
#ifdef FSYS_FFS
  { "ffs", ffs_mount, ffs_read, ffs_dir },
#endif
  { 0, 0, 0, 0 }
};

#endif

