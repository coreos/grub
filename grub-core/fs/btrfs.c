/* btrfs.c - B-tree file system.  */
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
#include <grub/lib/crc.h>

#define BTRFS_SIGNATURE "_BHRfS_M"

typedef grub_uint8_t btrfs_checksum_t[0x20];
typedef grub_uint16_t btrfs_uuid_t[8];

struct grub_btrfs_device
{
  grub_uint64_t device_id;
  grub_uint8_t dummy[0x62 - 8];
} __attribute__ ((packed));

struct btrfs_superblock
{ 
  btrfs_checksum_t checksum;
  btrfs_uuid_t uuid;
  grub_uint8_t dummy[0x10];
  grub_uint8_t signature[sizeof (BTRFS_SIGNATURE) - 1];
  grub_uint64_t generation;
  grub_uint64_t root_tree;
  grub_uint64_t chunk_tree;
  grub_uint8_t dummy2[0x20];
  grub_uint64_t root_dir_objectid;
  grub_uint8_t dummy3[0x41];
  struct grub_btrfs_device this_device; 
  char label[0x100];
  grub_uint8_t dummy4[0x100];
  grub_uint8_t bootstrap_mapping[0x800];
} __attribute__ ((packed));

struct btrfs_header
{
  btrfs_checksum_t checksum;
  btrfs_uuid_t uuid;
  grub_uint8_t dummy[0x30];
  grub_uint32_t nitems;
  grub_uint8_t level;
} __attribute__ ((packed));

struct grub_btrfs_data
{
  struct btrfs_superblock sblock;
  unsigned int sblock_number;
  grub_uint64_t tree;
  grub_uint64_t inode;
};

struct grub_btrfs_key
{
  grub_uint64_t object_id;
#define GRUB_BTRFS_ITEM_TYPE_INODE_ITEM 0x01
#define GRUB_BTRFS_ITEM_TYPE_DIR_ITEM 0x54
#define GRUB_BTRFS_ITEM_TYPE_EXTENT_ITEM 0x6c
#define GRUB_BTRFS_ITEM_TYPE_ROOT_ITEM 0x84
#define GRUB_BTRFS_ITEM_TYPE_DEVICE 0xd8
#define GRUB_BTRFS_ITEM_TYPE_CHUNK 0xe4
  grub_uint8_t type;
  grub_uint64_t offset;
} __attribute__ ((packed));

struct grub_btrfs_chunk_item
{
  grub_uint64_t size;
  grub_uint64_t dummy;
  grub_uint64_t stripe_length;
  grub_uint8_t dummy2[0x14];
  grub_uint16_t nstripes;
  grub_uint16_t dummy3;
} __attribute__ ((packed));

struct grub_btrfs_chunk_stripe
{
  grub_uint64_t device_id;
  grub_uint64_t offset;
  btrfs_uuid_t device_uuid;
} __attribute__ ((packed));

struct grub_btrfs_leaf_node
{
  struct grub_btrfs_key key;
  grub_uint32_t offset;
  grub_uint32_t size;
} __attribute__ ((packed));

struct grub_btrfs_internal_node
{
  struct grub_btrfs_key key;
  grub_uint64_t blockn;
  grub_uint64_t dummy;
} __attribute__ ((packed));

struct grub_btrfs_dir_item
{
  struct grub_btrfs_key key;
  grub_uint8_t dummy[8];
  grub_uint16_t m;
  grub_uint16_t n;
#define GRUB_BTRFS_DIR_ITEM_TYPE_REGULAR 1
#define GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY 2
#define GRUB_BTRFS_DIR_ITEM_TYPE_SYMLINK 7
  grub_uint8_t type;
  char name[0];
} __attribute__ ((packed));

struct grub_btrfs_leaf_descriptor
{
  unsigned depth;
  unsigned allocated;
  struct {
    grub_disk_addr_t addr;
    unsigned iter;
    unsigned maxiter;
    int leaf;
  } *data;
};

struct grub_btrfs_root_item
{
  grub_uint8_t dummy[0xb0];
  grub_uint64_t tree;
  grub_uint64_t inode;
};

struct grub_btrfs_inode
{
  grub_uint8_t dummy[0x10];
  grub_uint64_t size;
} __attribute__ ((packed));

