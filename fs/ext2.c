/* ext2.c - Second Extended filesystem */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

/* Magic value used to identify an ext2 filesystem.  */
#define	EXT2_MAGIC		0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS		12
/* Maximum lenght of a pathname.  */
#define EXT2_PATH_MAX		4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define	EXT2_MAX_SYMLINKCNT	8

/* Filetype used in directory entry.  */
#define	FILETYPE_DIRECTORY	2
#define	FILETYPE_SYMLINK	7

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data)			\
	(grub_le_to_cpu32 (data->sblock.log2_block_size) + 1)
     
/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)					\
	(grub_le_to_cpu32 (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)		(1 << LOG2_BLOCK_SIZE(data))

/* The ext2 superblock.  */
struct grub_ext_sblock
{
  grub_uint32_t total_inodes;
  grub_uint32_t total_blocks;
  grub_uint32_t reserved_blocks;
  grub_uint32_t free_blocks;
  grub_uint32_t free_inodes;
  grub_uint32_t first_data_block;
  grub_uint32_t log2_block_size;
  grub_uint32_t log2_fragment_size;
  grub_uint32_t blocks_per_group;
  grub_uint32_t fragments_per_group;
  grub_uint32_t inodes_per_group;
  grub_uint32_t mtime;
  grub_uint32_t utime;
  grub_uint16_t mnt_count;
  grub_uint16_t max_mnt_count;
  grub_uint16_t magic;
  grub_uint16_t fs_state;
  grub_uint16_t error_handling;
  grub_uint16_t minor_revision_level;
  grub_uint32_t lastcheck;
  grub_uint32_t checkinterval;
  grub_uint32_t creator_os;
  grub_uint32_t revision_level;
  grub_uint16_t uid_reserved;
  grub_uint16_t gid_reserved;
  grub_uint32_t first_inode;
  grub_uint16_t inode_size;
  grub_uint16_t block_group_number;
  grub_uint32_t feature_compatibility;
  grub_uint32_t feature_incompat;
  grub_uint32_t feature_ro_compat;
  grub_uint32_t unique_id[4];
  char volume_name[16];
  char last_mounted_on[64];
  grub_uint32_t compression_info;
};

/* The ext2 blockgroup.  */
struct ext2_block_group
{
  grub_uint32_t block_id;
  grub_uint32_t inode_id;
  grub_uint32_t inode_table_id;
  grub_uint16_t free_blocks;
  grub_uint16_t free_inodes;
  grub_uint16_t pad;
  grub_uint32_t reserved[3];
};

/* The ext2 inode.  */
struct grub_ext2_inode
{
  grub_uint16_t mode;
  grub_uint16_t uid;
  grub_uint32_t size;
  grub_uint32_t atime;
  grub_uint32_t ctime;
  grub_uint32_t mtime;
  grub_uint32_t dtime;
  grub_uint16_t gid;
  grub_uint16_t nlinks;
  grub_uint32_t blockcnt;  /* Blocks of 512 bytes!! */
  grub_uint32_t flags;
  grub_uint32_t osd1;
  union
  {
    struct datablocks
    {
      grub_uint32_t dir_blocks[INDIRECT_BLOCKS];
      grub_uint32_t indir_block;
      grub_uint32_t double_indir_block;
      grub_uint32_t tripple_indir_block;
    } blocks;
    char symlink[60];
  };
  grub_uint32_t version;
  grub_uint32_t acl;
  grub_uint32_t dir_acl;
  grub_uint32_t fragment_addr;
  grub_uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent
{
  grub_uint32_t inode;
  grub_uint16_t direntlen;
  grub_uint8_t namelen;
  grub_uint8_t filetype;
};

/* Information about a "mounted" ext2 filesystem.  */
struct grub_ext2_data
{
  struct grub_ext_sblock sblock;
  grub_disk_t disk;
  struct grub_ext2_inode inode;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

/* Read into BLKGRP the blockgroup descriptor of blockgroup GROUP of
   the mounted filesystem DATA.  */
inline static grub_err_t
grub_ext2_blockgroup (struct grub_ext2_data *data, int group, 
		      struct ext2_block_group *blkgrp)
{
  return grub_disk_read (data->disk, 
			 ((grub_le_to_cpu32 (data->sblock.first_data_block) + 1)
			  << LOG2_EXT2_BLOCK_SIZE (data)),
			 group * sizeof (struct ext2_block_group), 
			 sizeof (struct ext2_block_group), (char *) blkgrp);
}

/* Return in BLOCK the on disk block number of block FILEBLOCK in the
   opened file descibed by DATA.  If this block is not stored on disk
   in case of a sparse file return 0.  */
static grub_err_t
grub_ext2_get_file_block (struct grub_ext2_data *data,
			  int fileblock, int *block)
{
  int blknr;
  struct grub_ext2_inode *inode = &data->inode;

  /* Direct blocks.  */
  if (fileblock < INDIRECT_BLOCKS)
    blknr = grub_le_to_cpu32 (inode->blocks.dir_blocks[fileblock]);
  /* Indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + EXT2_BLOCK_SIZE (data) / 4)
    {
      grub_uint32_t indir[EXT2_BLOCK_SIZE (data) / 4];

      if (grub_disk_read (data->disk, 
			  grub_le_to_cpu32 (inode->blocks.indir_block)
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return grub_errno;
	  
      blknr = grub_le_to_cpu32 (indir[fileblock - INDIRECT_BLOCKS]);
    }
  /* Double indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + EXT2_BLOCK_SIZE (data) / 4 
	   * (EXT2_BLOCK_SIZE (data)  / 4 + 1))
    {
      unsigned int perblock = EXT2_BLOCK_SIZE (data) / 4;
      unsigned int rblock = fileblock - (INDIRECT_BLOCKS 
					 + EXT2_BLOCK_SIZE (data) / 4);
      grub_uint32_t indir[EXT2_BLOCK_SIZE (data) / 4];

      if (grub_disk_read (data->disk, 
			  grub_le_to_cpu32 (inode->blocks.double_indir_block) 
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return grub_errno;

      if (grub_disk_read (data->disk,
			  grub_le_to_cpu32 (indir[rblock / perblock])
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return grub_errno;

      
      blknr = grub_le_to_cpu32 (indir[rblock % perblock]);
    }
  /* Tripple indirect.  */
  else
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "ext2fs doesn't support tripple indirect blocks");
      return grub_errno;
    }

  *block = blknr;

  return 0;
}

/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_ext2_read_file (struct grub_ext2_data *data,
		     void (*read_hook) (unsigned long sector,
					unsigned offset, unsigned length),
		     int pos, unsigned int len, char *buf)
{
  int i;
  int blockcnt;

  /* Adjust len so it we can't read past the end of the file.  */
  if (len > grub_le_to_cpu32 (data->inode.size))
    len = grub_le_to_cpu32 (data->inode.size);

  blockcnt = ((len + pos) 
	      + EXT2_BLOCK_SIZE (data) - 1) / EXT2_BLOCK_SIZE (data);

  for (i = pos / EXT2_BLOCK_SIZE (data); i < blockcnt; i++)
    {
      int blknr;
      int blockoff = pos % EXT2_BLOCK_SIZE (data);
      int blockend = EXT2_BLOCK_SIZE (data);

      int skipfirst = 0;

      grub_ext2_get_file_block (data, i, &blknr);
      if (grub_errno)
	return -1;
      
      blknr = blknr << LOG2_EXT2_BLOCK_SIZE (data);

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) % EXT2_BLOCK_SIZE (data);
	  
	  /* The last portion is exactly EXT2_BLOCK_SIZE (data).  */
	  if (!blockend)
	    blockend = EXT2_BLOCK_SIZE (data);
	}

      /* First block.  */
      if (i == pos / EXT2_BLOCK_SIZE (data))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  data->disk->read_hook = read_hook;	  
	  grub_disk_read (data->disk, blknr, skipfirst,
			  blockend, buf);
	  data->disk->read_hook = 0;
	  if (grub_errno)
	    return -1;
	}
      else
	grub_memset (buf, EXT2_BLOCK_SIZE (data) - skipfirst, 0);

      buf += EXT2_BLOCK_SIZE (data) - skipfirst;
    }

  return len;
}


