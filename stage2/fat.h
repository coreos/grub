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


/*
 *  Defines for the FAT BIOS Parameter Block (embedded in the first block
 *  of the partition.
 */

#define FAT_BPB_SIGNATURE             0x29

/* is checking for this signature thing even valid? */
#define FAT_BPB_CHECK_SIG(bpb) \
  (*((unsigned char *) (((int)bpb) + 38)) == FAT_BPB_SIGNATURE)

#define FAT_BPB_NUM_SECTORS(bpb) \
  ( *((unsigned short *) (((int)bpb) + 19)) ? \
    *((unsigned short *) (((int)bpb) + 19)) : \
    *((unsigned long *) (((int)bpb) + 32)) )

#define FAT_BPB_BYTES_PER_SECTOR(bpb) \
  (*((unsigned short *) (((int)bpb) + 11)))
#define FAT_BPB_SECT_PER_CLUST(bpb) \
  (*((unsigned char *) (((int)bpb) + 13)))
#define FAT_BPB_NUM_FAT(bpb) \
  (*((unsigned char *) (((int)bpb) + 16)))

#define FAT_BPB_RESERVED_SECTORS(bpb) \
  (*((unsigned short *) (((int)bpb) + 14)))
#define FAT_BPB_FAT_SECTORS_16(bpb) \
  (*((unsigned short *) (((int)bpb) + 22)))
#define FAT_BPB_FAT_SECTORS_32(bpb) \
  (*((unsigned long *) (((int)bpb) + 36)))
#define FAT_BPB_IS_FAT32(bpb) \
  (FAT_BPB_FAT_SECTORS_16(bpb) == 0)
#define FAT_BPB_FAT_SECTORS(bpb) \
  (FAT_BPB_FAT_SECTORS_16(bpb) \
   ? FAT_BPB_FAT_SECTORS_16(bpb) : FAT_BPB_FAT_SECTORS_32(bpb))
#define FAT_BPB_FAT_START(bpb) FAT_BPB_RESERVED_SECTORS(bpb)

#define FAT_BPB_ROOT_DIR_CLUSTER(bpb) \
  (*((unsigned long *) (((int)bpb) + 44)))

/*
 *  This appears to be a MAJOR kludge!!  Don't use it if possible...
 */
#define FAT_BPB_HIDDEN_SECTORS(bpb) \
  (*((unsigned long *) (((int)bpb) + 28)))

#define FAT_BPB_ROOT_DIR_START(bpb) \
  ( FAT_BPB_NUM_FAT(bpb) * FAT_BPB_FAT_SECTORS(bpb) \
    + FAT_BPB_FAT_START(bpb) )

#define FAT_BPB_ROOT_DIR_LENGTH(bpb) \
  ( (*((unsigned short *) (((int)bpb) + 17)) + 0xF) >> 4 )

#define FAT_BPB_DATA_OFFSET(bpb) \
  ( FAT_BPB_ROOT_DIR_START(bpb) + FAT_BPB_ROOT_DIR_LENGTH(bpb) )

#define FAT_BPB_NUM_CLUST(bpb) \
  ( ( FAT_BPB_NUM_SECTORS(bpb) - FAT_BPB_DATA_OFFSET(bpb) ) \
    / FAT_BPB_SECT_PER_CLUST(bpb) )

/*
 *  Defines minimum disk size to be considered a FAT partition
 */

#define FAT_MIN_NUM_SECTORS       720	/* 360 K disk */

/*
 *  Defines how to differentiate a 12-bit and 16-bit FAT.
 */

#define FAT_MAX_12BIT_CLUST       4087	/* 4085 + 2 */

#define FAT_BPB_FLOPPY_NUM_SECTORS(bpb) \
  ( *((unsigned short *) (((int)bpb) + 19)) \
    && !*((unsigned long *) (((int)bpb) + 32)) \
    && *((unsigned short *) (((int)bpb) + 19)) >= FAT_MIN_NUM_SECTORS \
    && ((*((unsigned short *) (((int)bpb) + 19)) - FAT_BPB_DATA_OFFSET(bpb)) \
	/ FAT_BPB_SECT_PER_CLUST(bpb)) < (FAT_BPB_FAT_SECTORS(bpb) * 342) )

/*
 *  Defines for the file "attribute" byte
 */

#define FAT_ATTRIB_OK_MASK        0x37
#define FAT_ATTRIB_NOT_OK_MASK    0xC8
#define FAT_ATTRIB_DIR            0x10


/*
 *  Defines for FAT directory entries
 */

#define FAT_DIRENTRY_LENGTH       32

#define FAT_DIRENTRY_ATTRIB(entry) \
  (*((unsigned char *) (entry+11)))
#define FAT_DIRENTRY_VALID(entry) \
  ( ((*((unsigned char *) entry)) != 0) \
    & ((*((unsigned char *) entry)) != 0xE5) \
    & !(FAT_DIRENTRY_ATTRIB(entry) & FAT_ATTRIB_NOT_OK_MASK) )
#define FAT_DIRENTRY_FIRST_CLUSTER(entry) \
  ((*((unsigned short *) (entry+26)))+(*((unsigned short *) (entry+20)) << 16))
#define FAT_DIRENTRY_FILELENGTH(entry) \
  (*((unsigned long *) (entry+28)))
