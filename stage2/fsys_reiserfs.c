/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000  Free Software Foundation, Inc.
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

#ifdef FSYS_REISERFS
#include "shared.h"
#include "filesys.h"

#undef REISERDEBUG

/* Some parts of this code (mainly the structures and defines) are
 * from the original reiser fs code, as found in the linux kernel.
 */

/* include/asm-i386/types.h */
typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
typedef unsigned long long __u64;

/* linux/posix_type.h */
typedef long linux_off_t;

/* linux/little_endian.h */
#define __cpu_to_le64(x) ((__u64) (x))
#define __le64_to_cpu(x) ((__u64) (x))
#define __cpu_to_le32(x) ((__u32) (x))
#define __le32_to_cpu(x) ((__u32) (x))
#define __cpu_to_le16(x) ((__u16) (x))
#define __le16_to_cpu(x) ((__u16) (x))

/* include/linux/reiser_fs.h */
/* This is the new super block of a journaling reiserfs system */
struct reiserfs_super_block
{
  __u32 s_block_count;			/* blocks count         */
  __u32 s_free_blocks;                  /* free blocks count    */
  __u32 s_root_block;           	/* root block number    */
  __u32 s_journal_block;           	/* journal block number    */
  __u32 s_journal_dev;           	/* journal device number  */
  __u32 s_orig_journal_size; 		/* size of the journal on FS creation.  used to make sure they don't overflow it */
  __u32 s_journal_trans_max ;           /* max number of blocks in a transaction.  */
  __u32 s_journal_block_count ;         /* total size of the journal. can change over time  */
  __u32 s_journal_max_batch ;           /* max number of blocks to batch into a trans */
  __u32 s_journal_max_commit_age ;      /* in seconds, how old can an async commit be */
  __u32 s_journal_max_trans_age ;       /* in seconds, how old can a transaction be */
  __u16 s_blocksize;                   	/* block size           */
  __u16 s_oid_maxsize;			/* max size of object id array, see get_objectid() commentary  */
  __u16 s_oid_cursize;			/* current size of object id array */
  __u16 s_state;                       	/* valid or error       */
  char s_magic[16];                     /* reiserfs magic string indicates that file system is reiserfs */
  __u16 s_tree_height;                  /* height of disk tree */
  __u16 s_bmap_nr;                      /* amount of bitmap blocks needed to address each block of file system */
  __u16 s_version;
  char s_unused[128] ;			/* zero filled by mkreiserfs */
};

#define REISERFS_MAX_SUPPORTED_VERSION 2
#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"

#define MAX_HEIGHT 7

/*
 * directories use this key as well as old files
 */
struct offset_v1
{
  /*
   * for regular files this is the offset to the first byte of the
   * body, contained in the object-item, as measured from the start of
   * the entire body of the object.
   *
   * for directory entries, k_offset consists of hash derived from
   * hashing the name and using few bits (23 or more) of the resulting
   * hash, and generation number that allows distinguishing names with
   * hash collisions. If number of collisions overflows generation
   * number, we return EEXIST.  High order bit is 0 always 
   */
  __u32 k_offset;
  __u32 k_uniqueness;
};

struct offset_v2
{
  /*
   * for regular files this is the offset to the first byte of the
   * body, contained in the object-item, as measured from the start of
   * the entire body of the object.
   *
   * for directory entries, k_offset consists of hash derived from
   * hashing the name and using few bits (23 or more) of the resulting
   * hash, and generation number that allows distinguishing names with
   * hash collisions. If number of collisions overflows generation
   * number, we return EEXIST.  High order bit is 0 always 
   */
  __u64 k_offset:60;
  __u64 k_type: 4;
};


struct key
{
  /* packing locality: by default parent directory object id */
  __u32 k_dir_id;
  /* object identifier */
  __u32 k_objectid;
  /* the offset and node type (old and new form) */
  union
  {
    struct offset_v1 v1;
    struct offset_v2 v2;
  }
  u;
};

#define KEY_SIZE (sizeof (struct key))
#define K_OFFSET(key) (INFO->version < 2 ? \
		       key.u.v1.k_offset : key.u.v2.k_offset)