struct grub_btrfs_extent_data
{
  grub_uint64_t dummy;
  grub_uint64_t size;
  grub_uint8_t compression;
  grub_uint8_t encryption;
  grub_uint16_t encoding;
  grub_uint8_t type;
  union
  {
    char inl[0];
    grub_uint64_t laddr;
  };
} __attribute__ ((packed));

#define GRUB_BTRFS_EXTENT_INLINE 0
#define GRUB_BTRFS_EXTENT_REGULAR 1


#define GRUB_BTRFS_OBJECT_ID_CHUNK 0x100

static grub_disk_addr_t superblock_sectors[] = { 64 * 2, 64 * 1024 * 2,
						 256 * 1048576 * 2,
						 1048576ULL * 1048576ULL * 2 };

static grub_err_t
grub_btrfs_read_logical (struct grub_btrfs_data *data,
			 grub_disk_t disk, grub_disk_addr_t addr,
			 void *buf, grub_size_t size);

static int
key_cmp (const struct grub_btrfs_key *a, const struct grub_btrfs_key *b)
{
  if (grub_cpu_to_le64 (a->object_id) < grub_cpu_to_le64 (b->object_id))
    return -1;
  if (grub_cpu_to_le64 (a->object_id) > grub_cpu_to_le64 (b->object_id))
    return +1;

  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return +1;

  if (grub_cpu_to_le64 (a->offset) < grub_cpu_to_le64 (b->offset))
    return -1;
  if (grub_cpu_to_le64 (a->offset) > grub_cpu_to_le64 (b->offset))
    return +1;
  return 0;
}

static void
free_iterator (struct grub_btrfs_leaf_descriptor *desc)
{
  grub_free (desc->data);
}

static grub_err_t
save_ref (struct grub_btrfs_leaf_descriptor *desc, 
	  grub_disk_addr_t addr, unsigned i, unsigned m, int l)
{
  desc->depth++;
  if (desc->allocated > desc->depth)
    {
      void *newdata;
      desc->allocated *= 2;
      newdata = grub_realloc (desc->data, sizeof (desc->data[0])
			      * desc->allocated);
      if (!newdata)
	return grub_errno;
      desc->data = newdata;
    }
  desc->data[desc->depth - 1].addr = addr;
  desc->data[desc->depth - 1].iter = i;
  desc->data[desc->depth - 1].maxiter = m;
  desc->data[desc->depth - 1].leaf = l;
  return GRUB_ERR_NONE;
}

static int
next (struct grub_btrfs_data *data, grub_disk_t disk,
      struct grub_btrfs_leaf_descriptor *desc,
      grub_disk_addr_t *outaddr, grub_size_t *outsize,
      struct grub_btrfs_key *key_out)
{
  int i;
  grub_err_t err;
  struct grub_btrfs_leaf_node leaf;

  if (desc->depth == 0)
    return 0;
  for (i = desc->depth - 1; i >= 0; i--)
    {
      desc->data[i].iter++;
      if (desc->data[i].iter
	  < desc->data[desc->depth - 1].maxiter)
	break;
      desc->depth--;
    }
  if (i == -1)
    return 0;
  while (!desc->data[desc->depth - 1].leaf)
    {
      struct grub_btrfs_internal_node node;
      struct btrfs_header head;

      err = grub_btrfs_read_logical (data, disk,
				     desc->data[desc->depth - 1].iter
				     * sizeof (node)
				     + sizeof (struct btrfs_header)
				     + desc->data[desc->depth - 1].addr, &node,
				     sizeof (node));
      if (err)
	return -err;

      err = grub_btrfs_read_logical (data, disk,
				     grub_le_to_cpu64 (node.blockn), &head,
				     sizeof (head));
      if (err)
	return -err;

      save_ref (desc, grub_le_to_cpu64 (node.blockn), 0, 
		grub_le_to_cpu32 (head.nitems), !head.level);
    }
  err = grub_btrfs_read_logical (data, disk,
				 desc->data[desc->depth - 1].iter
				 * sizeof (leaf)
				 + sizeof (struct btrfs_header)
				 + desc->data[desc->depth - 1].addr, &leaf,
				 sizeof (leaf));
  if (err)
    return -err;
  *outsize = grub_le_to_cpu32 (leaf.size);
  *outaddr = desc->data[desc->depth - 1].addr + sizeof (struct btrfs_header)
    + grub_le_to_cpu32 (leaf.offset);
  *key_out = leaf.key;
  return 1;  
}

