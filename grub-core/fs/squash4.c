/* squash4.c - SquashFS */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/fshelp.h>
#include <grub/deflate.h>

/*
  object         format      Pointed by
  superblock     RAW         Fixed offset (0)
  data           RAW ?       Fixed offset (60)
  inode table    Chunk       superblock
  dir table      Chunk       superblock
  fragment table Chunk       unk1
  unk1           RAW, Chunk  superblock
  unk2           RAW         superblock
  UID/GID        Chunk       exttblptr
  exttblptr      RAW         superblock

  UID/GID table is the array ot uint32_t
  unk1 contains pointer to unk3 followed by some chunk.
  unk2 containts one uint64_t
*/

struct grub_squash_super
{
  grub_uint32_t magic;
#define SQUASH_MAGIC 0x73717368
  grub_uint32_t dummy1;
  grub_uint32_t creation_time;
  grub_uint32_t dummy2;
  grub_uint32_t dummy3[2];
  grub_uint8_t flags;
#define SQUASH_FLAG_UNCOMPRESSED_INODES 1
#define SQUASH_FLAG_UNCOMPRESSED_DATA 2
#define SQUASH_FLAG_UNCOMPRESSED_FRAGMENTS 8
  grub_uint8_t dummy4[7];
  grub_uint16_t root_ino_offset;
  grub_uint16_t root_ino_chunk;
  grub_uint32_t dummy5;
  grub_uint64_t total_size;
  grub_uint64_t exttbloffset;
  grub_uint32_t dummy6[2];
  grub_uint64_t inodeoffset;
  grub_uint64_t diroffset;
  grub_uint64_t unk1offset;
  grub_uint64_t unk2offset;
};


/* Chunk-based */
struct grub_squash_inode
{
  /* Same values as direlem types. */
  grub_uint16_t type;
  grub_uint16_t dummy[3];
  grub_uint32_t mtime;
  union
  {
    struct {
      grub_uint16_t dummy[2];
      grub_uint32_t chunk;
      grub_uint32_t fragment;
      grub_uint32_t offset;
      grub_uint32_t size;
    } file;
    struct {
      grub_uint16_t dummy1[2];
      grub_uint32_t chunk;
      grub_uint16_t dummy2[2];
      grub_uint16_t size;
      grub_uint16_t offset;
      grub_uint16_t dummy3[2];
    } dir;
  };
} __attribute__ ((packed));

/* Chunk-based.  */
struct grub_squash_dirent_header
{
  /* Actually the value is the number of elements - 1.  */
  grub_uint16_t nelems;
  grub_uint16_t dummy1;
  grub_uint32_t ino_chunk;
  grub_uint16_t dummy2[2];
};

struct grub_squash_dirent
{
  grub_uint16_t ino_offset;
  grub_uint16_t dummy;
  grub_uint16_t type;
#define SQUASH_TYPE_DIR 1
#define SQUASH_TYPE_REGULAR 2
  /* Actually the value is the length of name - 1.  */
  grub_uint16_t namelen;
  char name[0];
};

struct grub_squash_frag_desc
{
  grub_uint64_t offset;
  grub_uint64_t dummy;
};

#define SQUASH_CHUNK_SIZE 0x2000
#define SQUASH_CHUNK_FLAGS 0x8000
#define SQUASH_CHUNK_UNCOMPRESSED 0x8000

struct grub_squash_data
{
  grub_disk_t disk;
  struct grub_squash_super sb;
  struct grub_squash_inode ino;
  grub_uint64_t fragments;
};

struct grub_fshelp_node
{
  struct grub_squash_data *data;
  struct grub_squash_inode ino;
};

static grub_err_t
read_chunk (grub_disk_t disk, void *buf, grub_size_t len,
	    grub_uint64_t chunk, grub_off_t offset)
{
  grub_uint64_t chunk_start;
  chunk_start = grub_le_to_cpu64 (chunk);
  while (len > 0)
    {
      grub_uint64_t csize;
      grub_uint16_t d;
      grub_err_t err;
      while (1)
	{
	  err = grub_disk_read (disk, chunk_start >> GRUB_DISK_SECTOR_BITS,
				chunk_start & (GRUB_DISK_SECTOR_SIZE - 1),
				sizeof (d), &d);
	  if (err)
	    return err;
	  if (offset < SQUASH_CHUNK_SIZE)
	    break;
	  offset -= SQUASH_CHUNK_SIZE;
	  chunk_start += 2 + (grub_le_to_cpu16 (d) & ~SQUASH_CHUNK_FLAGS);
	}

      csize = SQUASH_CHUNK_SIZE - offset;
      if (csize > len)
	csize = len;
  
      if (grub_le_to_cpu16 (d) & SQUASH_CHUNK_UNCOMPRESSED)
	{
	  grub_disk_addr_t a = chunk_start + 2 + offset;
	  err = grub_disk_read (disk, (a >> GRUB_DISK_SECTOR_BITS),
				a & (GRUB_DISK_SECTOR_SIZE - 1),
				csize, buf);
	  if (err)
	    return err;
	}
      else
	{
	  char *tmp;
	  grub_size_t bsize = grub_le_to_cpu16 (d) & ~SQUASH_CHUNK_FLAGS; 
	  grub_disk_addr_t a = chunk_start + 2;
	  tmp = grub_malloc (bsize);
	  if (!tmp)
	    return grub_errno;
	  /* FIXME: buffer uncompressed data.  */
	  err = grub_disk_read (disk, (a >> GRUB_DISK_SECTOR_BITS),
				a & (GRUB_DISK_SECTOR_SIZE - 1),
				bsize, tmp);
	  if (err)
	    {
	      grub_free (tmp);
	      return err;
	    }

	  if (grub_zlib_decompress (tmp, bsize, offset,
				    buf, csize) < 0)
	    {
	      grub_free (tmp);
	      return grub_errno;
	    }
	  grub_free (tmp);
	}
      len -= csize;
      offset += csize;
      buf = (char *) buf + csize;
    }
  return GRUB_ERR_NONE;
}