/* Header of a disk block.  More precisely, header of a formatted leaf
   or internal node, and not the header of an unformatted node. */
struct block_head
{       
  __u16 blk_level;        /* Level of a block in the tree. */
  __u16 blk_nr_item;      /* Number of keys/items in a block. */
  __u16 blk_free_space;   /* Block free space in bytes. */
  struct key  blk_right_delim_key; /* Right delimiting key for this block (supported for leaf level nodes
				      only) */
};
#define BLKH_SIZE (sizeof(struct block_head))
#define DISK_LEAF_NODE_LEVEL  1 /* Leaf node level.                       */

struct item_head
{
  struct key ih_key; 	/* Everything in the tree is found by searching for it based on its key.*/
  
  union
  {
    __u16 ih_free_space; /* The free space in the last unformatted node of an indirect item if this
			    is an indirect item.  This equals 0xFFFF iff this is a direct item or
			    stat data item. Note that the key, not this field, is used to determine
			    the item type, and thus which field this union contains. */
    __u16 ih_entry_count; /* Iff this is a directory item, this field equals the number of directory
			     entries in the directory item. */
  }
  u;
  __u16 ih_item_len;           /* total size of the item body                  */
  __u16 ih_item_location;      /* an offset to the item body within the block  */
  __u16 ih_version;	       /* 0 for all old items, 2 for new
                                  ones. Highest bit is set by fsck
                                  temporary, cleaned after all done */
};
/* size of item header     */
#define IH_SIZE (sizeof (struct item_head))

struct disk_child
{
  unsigned long       dc_block_number;              /* Disk child's block number. */
  unsigned short      dc_size;		            /* Disk child's used space.   */
};

#define DC_SIZE (sizeof (struct disk_child))

/* Stat Data on disk.
 *
 * Note that reiserfs has two different forms of stat data.  Luckily
 * the fields needed by grub are at the same position.
 */
struct stat_data
{
  __u16 sd_mode;	/* file type, permissions */
  __u16 sd_notused1[3]; /* fields not needed by reiserfs */
  __u32 sd_size;	/* file size */
  __u32 sd_size_hi;	/* file size high 32 bits (since version 2) */
};

struct reiserfs_de_head
{
  __u32 deh_offset;  /* third component of the directory entry key */
  __u32 deh_dir_id;  /* objectid of the parent directory of the
			object, that is referenced by directory entry */
  __u32 deh_objectid;/* objectid of the object, that is referenced by
                        directory entry */
  __u16 deh_location;/* offset of name in the whole item */
  __u16 deh_state;   /* whether 1) entry contains stat data (for
			future), and 2) whether entry is hidden
			(unlinked) */
};

#define DEH_SIZE (sizeof (struct reiserfs_de_head))

#define DEH_Statdata (1 << 0)			/* not used now */
#define DEH_Visible  (1 << 2)

#define SD_OFFSET  0
#define SD_UNIQUENESS 0
#define DOT_OFFSET 1
#define DOT_DOT_OFFSET 2
#define DIRENTRY_UNIQUENESS 500

#define V1_TYPE_STAT_DATA 0x0
#define V1_TYPE_DIRECT 0xffffffff
#define V1_TYPE_INDIRECT 0xfffffffe
#define V1_TYPE_DIRECTORY_MAX 0xfffffffd
#define V2_TYPE_STAT_DATA 0
#define V2_TYPE_INDIRECT 1
#define V2_TYPE_DIRECT 2
#define V2_TYPE_DIRENTRY 3 

#define S_ISREG(mode) (((mode) & 0170000) == 0100000)
#define S_ISDIR(mode) (((mode) & 0170000) == 0040000)


#define REISERFS_ROOT_OBJECTID 2
#define REISERFS_ROOT_PARENT_OBJECTID 1
#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)
/* the spot for the super in versions 3.5 - 3.5.11 (inclusive) */
#define REISERFS_OLD_DISK_OFFSET_IN_BYTES (8 * 1024)
#define REISERFS_OLD_BLOCKSIZE 4096