static grub_err_t
lower_bound (struct grub_btrfs_data *data, grub_disk_t disk,
	     const struct grub_btrfs_key *key_in, 
	     struct grub_btrfs_key *key_out,
	     grub_disk_addr_t root,
	     grub_disk_addr_t *outaddr, grub_size_t *outsize,
	     struct grub_btrfs_leaf_descriptor *desc)
{
  grub_disk_addr_t addr = root;
  int depth = -1;

  if (desc)
    {
      desc->allocated = 16;
      desc->depth = 0;
      desc->data = grub_malloc (sizeof (desc->data[0]) * desc->allocated);
      if (!desc->data)
	return grub_errno;
    }

  grub_dprintf ("btrfs",
		"retrieving %" PRIxGRUB_UINT64_T
		" %x %" PRIxGRUB_UINT64_T "\n",
		key_in->object_id, key_in->type, key_in->offset);

  while (1)
    {
      grub_err_t err;
      struct btrfs_header head;

    reiter:
      depth++;
      /* FIXME: preread few nodes into buffer. */
      err = grub_btrfs_read_logical (data, disk, addr, &head, sizeof (head));
      if (err)
	return err;
      addr += sizeof (head);
      if (head.level)
	{
	  unsigned i;
	  struct grub_btrfs_internal_node node, node_last;
	  int have_last = 0;
	  grub_memset (&node_last, 0, sizeof (node_last));
	  for (i = 0; i < grub_le_to_cpu32 (head.nitems); i++)
	    {
	      err = grub_btrfs_read_logical (data, disk, addr
					     + i * sizeof (node),
					     &node, sizeof (node));
	      if (err)
		return err;

	      grub_dprintf ("btrfs",
			    "internal node (depth %d) %" PRIxGRUB_UINT64_T
			    " %x %" PRIxGRUB_UINT64_T "\n", depth,
			    node.key.object_id, node.key.type, node.key.offset);
	      
	      if (key_cmp (&node.key, key_in) == 0)
		{
		  err = GRUB_ERR_NONE;
		  if (desc)
		    err = save_ref (desc, addr - sizeof (head), i,
				    grub_le_to_cpu32 (head.nitems), 0);
		  if (err)
		    return err;
		  addr = grub_le_to_cpu64 (node.blockn);
		  goto reiter;
		}
	      if (key_cmp (&node.key, key_in) > 0)
		break;
	      node_last = node;
	      have_last = 1;
	    }
	  if (have_last)
	    {
	      addr = grub_le_to_cpu64 (node_last.blockn);
	      err = GRUB_ERR_NONE;
	      if (desc)
		err = save_ref (desc, addr - sizeof (head), i - 1,
				 grub_le_to_cpu32 (head.nitems), 0);
	      if (err)
		return err;
	      goto reiter;
	    }
	  *outsize = 0;
	  *outaddr = 0;
	  grub_memset (key_out, 0, sizeof (*key_out));
	  if (desc)
	    return save_ref (desc, addr - sizeof (head), -1,
			     grub_le_to_cpu32 (head.nitems), 0);
	  return GRUB_ERR_NONE;
	}
      {
	unsigned i;
	struct grub_btrfs_leaf_node leaf, leaf_last;
	int have_last = 0;
	for (i = 0; i < grub_le_to_cpu32 (head.nitems); i++)
	  {
	    err = grub_btrfs_read_logical (data, disk, addr + i * sizeof (leaf),
					   &leaf, sizeof (leaf));
	    if (err)
	      return err;
	    
	    grub_dprintf ("btrfs",
			  "leaf (depth %d) %" PRIxGRUB_UINT64_T
			  " %x %" PRIxGRUB_UINT64_T "\n", depth,
			  leaf.key.object_id, leaf.key.type, leaf.key.offset);

	    if (key_cmp (&leaf.key, key_in) == 0)
	      {
		grub_memcpy (key_out, &leaf.key, sizeof(*key_out));
		*outsize = grub_le_to_cpu32 (leaf.size);
		*outaddr = addr + grub_le_to_cpu32 (leaf.offset);
		if (desc)
		  return save_ref (desc, addr - sizeof (head), i,
				   grub_le_to_cpu32 (head.nitems), 1);
		return GRUB_ERR_NONE;	      
	      }
	    
	    if (key_cmp (&leaf.key, key_in) > 0)
	      break;
	    
	    have_last = 1;
	    leaf_last = leaf;
	  }

	if (have_last)
	  {
	    grub_memcpy (key_out, &leaf_last.key, sizeof(*key_out));
	    *outsize = grub_le_to_cpu32 (leaf_last.size);
	    *outaddr = addr + grub_le_to_cpu32 (leaf_last.offset);
	    if (desc)
	      return save_ref (desc, addr - sizeof (head), i - 1,
			       grub_le_to_cpu32 (head.nitems), 1);
	    return GRUB_ERR_NONE;	      
	  }
	*outsize = 0;
	*outaddr = 0;
	grub_memset (key_out, 0, sizeof (*key_out));
	if (desc)
	  return save_ref (desc, addr - sizeof (head), -1,
			   grub_le_to_cpu32 (head.nitems), 1);
	return GRUB_ERR_NONE;
      }
    }
}