static struct grub_squash_data *
squash_mount (grub_disk_t disk)
{
  struct grub_squash_super sb;
  grub_err_t err;
  struct grub_squash_data *data;
  grub_uint64_t frag;

  err = grub_disk_read (disk, 0, 0, sizeof (sb), &sb);
  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_error (GRUB_ERR_BAD_FS, "not a squash4");
  if (err)
    return NULL;
  if (grub_le_to_cpu32 (sb.magic) != SQUASH_MAGIC)
    {
      grub_error (GRUB_ERR_BAD_FS, "not squash4");
      return NULL;
    }

  err = grub_disk_read (disk, grub_le_to_cpu32 (sb.unk1offset)
			>> GRUB_DISK_SECTOR_BITS, 
			grub_le_to_cpu32 (sb.unk1offset)
			& (GRUB_DISK_SECTOR_SIZE - 1), sizeof (frag), &frag);
  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_error (GRUB_ERR_BAD_FS, "not a squash4");
  if (err)
    return NULL;

  data = grub_malloc (sizeof (*data));
  if (!data)
    return NULL;
  data->sb = sb;
  data->disk = disk;
  data->fragments = frag;

  return data;
}

static int
grub_squash_iterate_dir (grub_fshelp_node_t dir,
			 int NESTED_FUNC_ATTR
			 (*hook) (const char *filename,
				  enum grub_fshelp_filetype filetype,
				  grub_fshelp_node_t node))
{
  grub_uint32_t off = grub_le_to_cpu16 (dir->ino.dir.offset);
  grub_uint32_t endoff;
  unsigned i;

  /* FIXME: why - 3 ? */
  endoff = grub_le_to_cpu32 (dir->ino.dir.size) + off - 3;

  while (off < endoff)
    {
      struct grub_squash_dirent_header dh;
      grub_err_t err;

      err = read_chunk (dir->data->disk, &dh, sizeof (dh),
			grub_le_to_cpu64 (dir->data->sb.diroffset)
			+ grub_le_to_cpu32 (dir->ino.dir.chunk), off);
      if (err)
	return 0;
      off += sizeof (dh);
      for (i = 0; i < (unsigned) grub_le_to_cpu16 (dh.nelems) + 1; i++)
	{
	  char *buf;
	  int r;
	  struct grub_fshelp_node *node;
	  enum grub_fshelp_filetype filetype = GRUB_FSHELP_REG;
	  struct grub_squash_dirent di;
	  struct grub_squash_inode ino;

	  err = read_chunk (dir->data->disk, &di, sizeof (di),
			    grub_le_to_cpu64 (dir->data->sb.diroffset)
			    + grub_le_to_cpu32 (dir->ino.dir.chunk), off);
	  if (err)
	    return 0;
	  off += sizeof (di);

	  err = read_chunk (dir->data->disk, &ino, sizeof (ino),
			    grub_le_to_cpu64 (dir->data->sb.inodeoffset)
			    + grub_le_to_cpu32 (dh.ino_chunk),
			    grub_cpu_to_le16 (di.ino_offset));
	  if (err)
	    return 0;

	  buf = grub_malloc (grub_le_to_cpu16 (di.namelen) + 2);
	  if (!buf)
	    return 0;
	  err = read_chunk (dir->data->disk, buf,
			    grub_le_to_cpu16 (di.namelen) + 1,
			    grub_le_to_cpu64 (dir->data->sb.diroffset)
			    + grub_le_to_cpu32 (dir->ino.dir.chunk), off);
	  if (err)
	    return 0;

	  off += grub_le_to_cpu16 (di.namelen) + 1;
	  buf[grub_le_to_cpu16 (di.namelen) + 1] = 0;
	  if (grub_le_to_cpu16 (di.type) == SQUASH_TYPE_DIR)
	    filetype = GRUB_FSHELP_DIR;
	  node = grub_malloc (sizeof (*node));
	  if (! node)
	    return 0;
	  *node = *dir;
	  node->ino = ino;
	  r = hook (buf, filetype, node);

	  grub_free (buf);
	  if (r)
	    return r;
	}
    }
  return 0;
}