/* The size of the node cache (must be power of two) */
#define FSYSREISER_CACHE_SIZE 24*1024
#define FSYSREISER_MIN_BLOCKSIZE SECTOR_SIZE
#define FSYSREISER_MAX_BLOCKSIZE FSYSREISER_CACHE_SIZE / 3

struct fsys_reiser_fileinfo
{
  __u32 k_dir_id;
  __u32 k_objectid;
};

struct fsys_reiser_info
{
  struct item_head *current_ih;
  char *current_item;
  struct fsys_reiser_fileinfo fileinfo;
  __u16 version;
  __u16 tree_depth;
  __u8  blocksize_shift;
  __u8  fullblocksize_shift;
  __u16 blocksize;
  __u16 cached_slots;
  
  unsigned int blocks[MAX_HEIGHT];
  unsigned int next_key_nr[MAX_HEIGHT];
};

/* The cached s+tree blocks in FSYS_BUF */
#define ROOT     ((char *) ((int) FSYS_BUF))
#define CACHE(i) (ROOT + ((i) << INFO->fullblocksize_shift))
#define LEAF     CACHE (DISK_LEAF_NODE_LEVEL)

#define BLOCKHEAD(cache) ((struct block_head *) cache)
#define ITEMHEAD         ((struct item_head  *) ((int) LEAF + BLKH_SIZE))
#define KEY(cache)       ((struct key        *) ((int) cache + BLKH_SIZE))
#define DC(cache)        ((struct disk_child *) \
			  ((int) cache + BLKH_SIZE + KEY_SIZE * nr_item))
#define INFO \
    ((struct fsys_reiser_info *) ((int) FSYS_BUF + FSYSREISER_CACHE_SIZE))


static __inline__ unsigned long
log2 (unsigned long word)
{
  __asm__ ("bsfl %1,%0"
	   : "=r" (word)
	   : "r" (word));
  return word;
}

/* check filesystem types and read superblock into memory buffer */
int
reiserfs_mount (void)
{
  struct reiserfs_super_block super;
  
  if (! devread (REISERFS_DISK_OFFSET_IN_BYTES >> SECTOR_BITS, 0, 
		 sizeof (struct reiserfs_super_block), (char *) &super)
      || (substring (REISER2FS_SUPER_MAGIC_STRING, super.s_magic) > 0
	  && substring (REISERFS_SUPER_MAGIC_STRING, super.s_magic) > 0)
      || (/* check that this is not a copy inside the journal log */
	  super.s_journal_block * super.s_blocksize
	  <= REISERFS_DISK_OFFSET_IN_BYTES))
    {
      /* Try old super block position */
      if (! devread (REISERFS_OLD_DISK_OFFSET_IN_BYTES >> SECTOR_BITS, 0, 
		     sizeof (struct reiserfs_super_block), (char *) &super))
	return 0;
      
      if (substring (REISER2FS_SUPER_MAGIC_STRING, super.s_magic) > 0
	  && substring (REISERFS_SUPER_MAGIC_STRING, super.s_magic) > 0)
	{
	  /* pre journaling super block ? */
	  if (substring ("ReIsErFs", (char *) ((int) &super + 20)) > 0)
	    return 0;
	  
	  super.s_blocksize = REISERFS_OLD_BLOCKSIZE;
	  super.s_version = 0;
	}
    }
  
  /* check the version number.  */
  if (super.s_version > REISERFS_MAX_SUPPORTED_VERSION)
    return 0;
  
  INFO->version = super.s_version;
  INFO->blocksize = super.s_blocksize;
  INFO->fullblocksize_shift = log2 (super.s_blocksize);
  INFO->blocksize_shift = INFO->fullblocksize_shift - SECTOR_BITS;
  INFO->cached_slots = 
    (FSYSREISER_CACHE_SIZE >> INFO->fullblocksize_shift) - 1;
  
  if (super.s_blocksize < FSYSREISER_MIN_BLOCKSIZE
      || super.s_blocksize > FSYSREISER_MAX_BLOCKSIZE
      || (SECTOR_SIZE << INFO->blocksize_shift) != super.s_blocksize
      || ! devread (super.s_root_block << INFO->blocksize_shift, 
		    0, INFO->blocksize, (char *) ROOT))
    return 0;
  
  INFO->tree_depth = BLOCKHEAD (ROOT)->blk_level;
  
#ifdef REISERDEBUG
  printf ("root read_in: block=%d, depth=%d\n", 
	  super.s_root_block, INFO->tree_depth);
#endif /* REISERDEBUG */
  
  if (INFO->tree_depth >= MAX_HEIGHT)
    return 0;
  if (INFO->tree_depth == DISK_LEAF_NODE_LEVEL)
    {
      /* There is only one node in the whole filesystem, 
       * which is simultanously leaf and root */
      memcpy (LEAF, ROOT, INFO->blocksize);
    }
  return 1;
}