static grub_err_t
grub_btrfs_read_logical (struct grub_btrfs_data *data,
			 grub_disk_t disk, grub_disk_addr_t addr,
			 void *buf, grub_size_t size)
{
  while (size > 0)
    {
      grub_uint8_t *ptr;
      struct grub_btrfs_key *key;
      struct grub_btrfs_chunk_item *chunk;  
      struct grub_btrfs_chunk_stripe *stripe;
      grub_size_t csize;
      grub_err_t err; 
      grub_disk_addr_t paddr;
      grub_uint64_t stripen;
      grub_uint32_t stripe_length;
      grub_uint32_t stripe_offset;
      struct grub_btrfs_key key_out;
      int challoc = 0;
      for (ptr = data->sblock.bootstrap_mapping;
	   ptr < data->sblock.bootstrap_mapping
	     + sizeof (data->sblock.bootstrap_mapping)
	     - sizeof (struct grub_btrfs_key);
	   )
	{
	  key = (struct grub_btrfs_key *) ptr;
	  if (key->type != GRUB_BTRFS_ITEM_TYPE_CHUNK)
	    break;
	  chunk = (struct grub_btrfs_chunk_item *) (key + 1);
	  grub_dprintf ("btrfs", "%" PRIxGRUB_UINT64_T " %" PRIxGRUB_UINT64_T " \n",
			grub_le_to_cpu64 (key->offset),
			grub_le_to_cpu64 (chunk->size));
	  if (grub_le_to_cpu64 (key->offset) <= addr
	      && addr < grub_le_to_cpu64 (key->offset)
	      + grub_le_to_cpu64 (chunk->size))
	    goto chunk_found;
	  ptr += sizeof (*key) + sizeof (*chunk)
	    + sizeof (*stripe) * grub_le_to_cpu16 (chunk->nstripes);
	}
      struct grub_btrfs_key key_in;
      grub_size_t chsize;
      grub_disk_addr_t chaddr;
      key_in.object_id = GRUB_BTRFS_OBJECT_ID_CHUNK;
      key_in.type = GRUB_BTRFS_ITEM_TYPE_CHUNK;
      key_in.offset = addr;
      err = lower_bound (data, disk,
			 &key_in, &key_out,
			 grub_le_to_cpu64 (data->sblock.chunk_tree),
			 &chaddr, &chsize, NULL);
      if (err)
	return err;
      key = &key_out;
      if (key->type != GRUB_BTRFS_ITEM_TYPE_CHUNK
	  || !(grub_le_to_cpu64 (key->offset) <= addr))
	return grub_error (GRUB_ERR_BAD_FS,
			   "couldn't find the chunk descriptor");

      chunk = grub_malloc (chsize);
      if (!chunk)
	return grub_errno;

      challoc = 1;
      err = grub_btrfs_read_logical (data, disk, chaddr,
				     chunk, chsize);
      if (err)
	{
	  grub_free (chunk);
	  return err;
	}
      
      if (!(addr < grub_le_to_cpu64 (key->offset)
	    + grub_le_to_cpu64 (chunk->size)))
	return grub_error (GRUB_ERR_BAD_FS,
			   "couldn't find the chunk descriptor");      

    chunk_found:
      stripe_length = grub_divmod64 (grub_le_to_cpu64 (chunk->size),
				     grub_le_to_cpu16 (chunk->nstripes),
				     NULL);
      stripen = grub_divmod64 (addr - grub_le_to_cpu64 (key->offset),
			       stripe_length, &stripe_offset);
      stripe = (struct grub_btrfs_chunk_stripe *) (chunk + 1);
      stripe += stripen;
      csize = grub_le_to_cpu64 (key->offset) + grub_le_to_cpu64 (chunk->size)
	- addr;
      if (csize > size)
	csize = size;
      if (grub_le_to_cpu64 (stripe->device_id) != grub_le_to_cpu64 (data->sblock.this_device.device_id))
	{
	  if (challoc)
	    grub_free (chunk);
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "multidevice isn't implemented yet");
	}
      grub_dprintf ("btrfs", "chunk 0x%" PRIxGRUB_UINT64_T
		    "+0x%" PRIxGRUB_UINT64_T " (%d stripes of %"
		    PRIxGRUB_UINT64_T ") stripe %" PRIxGRUB_UINT64_T
		    " maps to 0x%" PRIxGRUB_UINT64_T "\n",
		    grub_le_to_cpu64 (key->offset),
		    grub_le_to_cpu64 (chunk->size),
		    grub_le_to_cpu16 (chunk->nstripes),
		    grub_le_to_cpu64 (chunk->stripe_length),
		    stripen,
		    stripe->offset);
      paddr = stripe->offset + stripe_offset;

      grub_dprintf ("btrfs", "reading paddr 0x%" PRIxGRUB_UINT64_T
		    " for laddr 0x%" PRIxGRUB_UINT64_T"\n", paddr,
		    addr);      
      err = grub_disk_read (disk, paddr >> GRUB_DISK_SECTOR_BITS, 
			    paddr & (GRUB_DISK_SECTOR_SIZE - 1), csize, buf);
      size -= csize;
      buf = (grub_uint8_t *) buf + csize;
      addr += csize;
      if (challoc)
	grub_free (chunk);
    }
  return GRUB_ERR_NONE;
}