static grub_err_t
make_root_node (struct grub_squash_data *data, struct grub_fshelp_node *root)
{
  grub_memset (root, 0, sizeof (*root));
  root->data = data;
 
 return read_chunk (data->disk, &root->ino, sizeof (root->ino),
		    grub_le_to_cpu64 (data->sb.inodeoffset) 
		    + grub_le_to_cpu16 (data->sb.root_ino_chunk),
		    grub_cpu_to_le16 (data->sb.root_ino_offset));
}

static grub_err_t
grub_squash_dir (grub_device_t device, const char *path,
	       int (*hook) (const char *filename,
			    const struct grub_dirhook_info *info))
{
  auto int NESTED_FUNC_ATTR iterate (const char *filename,
				     enum grub_fshelp_filetype filetype,
				     grub_fshelp_node_t node);

  int NESTED_FUNC_ATTR iterate (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node __attribute__ ((unused)))
    {
      struct grub_dirhook_info info;
      grub_memset (&info, 0, sizeof (info));
      info.dir = ((filetype & GRUB_FSHELP_TYPE_MASK) == GRUB_FSHELP_DIR);
      return hook (filename, &info);
    }

  struct grub_squash_data *data = 0;
  struct grub_fshelp_node *fdiro = 0;
  struct grub_fshelp_node root;
  grub_err_t err;

  data = squash_mount (device->disk);
  if (! data)
    return grub_errno;

  err = make_root_node (data, &root);
  if (err)
    return err;

  grub_fshelp_find_file (path, &root, &fdiro, grub_squash_iterate_dir,
			 NULL, GRUB_FSHELP_DIR);
  if (!grub_errno)
    grub_squash_iterate_dir (fdiro, iterate);

  grub_free (data);

  return grub_errno;
}

static grub_err_t
grub_squash_open (struct grub_file *file, const char *name)
{
  struct grub_squash_data *data = 0;
  struct grub_fshelp_node *fdiro = 0;
  struct grub_fshelp_node root;
  grub_err_t err;

  data = squash_mount (file->device->disk);
  if (! data)
    return grub_errno;

  err = make_root_node (data, &root);
  if (err)
    return err;

  grub_fshelp_find_file (name, &root, &fdiro, grub_squash_iterate_dir,
			 NULL, GRUB_FSHELP_REG);
  if (grub_errno)
    {
      grub_free (data);
      return grub_errno;
    }

  file->data = data;
  data->ino = fdiro->ino;
  file->size = grub_le_to_cpu32 (fdiro->ino.file.size);

  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_squash_read_data (struct grub_squash_data *data, 
		       grub_disk_t disk, const struct grub_squash_inode *ino,
		       grub_off_t off, char *buf, grub_size_t len)
{
  grub_err_t err;
  grub_uint64_t a, b;
  int compressed = 0;

  if (grub_le_to_cpu16 (ino->file.fragment) == 0xffff)
    {
      if (grub_le_to_cpu32 (ino->file.chunk))
	a = grub_le_to_cpu32 (ino->file.chunk);
      else
	a = sizeof (struct grub_squash_super);
      compressed = !(data->sb.flags & SQUASH_FLAG_UNCOMPRESSED_DATA);
    }
  else
    {
      struct grub_squash_frag_desc frag;
      err = read_chunk (disk, &frag, sizeof (frag),
			data->fragments, sizeof (frag)
			* grub_le_to_cpu16 (ino->file.fragment));
      if (err)
	return -1;
      a = grub_le_to_cpu64 (frag.offset) + grub_le_to_cpu32 (ino->file.chunk);
      compressed = !(data->sb.flags & SQUASH_FLAG_UNCOMPRESSED_FRAGMENTS);
    }

  b = grub_le_to_cpu32 (data->ino.file.offset) + off;

  /* FIXME: cache uncompressed chunks.  */
  if (compressed)
    err = grub_zlib_disk_read (disk, a, b, buf, len);
  else
    err = grub_disk_read (disk, (a + b) >> GRUB_DISK_SECTOR_BITS,
			  (a + b) & (GRUB_DISK_SECTOR_SIZE - 1), len, buf);
  if (err)
    return -1;
  return len;
}

static grub_ssize_t
grub_squash_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_squash_data *data = file->data;

  return grub_squash_read_data (data, file->device->disk, &data->ino,
				file->offset, buf, len);
}

static grub_err_t
grub_squash_close (grub_file_t file)
{
  grub_free (file->data);
  return GRUB_ERR_NONE;
}

static struct grub_fs grub_squash_fs =
  {
    .name = "squash4",
    .dir = grub_squash_dir,
    .open = grub_squash_open,
    .read = grub_squash_read,
    .close = grub_squash_close,
#ifdef GRUB_UTIL
    .reserved_first_sector = 0,
#endif
    .next = 0
  };

GRUB_MOD_INIT(squash4)
{
  grub_fs_register (&grub_squash_fs);
}

GRUB_MOD_FINI(squash4)
{
  grub_fs_unregister (&grub_squash_fs);
}