static char *
read_tree_node (int depth)
{
  char* cache = CACHE (depth > INFO->cached_slots ? 1 : depth);
#ifdef REISERDEBUG
  printf ("  next read_in: block=%d (depth=%d)\n",
	  INFO->blocks[depth], depth);
#endif /* REISERDEBUG */
  if (! devread (INFO->blocks[depth] << INFO->blocksize_shift,
		 0, INFO->blocksize, cache))
    return 0;
  
  /* Make sure it has the right node level */
  if (BLOCKHEAD (cache)->blk_level != depth)
    {
      errnum = ERR_FSYS_CORRUPT;
      return 0;
    }
  return cache;
}

/* Get the next key, i.e. the key following the last retrieved key in
 * tree order.  INFO->current_ih and 
 * INFO->current_info are adapted accordingly.  */
static int
next_key (void)
{
  int depth;
  struct item_head *ih = INFO->current_ih + 1;
  char *cache;
  
#ifdef REISERDEBUG
  printf ("next_key:\n  old key %d:%d:%d:%d\n", 
	  INFO->current_ih->ih_key.k_dir_id, 
	  INFO->current_ih->ih_key.k_objectid, 
	  INFO->current_ih->ih_key.u.v1.k_offset,
	  INFO->current_ih->ih_key.u.v1.k_uniqueness);
#endif /* REISERDEBUG */
  
  if (ih == &ITEMHEAD[BLOCKHEAD (LEAF)->blk_nr_item])
    {
      depth = DISK_LEAF_NODE_LEVEL;
      /* The last item, was the last in the leaf node.  
       * Read in the next block */
      do
	{
	  if (depth == INFO->tree_depth)
	    {
	      /* There are no more keys at all.
	       * Return a dummy item with MAX_KEY */
	      ih = (struct item_head *) &BLOCKHEAD (LEAF)->blk_right_delim_key;
	      goto found;
	    }
	  depth++;
#ifdef REISERDEBUG
	  printf ("  depth=%d, i=%d\n", depth, INFO->next_key_nr[depth]);
#endif /* REISERDEBUG */
	}
      while (INFO->next_key_nr[depth] == 0);
      
      if (depth == INFO->tree_depth)
	cache = ROOT;
      else if (depth <= INFO->cached_slots)
	cache = CACHE (depth);
      else
	goto read_at_depth;
      
      do
	{
	  int nr_item = BLOCKHEAD (cache)->blk_nr_item;
	  int key_nr = INFO->next_key_nr[depth]++;
#ifdef REISERDEBUG
	  printf ("  depth=%d, i=%d/%d\n", depth, key_nr, nr_item);
#endif /* REISERDEBUG */
	  if (key_nr == nr_item)
	    /* This is the last item in this block, set the next_key_nr to 0 */
	    INFO->next_key_nr[depth] = 0;
	  INFO->blocks[--depth] = DC (cache)[key_nr].dc_block_number;
	  
	read_at_depth:
	  cache = read_tree_node (depth);
	  if (! cache)
	    return 0;
	}
      while (depth > DISK_LEAF_NODE_LEVEL);
      
      ih = ITEMHEAD;
    }
 found:
  INFO->current_ih   = ih;
  INFO->current_item = &LEAF[ih->ih_item_location];
#ifdef REISERDEBUG
  printf ("  new key %d:%d:%d:%d\n", 
	  INFO->current_ih->ih_key.k_dir_id, 
	  INFO->current_ih->ih_key.k_objectid, 
	  INFO->current_ih->ih_key.u.v1.k_offset,
	  INFO->current_ih->ih_key.u.v1.k_uniqueness);
#endif /* REISERDEBUG */
  return 1;
}