static struct grub_btrfs_data *
grub_btrfs_mount (grub_disk_t disk)
{
  struct grub_btrfs_data *data = grub_malloc (sizeof (*data));
  unsigned i;
  grub_err_t err = GRUB_ERR_NONE;

  if (! data)
    return NULL;

  for (i = 0; i < ARRAY_SIZE (superblock_sectors); i++)
    {
      struct btrfs_superblock sblock;
      err = grub_disk_read (disk, superblock_sectors[i], 0,
			    sizeof (sblock), &sblock);
      if (err == GRUB_ERR_OUT_OF_RANGE)
	break;

      if (grub_memcmp ((char *) sblock.signature, BTRFS_SIGNATURE,
		       sizeof (BTRFS_SIGNATURE) - 1))
	break;
      if (i == 0 || grub_le_to_cpu64 (sblock.generation)
	  > grub_le_to_cpu64 (data->sblock.generation))
	{
	  grub_memcpy (&data->sblock, &sblock, sizeof (sblock));
	  data->sblock_number = i;
	}
    }

  if ((err == GRUB_ERR_OUT_OF_RANGE || !err) && i == 0)
    {
      grub_error (GRUB_ERR_BAD_FS, "not a Btrfs filesystem");
      goto fail;
    }

  if (err == GRUB_ERR_OUT_OF_RANGE)
    grub_errno = err = GRUB_ERR_NONE;

  grub_dprintf ("btrfs", "using superblock %d\n", data->sblock_number);

  return data;

 fail:
  grub_free (data);
  return NULL;
}

