/* ext2.c - Second Extended filesystem */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Marco Gerards <metgerards@student.han.nl>.
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

#include <pupa/err.h>
#include <pupa/file.h>
#include <pupa/mm.h>
#include <pupa/misc.h>
#include <pupa/disk.h>
#include <pupa/dl.h>
#include <pupa/types.h>

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data)			\
	(pupa_le_to_cpu32 (data->sblock.log2_block_size) + 1)
     
/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)					\
	(pupa_le_to_cpu32 (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)		(1 << LOG2_BLOCK_SIZE(data))

/* The ext2 superblock.  */
struct pupa_ext_sblock
{
  pupa_uint32_t total_inodes;
  pupa_uint32_t total_blocks;
  pupa_uint32_t reserved_blocks;
  pupa_uint32_t free_blocks;
  pupa_uint32_t free_inodes;
  pupa_uint32_t first_data_block;
  pupa_uint32_t log2_block_size;
  pupa_uint32_t log2_fragment_size;
  pupa_uint32_t blocks_per_group;
  pupa_uint32_t fragments_per_group;
  pupa_uint32_t inodes_per_group;
  pupa_uint32_t mtime;
  pupa_uint32_t utime;
  pupa_uint16_t mnt_count;
  pupa_uint16_t max_mnt_count;
  pupa_uint16_t magic;
  pupa_uint16_t fs_state;
  pupa_uint16_t error_handling;
  pupa_uint16_t minor_revision_level;
  pupa_uint32_t lastcheck;
  pupa_uint32_t checkinterval;
  pupa_uint32_t creator_os;
  pupa_uint32_t revision_level;
  pupa_uint16_t uid_reserved;
  pupa_uint16_t gid_reserved;
  pupa_uint32_t first_inode;
  pupa_uint16_t inode_size;
  pupa_uint16_t block_group_number;
  pupa_uint32_t feature_compatibility;
  pupa_uint32_t feature_incompat;
  pupa_uint32_t feature_ro_compat;
  pupa_uint32_t unique_id[4];
  char volume_name[16];
  char last_mounted_on[64];
  pupa_uint32_t compression_info;
};

/* The ext2 blockgroup.  */
struct ext2_block_group
{
  pupa_uint32_t block_id;
  pupa_uint32_t inode_id;
  pupa_uint32_t inode_table_id;
  pupa_uint16_t free_blocks;
  pupa_uint16_t free_inodes;
  pupa_uint16_t pad;
  pupa_uint32_t reserved[3];
};

/* The ext2 inode.  */
struct pupa_ext2_inode
{
  pupa_uint16_t mode;
  pupa_uint16_t uid;
  pupa_uint32_t size;
  pupa_uint32_t atime;
  pupa_uint32_t ctime;
  pupa_uint32_t mtime;
  pupa_uint32_t dtime;
  pupa_uint16_t gid;
  pupa_uint16_t nlinks;
  pupa_uint32_t blockcnt;  /* Blocks of 512 bytes!! */
  pupa_uint32_t flags;
  pupa_uint32_t osd1;
  union
  {
    struct datablocks
    {
      pupa_uint32_t dir_blocks[INDIRECT_BLOCKS];
      pupa_uint32_t indir_block;
      pupa_uint32_t double_indir_block;
      pupa_uint32_t tripple_indir_block;
    };
    char symlink[60];
  };
  pupa_uint32_t version;
  pupa_uint32_t acl;
  pupa_uint32_t dir_acl;
  pupa_uint32_t fragment_addr;
  pupa_uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent
{
  pupa_uint32_t inode;
  pupa_uint16_t direntlen;
  pupa_uint8_t namelen;
  pupa_uint8_t filetype;
};

/* Information about a "mounted" ext2 filesystem.  */
struct pupa_ext2_data
{
  struct pupa_ext_sblock sblock;
  pupa_disk_t disk;
  struct pupa_ext2_inode inode;
};

#ifndef PUPA_UTIL
static pupa_dl_t my_mod;
#endif

/* Read into BLKGRP the blockgroup descriptor of blockgroup GROUP of
   the mounted filesystem DATA.  */
inline static pupa_err_t
pupa_ext2_blockgroup (struct pupa_ext2_data *data, int group, 
		      struct ext2_block_group *blkgrp)
{
  return pupa_disk_read (data->disk, 
			 ((pupa_le_to_cpu32 (data->sblock.first_data_block) + 1)
			  << LOG2_EXT2_BLOCK_SIZE (data)),
			 group * sizeof (struct ext2_block_group), 
			 sizeof (struct ext2_block_group), (char *) blkgrp);
}

/* Return in BLOCK the on disk block number of block FILEBLOCK in the
   opened file descibed by DATA.  If this block is not stored on disk
   in case of a sparse file return 0.  */
static pupa_err_t
pupa_ext2_get_file_block (struct pupa_ext2_data *data,
			  int fileblock, int *block)
{
  int blknr;
  struct pupa_ext2_inode *inode = &data->inode;

  /* Direct blocks.  */
  if (fileblock < INDIRECT_BLOCKS)
    blknr = pupa_le_to_cpu32 (inode->dir_blocks[fileblock]);
  /* Indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + EXT2_BLOCK_SIZE (data) / 4)
    {
      pupa_uint32_t indir[EXT2_BLOCK_SIZE (data) / 4];

      if (pupa_disk_read (data->disk, pupa_le_to_cpu32 (inode->indir_block)
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return pupa_errno;
	  
      blknr = pupa_le_to_cpu32 (indir[fileblock - INDIRECT_BLOCKS]);
    }
  /* Double indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + EXT2_BLOCK_SIZE (data) / 4 
	   * (EXT2_BLOCK_SIZE (data)  / 4 + 1))
    {
      unsigned int perblock = EXT2_BLOCK_SIZE (data) / 4;
      unsigned int rblock = fileblock - (INDIRECT_BLOCKS 
					 + EXT2_BLOCK_SIZE (data) / 4);
      pupa_uint32_t indir[EXT2_BLOCK_SIZE (data) / 4];

      if (pupa_disk_read (data->disk, 
			  pupa_le_to_cpu32 (inode->double_indir_block) 
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return pupa_errno;

      if (pupa_disk_read (data->disk,
			  pupa_le_to_cpu32 (indir[rblock / perblock])
			  << LOG2_EXT2_BLOCK_SIZE (data),
			  0, EXT2_BLOCK_SIZE (data), (char *) indir))
	return pupa_errno;

      
      blknr = pupa_le_to_cpu32 (indir[rblock % perblock]);
    }
  /* Tripple indirect.  */
  else
    {
      pupa_error (PUPA_ERR_NOT_IMPLEMENTED_YET,
		  "ext2fs doesn't support tripple indirect blocks");
      return pupa_errno;
    }

  *block = blknr;

  return 0;
}

/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static pupa_err_t
pupa_ext2_read_file (struct pupa_ext2_data *data, int pos,
		     unsigned int len, char *buf)
{
  int i;
  int blockcnt;

  /* Adjust len so it we can't read past the end of the file.  */
  if (len > pupa_le_to_cpu32 (data->inode.size))
    len = pupa_le_to_cpu32 (data->inode.size);

  blockcnt = ((len + pos) 
	      + EXT2_BLOCK_SIZE (data) - 1) / EXT2_BLOCK_SIZE (data);

  for (i = pos / EXT2_BLOCK_SIZE (data); i < blockcnt; i++)
    {
      int blknr;
      int blockoff = pos % EXT2_BLOCK_SIZE (data);
      int blockend = (len + pos) % EXT2_BLOCK_SIZE (data);

      int skipfirst = 0;

      pupa_ext2_get_file_block (data, i, &blknr);
      if (pupa_errno)
	return pupa_errno;
      
      blknr = blknr << LOG2_EXT2_BLOCK_SIZE (data);

      /* First block.  */
      if (i == pos / EXT2_BLOCK_SIZE (data))
	skipfirst = blockoff;
      
      if (!blockend)
	blockend = EXT2_BLOCK_SIZE (data);
      
      blockend -= skipfirst;

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  pupa_disk_read (data->disk, blknr, skipfirst,
				blockend, buf);
	  if (pupa_errno)
	    return pupa_errno;
	}
      else
	pupa_memset (buf, EXT2_BLOCK_SIZE (data) - skipfirst, 0);

      buf += EXT2_BLOCK_SIZE (data) - skipfirst;
    }

  return len;
}


/* Read the inode INO for the file described by DATA into INODE.  */
static pupa_err_t
pupa_ext2_read_inode (struct pupa_ext2_data *data,
		      int ino, struct pupa_ext2_inode *inode)
{
  struct ext2_block_group blkgrp;
  struct pupa_ext_sblock *sblock = &data->sblock;
  int inodes_per_block;
  
  unsigned int blkno;
  unsigned int blkoff;

  /* It is easier to calculate if the first inode is 0.  */
  ino--;
  
  pupa_ext2_blockgroup (data, ino / pupa_le_to_cpu32 (sblock->inodes_per_group),
			&blkgrp);
  if (pupa_errno)
    return pupa_errno;

  inodes_per_block = EXT2_BLOCK_SIZE (data) / 128;
  blkno = (ino % pupa_le_to_cpu32 (sblock->inodes_per_group))
    / inodes_per_block;
  blkoff = (ino % pupa_le_to_cpu32 (sblock->inodes_per_group))
    % inodes_per_block;
  
  /* Read the inode.  */
  if (pupa_disk_read (data->disk, 
		      ((pupa_le_to_cpu32 (blkgrp.inode_table_id) + blkno)
		       << LOG2_EXT2_BLOCK_SIZE (data)),
		      sizeof (struct pupa_ext2_inode) * blkoff,
		      sizeof (struct pupa_ext2_inode), (char *) inode))
    return pupa_errno;
  
  return 0;
}

static struct pupa_ext2_data *
pupa_ext2_mount (pupa_disk_t disk)
{
  struct pupa_ext2_data *data;

  data = pupa_malloc (sizeof (struct pupa_ext2_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  pupa_disk_read (disk, 1 * 2, 0, sizeof (struct pupa_ext_sblock),
			(char *) &data->sblock);
  if (pupa_errno)
    goto fail;

  /* Make sure this is an ext2 filesystem.  */
  if (pupa_le_to_cpu16 (data->sblock.magic) != EXT2_MAGIC)
    goto fail;
  
  data->disk = disk;
  return data;

 fail:
  pupa_error (PUPA_ERR_BAD_FS, "not an ext2 filesystem");
  pupa_free (data);
  return 0;
}

/* Find the file with the pathname PATH on the filesystem described by
   DATA.  Return its inode number in INO.  */
static pupa_err_t
pupa_ext2_find_file (struct pupa_ext2_data *data, const char *path, int *ino)
{
  int blocksize = EXT2_BLOCK_SIZE (data)
    << pupa_le_to_cpu32 (data->sblock.log2_block_size);
  struct pupa_ext2_inode *inode = &data->inode;
  int currinode = 2;
  int symlinkcnt = 0;

  char fpath[EXT2_PATH_MAX];
  char *name = fpath;

  pupa_strncpy (fpath, path, EXT2_PATH_MAX);

  if (!name || name[0] != '/')
    {
      pupa_error (PUPA_ERR_BAD_FILENAME, "bad filename");
      return pupa_errno;
    }

  /* Skip the first slash.  */
  name++;
  if (!*name)
    {
      *ino = 2;
      return 0;
    }

  /* Remove trailing "/".  */
  if (name[pupa_strlen (name) - 1] =='/')
    name[pupa_strlen (name) - 1] = '\0';

  while (*name)
    {
      unsigned int fpos = 0;
      char *next;
      int namesize;

      /* Extract the actual part from the pathname.  */
      next = pupa_strchr (name, '/');
      if (next)
	{
	  next[0] = '\0';
	  next++;
	}

      namesize = pupa_strlen (name);

      /* Open the file.  */
      pupa_ext2_read_inode (data, currinode, inode);
      if (pupa_errno)
	goto fail;
      
      /* Search the file.  */
      while (fpos < pupa_le_to_cpu32 (inode->size))
	{
	  struct ext2_dirent dirent;

	  /* Read the directory entry.  */
	  pupa_ext2_read_file (data, fpos, sizeof (struct ext2_dirent),
			       (char *) &dirent);
	  if (pupa_errno)
	    goto fail;

	  if (dirent.namelen != 0)
	    {
	      char filename[dirent.namelen + 1];

	      /* Read the filename part of this directory entry.  */
	      pupa_ext2_read_file (data, fpos 
				   + sizeof (struct ext2_dirent),
				   dirent.namelen, filename);
	      if (pupa_errno)
		goto fail;
	  
	      filename[dirent.namelen] = '\0';

	      /* Check if the current directory entry described the
		 file we are looking for.  */
	      if (dirent.namelen == namesize 
		  && !pupa_strncmp (name, filename, namesize))
		{
		  /* If this is a symlink, follow it.  */
		  if (dirent.filetype == FILETYPE_SYMLINK)
		    {
		      /* XXX: Use malloc instead?  */
		      char symlink[blocksize];

		      if (++symlinkcnt == EXT2_MAX_SYMLINKCNT)
			{
			  pupa_error (PUPA_ERR_SYMLINK_LOOP,
				      "too deep nesting of symlinks");
			  goto fail;
			}

		      /* Read the symlink.  */
		      pupa_ext2_read_inode (data, 
					    pupa_le_to_cpu32 (dirent.inode),
					    inode);

		      /* If the filesize of the symlink is bigger than
			 60 the symlink is stored in a separate block,
			 otherwise it is stored in the inode.  */
		      if (pupa_le_to_cpu32 (inode->size) <= 60)
			pupa_strncpy (symlink, 
				      inode->symlink,
				      pupa_le_to_cpu32 (inode->size));
		      else
			{
			  pupa_ext2_read_file (data, 0, 
					       pupa_le_to_cpu32 (inode->size),
					       symlink);
			  if (pupa_errno)
			    goto fail;
			}

		      symlink[pupa_le_to_cpu32 (inode->size)] = '\0';
	  
		      /* Check if the symlink is absolute or relative.  */
		      if (symlink[0] == '/')
			{
			  pupa_strncpy (fpath, symlink, EXT2_PATH_MAX);
			  name = fpath;
			  currinode = 2;
			}
		      else
			{
			  char *bak = 0;

			  if (next)
			    {
			      bak = pupa_strdup (next);
			      if (!bak)
				goto fail;
			    }
		      
			  /* Relative symlink, construct the new path.  */
			  pupa_strcpy (fpath, symlink);
			  name = fpath;
		      
			  if (next)
			    {
			      pupa_strcat (name, "/");
			      pupa_strcat (name, bak);
			      pupa_free (bak);
			    }
			}
		  
		      fpos = 0;
		      break;
		    }

		  if (next)
		    {
		      currinode = pupa_le_to_cpu32 (dirent.inode);
		      name = next;

		      if (dirent.filetype != FILETYPE_DIRECTORY)
			{
			  pupa_error (PUPA_ERR_BAD_FILE_TYPE,
				      "not a directory");
			  goto fail;
			}
		      break;
		    }
		  else /* Found it!  */
		    {
		      *ino = pupa_le_to_cpu32 (dirent.inode);
		      return 0;
		    }
		}
	    }

	  /* Move to next directory entry.  */
	  fpos += pupa_le_to_cpu16 (dirent.direntlen);
	}

      /* The complete directory was read and no matching file was
	 found.  */
      if (fpos >= pupa_le_to_cpu32 (inode->size))
	{
	  pupa_error (PUPA_ERR_FILE_NOT_FOUND, "file not found");
	  goto fail;
	}
    }

 fail:
  return pupa_errno;
}

/* Open a file named NAME and initialize FILE.  */
static pupa_err_t
pupa_ext2_open (struct pupa_file *file, const char *name)
{
  struct pupa_ext2_data *data;
  int ino;

#ifndef PUPA_UTIL
  pupa_dl_ref (my_mod);
#endif

  data = pupa_ext2_mount (file->device->disk);
  if (!data)
    goto fail;
  
  pupa_ext2_find_file (data, name, &ino);
  if (pupa_errno)
    goto fail;

  pupa_ext2_read_inode (data, ino, &data->inode);
  if (pupa_errno)
    goto fail;

  if (!(pupa_le_to_cpu16 (data->inode.mode) & 0100000))
    {
      pupa_error (PUPA_ERR_BAD_FILE_TYPE, "not a regular file");
      goto fail;
    }
  
  file->size = pupa_le_to_cpu32 (data->inode.size);
  file->data = data;
  file->offset = 0;

  return 0;

 fail:

  pupa_free (data);

#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif

  return pupa_errno;
}

static pupa_err_t
pupa_ext2_close (pupa_file_t file)
{
  pupa_free (file->data);

#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif

  return pupa_errno;
}

/* Read LEN bytes data from FILE into BUF.  */
static pupa_ssize_t
pupa_ext2_read (pupa_file_t file, char *buf, pupa_ssize_t len)
{
  struct pupa_ext2_data *data = 
    (struct pupa_ext2_data *) file->data;
  
  return pupa_ext2_read_file (data, file->offset, len, buf);
}


static pupa_err_t
pupa_ext2_dir (pupa_device_t device, const char *path, 
	       int (*hook) (const char *filename, int dir))
{
  struct pupa_ext2_data *data = 0;;

  int ino;
  unsigned int fpos = 0;
  
#ifndef PUPA_UTIL
  pupa_dl_ref (my_mod);
#endif
  
  data = pupa_ext2_mount (device->disk);
  if (!data)
    goto fail;

  pupa_ext2_find_file (data, (char *) path, &ino);
  if (pupa_errno)
    goto fail;

  pupa_ext2_read_inode (data, ino, &data->inode);
  if (pupa_errno)
    goto fail;

  if (!(pupa_le_to_cpu16 (data->inode.mode) & 040000))
    {
      pupa_error (PUPA_ERR_BAD_FILE_TYPE, "not a directory");
      goto fail;
    }

  /* Search the file.  */
  while (fpos < pupa_le_to_cpu32 (data->inode.size))
    {
      struct ext2_dirent dirent;
	
      pupa_ext2_read_file (data, fpos, sizeof (struct ext2_dirent),
			   (char *) &dirent);
      if (pupa_errno)
	goto fail;

      if (dirent.namelen != 0)
	{
	  char filename[dirent.namelen + 1];

	  pupa_ext2_read_file (data, fpos + sizeof (struct ext2_dirent),
			       dirent.namelen, filename);
	  if (pupa_errno)
	    goto fail;
	  
	  filename[dirent.namelen] = '\0';
	  
	  hook (filename, dirent.filetype == FILETYPE_DIRECTORY);
	}

      fpos += pupa_le_to_cpu16 (dirent.direntlen);
    }

 fail:

  pupa_free (data);

#ifndef PUPA_UTIL
  pupa_dl_unref (my_mod);
#endif

  return pupa_errno;
}


static struct pupa_fs pupa_ext2_fs =
  {
    .name = "ext2",
    .dir = pupa_ext2_dir,
    .open = pupa_ext2_open,
    .read = pupa_ext2_read,
    .close = pupa_ext2_close,
    .next = 0
  };

#ifdef PUPA_UTIL
void
pupa_ext2_init (void)
{
  pupa_fs_register (&pupa_ext2_fs);
}

void
pupa_ext2_fini (void)
{
  pupa_fs_unregister (&pupa_ext2_fs);
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  pupa_fs_register (&pupa_ext2_fs);
  my_mod = mod;
}

PUPA_MOD_FINI
{
  pupa_fs_unregister (&pupa_ext2_fs);
}
#endif /* ! PUPA_UTIL */