/* preconditions: reiserfs_mount already executed, therefore 
 *   INFO block is valid
 * returns: 0 if error (errnum is set), 
 *   nonzero iff we were able to find the key successfully.
 * postconditions: on a nonzero return, the current_ih and 
 *   current_item fields describe the key that equals the
 *   searched key.  INFO->next_key contains the next key after
 *   the searched key.
 * side effects: messes around with the cache.
 */
static int
search_stat (__u32 dir_id, __u32 objectid) 
{
  char *cache;
  int depth;
  int nr_item;
  int i;
  struct item_head *ih;
#ifdef REISERDEBUG
  printf ("search_stat:\n  key %d:%d:0:0\n", dir_id, objectid);
#endif /* REISERDEBUG */
  
  depth = INFO->tree_depth;
  cache = ROOT;
  
  while (depth > DISK_LEAF_NODE_LEVEL)
    {
      struct key *key;
      nr_item = BLOCKHEAD (cache)->blk_nr_item;
      
      key = KEY (cache);
      
      for (i = 0; i < nr_item; i++) 
	{
	  if (key->k_dir_id > dir_id
	      || (key->k_dir_id == dir_id 
		  && (key->k_objectid > objectid
		      || (key->k_objectid == objectid
			  && (key->u.v1.k_offset
			      | key->u.v1.k_uniqueness) > 0))))
	    break;
	  key++;
	}
      
#ifdef REISERDEBUG
      printf ("  depth=%d, i=%d/%d\n", depth, i, nr_item);
#endif /* REISERDEBUG */
      INFO->next_key_nr[depth] = (i == nr_item) ? 0 : i+1;
      INFO->blocks[--depth] = DC (cache)[i].dc_block_number;
      
      cache = read_tree_node (depth);
      if (! cache)
	return 0;
    }
  
  /* cache == LEAF */
  nr_item = BLOCKHEAD (LEAF)->blk_nr_item;
  ih = ITEMHEAD;
  for (i = 0; i < nr_item; i++) 
    {
      if (ih->ih_key.k_dir_id == dir_id 
	  && ih->ih_key.k_objectid == objectid
	  && ih->ih_key.u.v1.k_offset == 0
	  && ih->ih_key.u.v1.k_uniqueness == 0)
	{
#ifdef REISERDEBUG
	  printf ("  depth=%d, i=%d/%d\n", depth, i, nr_item);
#endif /* REISERDEBUG */
	  INFO->current_ih   = ih;
	  INFO->current_item = &LEAF[ih->ih_item_location];
	  return 1;
	}
      ih++;
    }
  errnum = ERR_FSYS_CORRUPT;
  return 0;
}