static grub_err_t
find_path (struct grub_btrfs_data *data,
	   grub_disk_t disk,
	   const char *path, struct grub_btrfs_key *key,
	   grub_uint64_t *tree, grub_uint8_t *type)
{
  const char *slash = path;
  grub_err_t err;
  grub_disk_addr_t elemaddr;
  grub_size_t elemsize;
  grub_size_t allocated = 0;
  struct grub_btrfs_dir_item *direl = NULL;
  struct grub_btrfs_key key_out;
  int skip_default = 1;
  const char *ctoken;
  grub_size_t ctokenlen;

  *type = GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY;
  *tree = data->sblock.root_tree;
  key->object_id = data->sblock.root_dir_objectid;
  key->type = GRUB_BTRFS_ITEM_TYPE_DIR_ITEM;
  key->offset = 0;

  while (1)
    {
      if (!skip_default)
	{
	  while (path[0] == '/')
	    path++;
	  if (!path[0])
	    break;
	  slash = grub_strchr (path, '/');
	  if (!slash)
	    slash = path + grub_strlen (path);
	  ctoken = path;
	  ctokenlen = slash - path;
	}
      else
	{
	  ctoken = "default";
	  ctokenlen = sizeof ("default") - 1;
	}

      if (*type != GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
      	return grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");

      key->type = GRUB_BTRFS_ITEM_TYPE_DIR_ITEM;
      key->offset = grub_cpu_to_le64 (~grub_getcrc32c (1, ctoken, ctokenlen));
      
      err = lower_bound (data, disk, key, &key_out, *tree,
			 &elemaddr, &elemsize, NULL);
      if (err)
	{
	  grub_free (direl);
	  return err;
	}
      if (key_cmp (key, &key_out) != 0)
	{
	  grub_free (direl);
	  return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	}

      struct grub_btrfs_dir_item *cdirel;
      if (elemsize > allocated)
	{
	  allocated = 2 * elemsize;
	  grub_free (direl);
	  direl = grub_malloc (allocated + 1);
	  if (!direl)
	    return grub_errno;
	}

      err = grub_btrfs_read_logical (data, disk, elemaddr,
				     direl, elemsize);
      if (err)
	{
	  grub_free (direl);
	  return err;
	}

      for (cdirel = direl;
	   (grub_uint8_t *) cdirel - (grub_uint8_t *) direl
	     < (grub_ssize_t) elemsize;
	   cdirel = (void *) ((grub_uint8_t *) (direl + 1)
			      + grub_le_to_cpu16 (cdirel->n)
			      + grub_le_to_cpu16 (cdirel->m)))
	{
	  char c;
	  c = cdirel->name[grub_le_to_cpu16 (cdirel->n)];
	  cdirel->name[grub_le_to_cpu16 (cdirel->n)] = 0;
	  if (grub_strncmp (cdirel->name, ctoken, ctokenlen) == 0)
	    break;
	  cdirel->name[grub_le_to_cpu16 (cdirel->n)] = c;
	}
      if ((grub_uint8_t *) cdirel - (grub_uint8_t *) direl
	  >= (grub_ssize_t) elemsize)
	{
	  grub_free (direl);
	  return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	}

      if (!skip_default)
	path = slash;
      skip_default = 0;
      *type = cdirel->type;
      if (*type == GRUB_BTRFS_DIR_ITEM_TYPE_SYMLINK)
	{
	  grub_free (direl);
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "symlinks not supported");
	}

      switch (cdirel->key.type)
	{
	case GRUB_BTRFS_ITEM_TYPE_ROOT_ITEM:
	  {
	    struct grub_btrfs_root_item ri;
	    err = lower_bound (data, disk, &cdirel->key, &key_out,
			       data->sblock.root_tree,
			       &elemaddr, &elemsize, NULL);
	    if (err)
	      return err;
	    if (cdirel->key.object_id != key_out.object_id
		|| cdirel->key.type != key_out.type)
	      return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	    err = grub_btrfs_read_logical (data, disk, elemaddr,
					   &ri, sizeof (ri));
	    if (err)
	      return err;
	    key->type = GRUB_BTRFS_ITEM_TYPE_DIR_ITEM;
	    key->offset = 0;
	    key->object_id = GRUB_BTRFS_OBJECT_ID_CHUNK;
	    *tree = grub_le_to_cpu64 (ri.tree);
	    break;
	  }
	case GRUB_BTRFS_ITEM_TYPE_INODE_ITEM:
	  if (*slash && *type == GRUB_BTRFS_DIR_ITEM_TYPE_REGULAR)
	    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	  *key = cdirel->key;
	  if (*type == GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
	    key->type = GRUB_BTRFS_ITEM_TYPE_DIR_ITEM;	    
	  break;
	default:
	  return grub_error (GRUB_ERR_BAD_FS, "unrecognised object type 0x%x", 
			     cdirel->key.type);
	}
    }

  grub_free (direl);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_btrfs_dir (grub_device_t device,
                const char *path __attribute__ ((unused)),
                int (*hook) (const char *filename,
                             const struct grub_dirhook_info *info))
{
  struct grub_btrfs_data *data = grub_btrfs_mount (device->disk);
  struct grub_btrfs_key key_in, key_out;
  grub_err_t err;
  grub_disk_addr_t elemaddr;
  grub_size_t elemsize;
  grub_size_t allocated = 0;
  struct grub_btrfs_dir_item *direl = NULL;
  struct grub_btrfs_leaf_descriptor desc;
  int r = 0;
  grub_uint64_t tree;
  grub_uint8_t type;

  if (!data)
    return grub_errno;

  err = find_path (data, device->disk, path, &key_in, &tree, &type);
  if (err)
    return err;
  if (type != GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
    return grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");  

  err = lower_bound (data, device->disk, &key_in, &key_out, 
		     tree,
		     &elemaddr, &elemsize, &desc);
  if (err)
    return err;
  if (key_out.type != GRUB_BTRFS_ITEM_TYPE_DIR_ITEM
      || key_out.object_id != key_in.object_id)
    {
      r = next (data, device->disk, &desc, &elemaddr, &elemsize, &key_out);
      if (r <= 0)
	{
	  free_iterator (&desc);
	  return -r;
	}
    }
  do
    {
      struct grub_dirhook_info info;
      struct grub_btrfs_dir_item *cdirel;
      if (key_out.type != GRUB_BTRFS_ITEM_TYPE_DIR_ITEM
	  || key_out.object_id != key_in.object_id)
	{
	  r = 0;
	  break;
	}
      if (elemsize > allocated)
	{
	  allocated = 2 * elemsize;
	  grub_free (direl);
	  direl = grub_malloc (allocated + 1);
	  if (!direl)
	    {
	      free_iterator (&desc);
	      return grub_errno;
	    }
	}

      err = grub_btrfs_read_logical (data, device->disk, elemaddr,
				     direl, elemsize);
      if (err)
	return err;

      for (cdirel = direl;
	   (grub_uint8_t *) cdirel - (grub_uint8_t *) direl
	     < (grub_ssize_t) elemsize;
	   cdirel = (void *) ((grub_uint8_t *) (direl + 1)
			      + grub_le_to_cpu16 (cdirel->n)
			      + grub_le_to_cpu16 (cdirel->m)))
	{
	  char c;
	  c = cdirel->name[grub_le_to_cpu16 (cdirel->n)];
	  cdirel->name[grub_le_to_cpu16 (cdirel->n)] = 0;
	  grub_memset (&info, 0, sizeof (info));
	  info.dir = (cdirel->type == GRUB_BTRFS_DIR_ITEM_TYPE_DIRECTORY);
	  if (hook (cdirel->name, &info))
	    goto out;
	  cdirel->name[grub_le_to_cpu16 (cdirel->n)] = c;
	}
      r = next (data, device->disk, &desc, &elemaddr, &elemsize, &key_out);
    }
  while (r > 0);

 out:
  grub_free (direl);

  free_iterator (&desc);
  grub_free (data);

  return -r;
}

static grub_err_t
grub_btrfs_open (struct grub_file *file, const char *name)
{
  struct grub_btrfs_data *data = grub_btrfs_mount (file->device->disk);
  struct grub_btrfs_key key_in, key_out;
  grub_err_t err;
  grub_disk_addr_t elemaddr;
  grub_size_t elemsize;
  struct grub_btrfs_inode inode;
  grub_uint8_t type;

  if (!data)
    return grub_errno;

  err = find_path (data, file->device->disk, name, &key_in, &data->tree, &type);
  if (err)
    {
      grub_free (data);
      return err;
    }
  if (type != GRUB_BTRFS_DIR_ITEM_TYPE_REGULAR)
    return grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a regular file");
  data->inode = key_in.object_id;
  key_in.type = GRUB_BTRFS_ITEM_TYPE_INODE_ITEM;

  err = lower_bound (data, file->device->disk, &key_in, &key_out, 
		     data->tree,
		     &elemaddr, &elemsize, NULL);
  if (err)
    return err;
  if (data->inode != key_out.object_id
      || key_out.type != GRUB_BTRFS_ITEM_TYPE_INODE_ITEM)
    return grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a regular file");

  err = grub_btrfs_read_logical (data, file->device->disk, elemaddr,
				 &inode, sizeof (inode));
  if (err)
    return err;

  file->data = data;
  file->size = grub_le_to_cpu64 (inode.size);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_btrfs_close (grub_file_t file)
{
  grub_free (file->data);

  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_btrfs_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_btrfs_data *data = file->data;
  grub_off_t pos = file->offset;
  grub_disk_addr_t elemaddr;
  grub_size_t elemsize;
  struct grub_btrfs_key key_in, key_out;

  while (len)
    {
      grub_size_t csize;
      struct grub_btrfs_extent_data *extent;
      grub_err_t err;
      key_in.object_id = data->inode;
      key_in.type = GRUB_BTRFS_ITEM_TYPE_EXTENT_ITEM;
      key_in.offset = grub_cpu_to_le64 (pos);
      err = lower_bound (data, file->device->disk, &key_in, &key_out, 
			 data->tree,
			 &elemaddr, &elemsize, NULL);
      if (err)
	return -1;
      if (key_out.object_id != data->inode
	  || key_out.type != GRUB_BTRFS_ITEM_TYPE_EXTENT_ITEM)
	{
	  grub_error (GRUB_ERR_BAD_FS, "extent not found");
	  return -1;
	}
      extent = grub_malloc (elemsize);
      if (!extent)
	return grub_errno;

      err = grub_btrfs_read_logical (data, file->device->disk, elemaddr,
				     extent, elemsize);
      if (err)
	{
	  grub_free (extent);
	  return err;
	}
      if (grub_le_to_cpu64 (extent->size) + grub_le_to_cpu64 (key_out.offset)
	  <= pos)
	{
	  grub_free (extent);
	  return grub_error (GRUB_ERR_BAD_FS, "extent not found");
	}
      grub_dprintf ("btrfs", "extent 0x%" PRIxGRUB_UINT64_T "+0x%"
		    PRIxGRUB_UINT64_T "\n",
		    grub_le_to_cpu64 (key_out.offset),
		    grub_le_to_cpu64 (extent->size));
      csize = grub_le_to_cpu64 (extent->size)
	+ grub_le_to_cpu64 (key_out.offset) - pos;
      if (csize > len)
	csize = len;

      if (extent->encryption)
	{
	  grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		      "encryption not supported");
	  return -1;
	}

      if (extent->compression)
	{
	  grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		      "compression not supported");
	  return -1;
	}


      if (extent->encoding)
	{
	  grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		      "encoding not supported");	
	  return -1;
	}

      switch (extent->type)
	{
	case GRUB_BTRFS_EXTENT_INLINE:
	  grub_memcpy (buf, extent->inl, csize);
	  grub_free (extent);
	  break;
	case GRUB_BTRFS_EXTENT_REGULAR:
	  if (!extent->laddr)
	    {
	      grub_memset (buf, 0, csize);
	      break;
	    }
	  err = grub_btrfs_read_logical (data, file->device->disk,
					 grub_le_to_cpu64 (extent->laddr),
					 buf, csize);
	  grub_free (extent);
	  if (err)
	    return -1;
	  break;
	default:
	  grub_free (extent);
	  grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		      "unsupported extent type 0x%x", extent->type);	
	  return -1;
	}
      buf += csize;
      pos += csize;
      len -= csize;
    }
  return pos - file->offset;
}