/* Read the inode INO for the file described by DATA into INODE.  */
static grub_err_t
grub_ext2_read_inode (struct grub_ext2_data *data,
		      int ino, struct grub_ext2_inode *inode)
{
  struct ext2_block_group blkgrp;
  struct grub_ext_sblock *sblock = &data->sblock;
  int inodes_per_block;
  
  unsigned int blkno;
  unsigned int blkoff;

  /* It is easier to calculate if the first inode is 0.  */
  ino--;
  
  grub_ext2_blockgroup (data, ino / grub_le_to_cpu32 (sblock->inodes_per_group),
			&blkgrp);
  if (grub_errno)
    return grub_errno;

  inodes_per_block = EXT2_BLOCK_SIZE (data) / 128;
  blkno = (ino % grub_le_to_cpu32 (sblock->inodes_per_group))
    / inodes_per_block;
  blkoff = (ino % grub_le_to_cpu32 (sblock->inodes_per_group))
    % inodes_per_block;
  
  /* Read the inode.  */
  if (grub_disk_read (data->disk, 
		      ((grub_le_to_cpu32 (blkgrp.inode_table_id) + blkno)
		       << LOG2_EXT2_BLOCK_SIZE (data)),
		      sizeof (struct grub_ext2_inode) * blkoff,
		      sizeof (struct grub_ext2_inode), (char *) inode))
    return grub_errno;
  
  return 0;
}