int
reiserfs_read (char *buf, int len)
{
  unsigned int blocksize;
  unsigned int offset;
  unsigned int to_read;
  char *prev_buf = buf;
  
#ifdef REISERDEBUG
  printf ("reiserfs_read: filepos=%d len=%d, offset=%d\n",
	  filepos, len, K_OFFSET (INFO->current_ih->ih_key) - 1);
#endif /* REISERDEBUG */
  
  if (INFO->current_ih->ih_key.k_objectid != INFO->fileinfo.k_objectid
      || K_OFFSET (INFO->current_ih->ih_key) > filepos + 1)
    {
      search_stat (INFO->fileinfo.k_dir_id, INFO->fileinfo.k_objectid);
      goto get_next_key;
    }
  
  while (! errnum)
    {
      if (INFO->current_ih->ih_key.k_objectid != INFO->fileinfo.k_objectid)
	break;
      
      offset = filepos - K_OFFSET (INFO->current_ih->ih_key) + 1;
      blocksize = INFO->current_ih->ih_item_len;
      
#ifdef REISERDEBUG
      printf ("  loop: filepos=%d len=%d, offset=%d blocksize=%d\n",
	      filepos, len, offset, blocksize);
#endif /* REISERDEBUG */
      
      if ((INFO->version < 2 
	   ? INFO->current_ih->ih_key.u.v1.k_uniqueness == V1_TYPE_DIRECT
	   : INFO->current_ih->ih_key.u.v2.k_type == V2_TYPE_DIRECT)
	  && offset < blocksize)
	{
	  to_read = blocksize - offset;
	  if (to_read > len)
	    to_read = len;
	  
#ifndef STAGE1_5
	  if (disk_read_hook != NULL)
	    {
	      disk_read_func = disk_read_hook;
	      
	      devread (INFO->blocks[DISK_LEAF_NODE_LEVEL] 
		       << INFO->blocksize_shift,
		       (INFO->current_item - LEAF + offset), to_read, buf);
	      
	      disk_read_func = NULL;
	    }
	  else
#endif /* ! STAGE1_5 */
	    memcpy (buf, INFO->current_item + offset, to_read);
	  goto update_buf_len;
	}
      else if (INFO->version < 2 
	       ? INFO->current_ih->ih_key.u.v1.k_uniqueness == V1_TYPE_INDIRECT
	       : INFO->current_ih->ih_key.u.v2.k_type == V2_TYPE_INDIRECT)
	{
	  blocksize = (blocksize >> 2) << INFO->fullblocksize_shift;
	  
	  while (offset < blocksize)
	    {
	      __u32 blocknr = ((__u32 *) INFO->current_item)
		[offset >> INFO->fullblocksize_shift];
	      int blk_offset = offset & (INFO->blocksize-1);
	      
	      to_read = INFO->blocksize - blk_offset;
	      if (to_read > len)
		to_read = len;
	      
#ifndef STAGE1_5
	      disk_read_func = disk_read_hook;
#endif /* ! STAGE1_5 */
	      
	      devread (blocknr << INFO->blocksize_shift, 
		       blk_offset, to_read, buf);
	      
#ifndef STAGE1_5
	      disk_read_func = NULL;
#endif /* ! STAGE1_5 */
	    update_buf_len:
	      len -= to_read;
	      buf += to_read;
	      offset += to_read;
	      filepos += to_read;
	      if (len == 0)
		goto done;
	    }
	}
    get_next_key:
      next_key ();
    }
 done:
  return errnum ? 0 : buf - prev_buf;
}


/* preconditions: reiserfs_mount already executed, therefore 
 *   INFO block is valid
 * returns: 0 if error, nonzero iff we were able to find the file successfully
 * postconditions: on a nonzero return, INFO->fileinfo contains the info
 *   of the file we were trying to look up, filepos is 0 and filemax is 
 *   the size of the file.
 */