static grub_err_t
grub_btrfs_uuid (grub_device_t device, char **uuid)
{
  struct grub_btrfs_data *data;

  *uuid = NULL;

  data = grub_btrfs_mount (device->disk);
  if (! data)
    return grub_errno;

  *uuid = grub_xasprintf ("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
			  grub_be_to_cpu16 (data->sblock.uuid[0]),
			  grub_be_to_cpu16 (data->sblock.uuid[1]),
			  grub_be_to_cpu16 (data->sblock.uuid[2]),
			  grub_be_to_cpu16 (data->sblock.uuid[3]),
			  grub_be_to_cpu16 (data->sblock.uuid[4]),
			  grub_be_to_cpu16 (data->sblock.uuid[5]),
			  grub_be_to_cpu16 (data->sblock.uuid[6]),
			  grub_be_to_cpu16 (data->sblock.uuid[7]));

  grub_free (data);

  return grub_errno;
}

static grub_err_t
grub_btrfs_label (grub_device_t device, char **label)
{
  struct grub_btrfs_data *data;

  *label = NULL;

  data = grub_btrfs_mount (device->disk);
  if (! data)
    return grub_errno;

  *label = grub_strndup (data->sblock.label, sizeof (data->sblock.label));

  grub_free (data);

  return grub_errno;
}

static struct grub_fs grub_btrfs_fs =
  {
    .name = "btrfs",
    .dir = grub_btrfs_dir,
    .open = grub_btrfs_open,
    .read = grub_btrfs_read,
    .close = grub_btrfs_close,
    .uuid = grub_btrfs_uuid,
    .label = grub_btrfs_label,
  };

GRUB_MOD_INIT(btrfs)
{
  grub_fs_register (&grub_btrfs_fs);
}

GRUB_MOD_FINI(btrfs)
{
  grub_fs_unregister (&grub_btrfs_fs);
}