static struct grub_ext2_data *
grub_ext2_mount (grub_disk_t disk)
{
  struct grub_ext2_data *data;

  data = grub_malloc (sizeof (struct grub_ext2_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  grub_disk_read (disk, 1 * 2, 0, sizeof (struct grub_ext_sblock),
			(char *) &data->sblock);
  if (grub_errno)
    goto fail;

  /* Make sure this is an ext2 filesystem.  */
  if (grub_le_to_cpu16 (data->sblock.magic) != EXT2_MAGIC)
    goto fail;
  
  data->disk = disk;
  return data;

 fail:
  grub_error (GRUB_ERR_BAD_FS, "not an ext2 filesystem");
  grub_free (data);
  return 0;
}

/* Find the file with the pathname PATH on the filesystem described by
   DATA.  Return its inode number in INO.  */
static grub_err_t
grub_ext2_find_file (struct grub_ext2_data *data, const char *path, int *ino)
{
  int blocksize = EXT2_BLOCK_SIZE (data)
    << grub_le_to_cpu32 (data->sblock.log2_block_size);
  struct grub_ext2_inode *inode = &data->inode;
  int currinode = 2;
  int symlinkcnt = 0;

  char fpath[EXT2_PATH_MAX];
  char *name = fpath;

  grub_strncpy (fpath, path, EXT2_PATH_MAX);

  if (!name || name[0] != '/')
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "bad filename");
      return grub_errno;
    }

  /* Skip the first slash.  */
  name++;
  if (!*name)
    {
      *ino = 2;
      return 0;
    }

  /* Remove trailing "/".  */
  if (name[grub_strlen (name) - 1] =='/')
    name[grub_strlen (name) - 1] = '\0';

  while (*name)
    {
      unsigned int fpos = 0;
      char *next;
      int namesize;

      /* Extract the actual part from the pathname.  */
      next = grub_strchr (name, '/');
      if (next)
	{
	  next[0] = '\0';
	  next++;
	}

      namesize = grub_strlen (name);

      /* Open the file.  */
      grub_ext2_read_inode (data, currinode, inode);
      if (grub_errno)
	goto fail;
      
      /* Search the file.  */
      while (fpos < grub_le_to_cpu32 (inode->size))
	{
	  struct ext2_dirent dirent;

	  /* Read the directory entry.  */
	  grub_ext2_read_file (data, 0, fpos, sizeof (struct ext2_dirent),
			       (char *) &dirent);
	  if (grub_errno)
	    goto fail;

	  if (dirent.namelen != 0)
	    {
	      char filename[dirent.namelen + 1];

	      /* Read the filename part of this directory entry.  */
	      grub_ext2_read_file (data, 0, fpos 
				   + sizeof (struct ext2_dirent),
				   dirent.namelen, filename);
	      if (grub_errno)
		goto fail;
	  
	      filename[dirent.namelen] = '\0';

	      /* Check if the current directory entry described the
		 file we are looking for.  */
	      if (dirent.namelen == namesize 
		  && !grub_strncmp (name, filename, namesize))
		{
		  /* If this is a symlink, follow it.  */
		  if (dirent.filetype == FILETYPE_SYMLINK)
		    {
		      /* XXX: Use malloc instead?  */
		      char symlink[blocksize];

		      if (++symlinkcnt == EXT2_MAX_SYMLINKCNT)
			{
			  grub_error (GRUB_ERR_SYMLINK_LOOP,
				      "too deep nesting of symlinks");
			  goto fail;
			}

		      /* Read the symlink.  */
		      grub_ext2_read_inode (data, 
					    grub_le_to_cpu32 (dirent.inode),
					    inode);

		      /* If the filesize of the symlink is bigger than
			 60 the symlink is stored in a separate block,
			 otherwise it is stored in the inode.  */
		      if (grub_le_to_cpu32 (inode->size) <= 60)
			grub_strncpy (symlink, 
				      inode->symlink,
				      grub_le_to_cpu32 (inode->size));
		      else
			{
			  grub_ext2_read_file (data, 0, 0,
					       grub_le_to_cpu32 (inode->size),
					       symlink);
			  if (grub_errno)
			    goto fail;
			}

		      symlink[grub_le_to_cpu32 (inode->size)] = '\0';
	  
		      /* Check if the symlink is absolute or relative.  */
		      if (symlink[0] == '/')
			{
			  grub_strncpy (fpath, symlink, EXT2_PATH_MAX);
			  name = fpath;
			  currinode = 2;
			}
		      else
			{
			  char *bak = 0;

			  if (next)
			    {
			      bak = grub_strdup (next);
			      if (!bak)
				goto fail;
			    }
		      
			  /* Relative symlink, construct the new path.  */
			  grub_strcpy (fpath, symlink);
			  name = fpath;
		      
			  if (next)
			    {
			      grub_strcat (name, "/");
			      grub_strcat (name, bak);
			      grub_free (bak);
			    }
			}
		  
		      fpos = 0;
		      break;
		    }

		  if (next)
		    {
		      currinode = grub_le_to_cpu32 (dirent.inode);
		      name = next;

		      if (dirent.filetype != FILETYPE_DIRECTORY)
			{
			  grub_error (GRUB_ERR_BAD_FILE_TYPE,
				      "not a directory");
			  goto fail;
			}
		      break;
		    }
		  else /* Found it!  */
		    {
		      *ino = grub_le_to_cpu32 (dirent.inode);
		      return 0;
		    }
		}
	    }

	  /* Move to next directory entry.  */
	  fpos += grub_le_to_cpu16 (dirent.direntlen);
	}

      /* The complete directory was read and no matching file was
	 found.  */
      if (fpos >= grub_le_to_cpu32 (inode->size))
	{
	  grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	  goto fail;
	}
    }

 fail:
  return grub_errno;
}