int
reiserfs_dir (char *dirname)
{
  struct reiserfs_de_head *de_head;
  char *rest, ch;
  __u32 dir_id, objectid;
#ifndef STAGE1_5
  int do_possibilities = 0;
#endif /* ! STAGE1_5 */
  dir_id = REISERFS_ROOT_PARENT_OBJECTID;
  objectid = REISERFS_ROOT_OBJECTID;
  
  while (1)
    {
#ifdef REISERDEBUG
      printf ("dirname=%s\n", dirname);
#endif /* REISERDEBUG */
      
      /* Search for the stat info first. */
      if (! search_stat (dir_id, objectid))
	return 0;
      
#ifdef REISERDEBUG
      printf ("sd_mode=%x sd_size=%d\n", 
	      ((struct stat_data *) INFO->current_item)->sd_mode,
	      ((struct stat_data *) INFO->current_item)->sd_size);
#endif /* REISERDEBUG */
      
      /* if we have a real file (and we're not just printing possibilities),
	 then this is where we want to exit */
      
      if (! *dirname || isspace (*dirname))
	{
	  if (! S_ISREG (((struct stat_data *) INFO->current_item)->sd_mode))
	    {
	      errnum = ERR_BAD_FILETYPE;
	      return 0;
	    }
	  
	  filepos = 0;
	  filemax = ((struct stat_data *) INFO->current_item)->sd_size;
	  
	  /* If this is a new stat data and size is > 4GB set filemax to 
	   * maximum
	   */
	  if (INFO->current_ih->ih_version == 2
	      && ((struct stat_data *) INFO->current_item)->sd_size_hi > 0)
	    filemax = 0xffffffff;
	  
	  INFO->fileinfo.k_dir_id = dir_id;
	  INFO->fileinfo.k_objectid = objectid;
	  return next_key ();
	}
      
      /* continue with the file/directory name interpretation */
      while (*dirname == '/')
	dirname++;
      if (! S_ISDIR (((struct stat_data *) INFO->current_item)->sd_mode))
	{
	  errnum = ERR_BAD_FILETYPE;
	  return 0;
	}
      for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);
      *rest = 0;
      
# ifndef STAGE1_5
      if (print_possibilities && ch != '/')
	do_possibilities = 1;
# endif /* ! STAGE1_5 */
      
      while (1)
	{
	  char *name_end;
	  int num_entries;
	  
	  if (! next_key ())
	    return 0;
#ifdef REISERDEBUG
	  printf ("key %d:%d:%d:%d\n", INFO->current_ih->ih_key.k_dir_id, 
		  INFO->current_ih->ih_key.k_objectid, 
		  INFO->current_ih->ih_key.u.v1.k_offset,
		  INFO->current_ih->ih_key.u.v1.k_uniqueness);
#endif /* REISERDEBUG */
	  
	  if (INFO->current_ih->ih_key.k_objectid != objectid)
	    break;
	  
	  name_end = INFO->current_item + INFO->current_ih->ih_item_len;
	  de_head = (struct reiserfs_de_head *) INFO->current_item;
	  num_entries = INFO->current_ih->u.ih_entry_count;
	  while (num_entries > 0)
	    {
	      char *filename = INFO->current_item + de_head->deh_location;
	      char  tmp = *name_end;
	      if ((de_head->deh_state & DEH_Visible))
		{
		  int cmp;
		  /* Note that this may also overwrite the next block in
		   * the cache.  But that doesn't hurt as long as we don't
		   * call next_key ().  */
		  *name_end = 0;
		  cmp = substring (dirname, filename);
		  *name_end = tmp;
# ifndef STAGE1_5
		  if (do_possibilities)
		    {
		      if (cmp <= 0)
			{
			  if (print_possibilities > 0)
			    print_possibilities = -print_possibilities;
			  *name_end = 0;
			  print_a_completion (filename);
			  *name_end = tmp;
			}
		    }
		  else
# endif /* ! STAGE1_5 */
		    if (cmp == 0)
		      goto found;
		}
	      /* The beginning of this name marks the end of the next name.
	       */
	      name_end = filename;
	      de_head++;
	      num_entries--;
	    }
	}
      
# ifndef STAGE1_5
      if (print_possibilities < 0)
	return 1;
# endif /* ! STAGE1_5 */
      
      errnum = ERR_FILE_NOT_FOUND;
      *rest = ch;
      return 0;
      
    found:
      
      *rest = ch;
      dirname = rest;
      
      dir_id = de_head->deh_dir_id;
      objectid = de_head->deh_objectid;
    }
}

int
reiserfs_embed (int *start_sector, int needed_sectors)
{
  struct reiserfs_super_block super;
  int num_sectors;
  
  if (! devread (REISERFS_DISK_OFFSET_IN_BYTES >> SECTOR_BITS, 0, 
		 sizeof (struct reiserfs_super_block), (char *) &super))
    return 0;
  
  *start_sector = 1; /* reserve first sector for stage1 */
  if ((substring (REISERFS_SUPER_MAGIC_STRING, super.s_magic) <= 0
       || substring (REISER2FS_SUPER_MAGIC_STRING, super.s_magic) <= 0)
      && (/* check that this is not a super block copy inside
	   * the journal log */
	  super.s_journal_block * super.s_blocksize 
	  > REISERFS_DISK_OFFSET_IN_BYTES))
    num_sectors = (REISERFS_DISK_OFFSET_IN_BYTES >> SECTOR_BITS) - 1;
  else
    num_sectors = (REISERFS_OLD_DISK_OFFSET_IN_BYTES >> SECTOR_BITS) - 1;
  
  return (needed_sectors <= num_sectors);
}
#endif /* FSYS_REISERFS */