/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_ext2_open (struct grub_file *file, const char *name)
{
  struct grub_ext2_data *data;
  int ino;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_ext2_mount (file->device->disk);
  if (!data)
    goto fail;
  
  grub_ext2_find_file (data, name, &ino);
  if (grub_errno)
    goto fail;

  grub_ext2_read_inode (data, ino, &data->inode);
  if (grub_errno)
    goto fail;

  if (!(grub_le_to_cpu16 (data->inode.mode) & 0100000))
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a regular file");
      goto fail;
    }
  
  file->size = grub_le_to_cpu32 (data->inode.size);
  file->data = data;
  file->offset = 0;

  return 0;

 fail:

  grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_ext2_close (grub_file_t file)
{
  grub_free (file->data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return GRUB_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static grub_ssize_t
grub_ext2_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_ext2_data *data = 
    (struct grub_ext2_data *) file->data;
  
  return grub_ext2_read_file (data, file->read_hook, file->offset, len, buf);
}


static grub_err_t
grub_ext2_dir (grub_device_t device, const char *path, 
	       int (*hook) (const char *filename, int dir))
{
  struct grub_ext2_data *data = 0;;

  int ino;
  unsigned int fpos = 0;
  
#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif
  
  data = grub_ext2_mount (device->disk);
  if (!data)
    goto fail;

  grub_ext2_find_file (data, (char *) path, &ino);
  if (grub_errno)
    goto fail;

  grub_ext2_read_inode (data, ino, &data->inode);
  if (grub_errno)
    goto fail;

  if (!(grub_le_to_cpu16 (data->inode.mode) & 040000))
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");
      goto fail;
    }

  /* Search the file.  */
  while (fpos < grub_le_to_cpu32 (data->inode.size))
    {
      struct ext2_dirent dirent;
	
      grub_ext2_read_file (data, 0, fpos, sizeof (struct ext2_dirent),
			   (char *) &dirent);
      if (grub_errno)
	goto fail;

      if (dirent.namelen != 0)
	{
	  char filename[dirent.namelen + 1];

	  grub_ext2_read_file (data, 0, fpos + sizeof (struct ext2_dirent),
			       dirent.namelen, filename);
	  if (grub_errno)
	    goto fail;
	  
	  filename[dirent.namelen] = '\0';
	  
	  hook (filename, dirent.filetype == FILETYPE_DIRECTORY);
	}

      fpos += grub_le_to_cpu16 (dirent.direntlen);
    }

 fail:

  grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_ext2_label (grub_device_t device, char **label)
{
  struct grub_ext2_data *data;
  grub_disk_t disk = device->disk;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_ext2_mount (disk);
  if (data)
    *label = grub_strndup (data->sblock.volume_name, 14);
  else
    *label = 0;

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  grub_free (data);

  return grub_errno;
}


static struct grub_fs grub_ext2_fs =
  {
    .name = "ext2",
    .dir = grub_ext2_dir,
    .open = grub_ext2_open,
    .read = grub_ext2_read,
    .close = grub_ext2_close,
    .label = grub_ext2_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_ext2_init (void)
{
  grub_fs_register (&grub_ext2_fs);
}

void
grub_ext2_fini (void)
{
  grub_fs_unregister (&grub_ext2_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_ext2_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_ext2_fs);
}
#endif /* ! GRUB_UTIL */
