/* ntfs.c - NTFS filesystem */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/fshelp.h>

#define FILE_MFT      0
#define FILE_MFTMIRR  1
#define FILE_LOGFILE  2
#define FILE_VOLUME   3
#define FILE_ATTRDEF  4
#define FILE_ROOT     5
#define FILE_BITMAP   6
#define FILE_BOOT     7
#define FILE_BADCLUS  8
#define FILE_QUOTA    9
#define FILE_UPCASE  10

#define AT_STANDARD_INFORMATION	0x10
#define AT_ATTRIBUTE_LIST	0x20
#define AT_FILENAME		0x30
#define AT_OBJECT_ID		0x40
#define AT_SECURITY_DESCRIPTOR	0x50
#define AT_VOLUME_NAME		0x60
#define AT_VOLUME_INFORMATION	0x70
#define AT_DATA			0x80
#define AT_INDEX_ROOT		0x90
#define AT_INDEX_ALLOCATION	0xA0
#define AT_BITMAP		0xB0
#define AT_SYMLINK		0xC0
#define AT_EA_INFORMATION	0xD0
#define AT_EA			0xE0

#define ATTR_READ_ONLY		0x1
#define ATTR_HIDDEN		0x2
#define ATTR_SYSTEM		0x4
#define ATTR_ARCHIVE		0x20
#define ATTR_DEVICE		0x40
#define ATTR_NORMAL		0x80
#define ATTR_TEMPORARY		0x100
#define ATTR_SPARSE		0x200
#define ATTR_REPARSE		0x400
#define ATTR_COMPRESSED		0x800
#define ATTR_OFFLINE		0x1000
#define ATTR_NOT_INDEXED	0x2000
#define ATTR_ENCRYPTED		0x4000
#define ATTR_DIRECTORY		0x10000000
#define ATTR_INDEX_VIEW		0x20000000

#define FLAG_COMPRESSED		1
#define FLAG_ENCRYPTED		0x4000
#define FLAG_SPARSE		0x8000

#define BLK_SHR		GRUB_DISK_SECTOR_BITS

#define MAX_MFT		(1024 >> BLK_SHR)
#define MAX_IDX		(16384 >> BLK_SHR)
#define MAX_SPC		(4096 >> BLK_SHR)

#define COM_LEN		4096
#define COM_SEC		(COM_LEN >> BLK_SHR)

#define BMP_LEN		4096

#define AF_ALST		1
#define AF_MMFT		2
#define AF_GPOS		4

#define RF_COMP		1
#define RF_CBLK		2
#define RF_BLNK		4

#define valueat(buf,ofs,type)	*((type*)(((char*)buf)+ofs))

#define u16at(buf,ofs)	grub_le_to_cpu16(valueat(buf,ofs,grub_uint16_t))
#define u32at(buf,ofs)	grub_le_to_cpu32(valueat(buf,ofs,grub_uint32_t))
#define u64at(buf,ofs)	grub_le_to_cpu64(valueat(buf,ofs,grub_uint64_t))

#define v16at(buf,ofs)	valueat(buf,ofs,grub_uint16_t)
#define v32at(buf,ofs)	valueat(buf,ofs,grub_uint32_t)
#define v64at(buf,ofs)	valueat(buf,ofs,grub_uint64_t)

struct grub_ntfs_bpb
{
  grub_uint8_t jmp_boot[3];
  grub_uint8_t oem_name[8];
  grub_uint16_t bytes_per_sector;
  grub_uint8_t sectors_per_cluster;
  grub_uint8_t reserved_1[7];
  grub_uint8_t media;
  grub_uint16_t reserved_2;
  grub_uint16_t sectors_per_track;
  grub_uint16_t num_heads;
  grub_uint32_t num_hidden_sectors;
  grub_uint32_t reserved_3[2];
  grub_uint64_t num_total_sectors;
  grub_uint64_t mft_lcn;
  grub_uint64_t mft_mirr_lcn;
  grub_int8_t clusters_per_mft;
  grub_int8_t reserved_4[3];
  grub_int8_t clusters_per_index;
  grub_int8_t reserved_5[3];
  grub_uint64_t serial_number;
  grub_uint32_t checksum;
} __attribute__ ((packed));

#define grub_ntfs_file grub_fshelp_node

struct grub_ntfs_attr
{
  int flags;
  char *emft_buf, *edat_buf;
  char *attr_cur, *attr_nxt, *attr_end;
  unsigned long save_pos;
  char *sbuf;
  struct grub_ntfs_file *mft;
};

struct grub_fshelp_node
{
  struct grub_ntfs_data *data;
  char *buf;
  grub_uint32_t size;
  grub_uint32_t ino;
  int inode_read;
  struct grub_ntfs_attr attr;
};

struct grub_ntfs_data
{
  struct grub_ntfs_file cmft;
  struct grub_ntfs_file mmft;
  grub_disk_t disk;
  grub_uint32_t mft_size;
  grub_uint32_t idx_size;
  grub_uint32_t spc;
  grub_uint32_t blocksize;
  grub_uint32_t mft_start;
};

struct grub_ntfs_comp
{
  grub_disk_t disk;
  int comp_head, comp_tail;
  unsigned long comp_table[16][2];
  unsigned long cbuf_ofs, cbuf_vcn, spc;
  char *cbuf;
};

struct grub_ntfs_rlst
{
  int flags;
  unsigned long target_vcn, curr_vcn, next_vcn, curr_lcn;
  char *cur_run;
  struct grub_ntfs_attr *attr;
  struct grub_ntfs_comp comp;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

static grub_err_t
fixup (struct grub_ntfs_data *data, char *buf, int len, char *magic)
{
  int ss;
  char *pu;
  unsigned us;

  if (grub_memcmp (buf, magic, 4))
    return grub_error (GRUB_ERR_BAD_FS, "%s label not found", magic);

  ss = u16at (buf, 6) - 1;
  if (ss * (int) data->blocksize != len * GRUB_DISK_SECTOR_SIZE)
    return grub_error (GRUB_ERR_BAD_FS, "Size not match",
		       ss * (int) data->blocksize,
		       len * GRUB_DISK_SECTOR_SIZE);
  pu = buf + u16at (buf, 4);
  us = u16at (pu, 0);
  buf -= 2;
  while (ss > 0)
    {
      buf += data->blocksize;
      pu += 2;
      if (u16at (buf, 0) != us)
	return grub_error (GRUB_ERR_BAD_FS, "Fixup signature not match");
      v16at (buf, 0) = v16at (pu, 0);
      ss--;
    }

  return 0;
}

static grub_err_t read_mft (struct grub_ntfs_data *data, char *buf,
			    unsigned long mftno);
static grub_err_t read_attr (struct grub_ntfs_attr *at, char *dest,
			     unsigned long ofs, unsigned long len,
			     int cached,
			     void
			     NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t
							    sector,
							    unsigned offset,
							    unsigned length));

static grub_err_t read_data (struct grub_ntfs_attr *at, char *pa, char *dest,
			     unsigned long ofs, unsigned long len,
			     int cached,
			     void
			     NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t
							    sector,
							    unsigned offset,
							    unsigned length));

static void
init_attr (struct grub_ntfs_attr *at, struct grub_ntfs_file *mft)
{
  at->mft = mft;
  at->flags = (mft == &mft->data->mmft) ? AF_MMFT : 0;
  at->attr_nxt = mft->buf + u16at (mft->buf, 0x14);
  at->attr_end = at->emft_buf = at->edat_buf = at->sbuf = NULL;
}

static void
free_attr (struct grub_ntfs_attr *at)
{
  grub_free (at->emft_buf);
  grub_free (at->edat_buf);
  grub_free (at->sbuf);
}

static char *
find_attr (struct grub_ntfs_attr *at, unsigned char attr)
{
  if (at->flags & AF_ALST)
    {
    retry:
      while (at->attr_nxt < at->attr_end)
	{
	  at->attr_cur = at->attr_nxt;
	  at->attr_nxt += u16at (at->attr_cur, 4);
	  if (((unsigned char) *at->attr_cur == attr) || (attr == 0))
	    {
	      char *new_pos;

	      if (at->flags & AF_MMFT)
		{
		  if ((grub_disk_read
		       (at->mft->data->disk, v32at (at->attr_cur, 0x10), 0,
			512, at->emft_buf))
		      ||
		      (grub_disk_read
		       (at->mft->data->disk, v32at (at->attr_cur, 0x14), 0,
			512, at->emft_buf + 512)))
		    return NULL;

		  if (fixup
		      (at->mft->data, at->emft_buf, at->mft->data->mft_size,
		       "FILE"))
		    return NULL;
		}
	      else
		{
		  if (read_mft (at->mft->data, at->emft_buf,
				u32at (at->attr_cur, 0x10)))
		    return NULL;
		}

	      new_pos = &at->emft_buf[u16at (at->emft_buf, 0x14)];
	      while ((unsigned char) *new_pos != 0xFF)
		{
		  if (((unsigned char) *new_pos ==
		       (unsigned char) *at->attr_cur)
		      && (u16at (new_pos, 0xE) == u16at (at->attr_cur, 0x18)))
		    {
		      return new_pos;
		    }
		  new_pos += u16at (new_pos, 4);
		}
	      grub_error (GRUB_ERR_BAD_FS,
			  "Can\'t find 0x%X in attribute list",
			  (unsigned char) *at->attr_cur);
	      return NULL;
	    }
	}
      return NULL;
    }
  at->attr_cur = at->attr_nxt;
  while ((unsigned char) *at->attr_cur != 0xFF)
    {
      at->attr_nxt += u16at (at->attr_cur, 4);
      if ((unsigned char) *at->attr_cur == AT_ATTRIBUTE_LIST)
	at->attr_end = at->attr_cur;
      if (((unsigned char) *at->attr_cur == attr) || (attr == 0))
	return at->attr_cur;
      at->attr_cur = at->attr_nxt;
    }
  if (at->attr_end)
    {
      char *pa;

      at->emft_buf = grub_malloc (at->mft->data->mft_size << BLK_SHR);
      if (at->emft_buf == NULL)
	return NULL;

      pa = at->attr_end;
      if (pa[8])
	{
	  if (u32at (pa, 0x28) > 4096)
	    {
	      grub_error (GRUB_ERR_BAD_FS,
			  "Non-resident attribute list too large");
	      return NULL;
	    }
	  at->attr_cur = at->attr_end;
	  at->edat_buf = grub_malloc (u32at (pa, 0x28));
	  if (!at->edat_buf)
	    return NULL;
	  if (read_data (at, pa, at->edat_buf, 0, u32at (pa, 0x28), 0, 0))
	    {
	      grub_error (GRUB_ERR_BAD_FS,
			  "Fail to read non-resident attribute list");
	      return NULL;
	    }
	  at->attr_nxt = at->edat_buf;
	  at->attr_end = at->edat_buf + u32at (pa, 0x30);
	}
      else
	{
	  at->attr_nxt = at->attr_end + u16at (pa, 0x14);
	  at->attr_end = at->attr_end + u32at (pa, 4);
	}
      at->flags |= AF_ALST;
      while (at->attr_nxt < at->attr_end)
	{
	  if (((unsigned char) *at->attr_nxt == attr) || (attr == 0))
	    break;
	  at->attr_nxt += u16at (at->attr_nxt, 4);
	}
      if (at->attr_nxt >= at->attr_end)
	return NULL;

      if ((at->flags & AF_MMFT) && (attr == AT_DATA))
	{
	  at->flags |= AF_GPOS;
	  at->attr_cur = at->attr_nxt;
	  pa = at->attr_cur;
	  v32at (pa, 0x10) = at->mft->data->mft_start;
	  v32at (pa, 0x14) = at->mft->data->mft_start + 1;
	  pa = at->attr_nxt + u16at (pa, 4);
	  while (pa < at->attr_end)
	    {
	      if ((unsigned char) *pa != attr)
		break;
	      if (read_attr
		  (at, pa + 0x10,
		   u32at (pa, 0x10) * (at->mft->data->mft_size << BLK_SHR),
		   at->mft->data->mft_size << BLK_SHR, 0, 0))
		return NULL;
	      pa += u16at (pa, 4);
	    }
	  at->attr_nxt = at->attr_cur;
	  at->flags &= ~AF_GPOS;
	}
      goto retry;
    }
  return NULL;
}

static char *
locate_attr (struct grub_ntfs_attr *at, struct grub_ntfs_file *mft,
	     unsigned char attr)
{
  char *pa;

  init_attr (at, mft);
  if ((pa = find_attr (at, attr)) == NULL)
    return NULL;
  if ((at->flags & AF_ALST) == 0)
    {
      while (1)
	{
	  if ((pa = find_attr (at, attr)) == NULL)
	    break;
	  if (at->flags & AF_ALST)
	    return pa;
	}
      grub_errno = GRUB_ERR_NONE;
      free_attr (at);
      init_attr (at, mft);
      pa = find_attr (at, attr);
    }
  return pa;
}

static char *
read_run_data (char *run, int nn, unsigned long *val, int sig)
{
  unsigned long r, v;

  r = 0;
  v = 1;

  while (nn--)
    {
      r += v * (*(unsigned char *) (run++));
      v <<= 8;
    }

  if ((sig) && (r & (v >> 1)))
    r -= v;

  *val = r;
  return run;
}

static grub_err_t
read_run_list (struct grub_ntfs_rlst *ctx)
{
  int c1, c2;
  unsigned long val;
  char *run;

  run = ctx->cur_run;
retry:
  c1 = ((unsigned char) (*run) & 0xF);
  c2 = ((unsigned char) (*run) >> 4);
  if (!c1)
    {
      if ((ctx->attr) && (ctx->attr->flags & AF_ALST))
	{
	  void NESTED_FUNC_ATTR (*save_hook) (grub_disk_addr_t sector,
					      unsigned offset,
					      unsigned length);

	  save_hook = ctx->comp.disk->read_hook;
	  ctx->comp.disk->read_hook = 0;
	  run = find_attr (ctx->attr, (unsigned char) *ctx->attr->attr_cur);
	  ctx->comp.disk->read_hook = save_hook;
	  if (run)
	    {
	      if (run[8] == 0)
		return grub_error (GRUB_ERR_BAD_FS,
				   "$DATA should be non-resident");

	      run += u16at (run, 0x20);
	      ctx->curr_lcn = 0;
	      goto retry;
	    }
	}
      return grub_error (GRUB_ERR_BAD_FS, "Run list overflown");
    }
  run = read_run_data (run + 1, c1, &val, 0);	/* length of current VCN */
  ctx->curr_vcn = ctx->next_vcn;
  ctx->next_vcn += val;
  run = read_run_data (run, c2, &val, 1);	/* offset to previous LCN */
  ctx->curr_lcn += val;
  if (val == 0)
    ctx->flags |= RF_BLNK;
  else
    ctx->flags &= ~RF_BLNK;
  ctx->cur_run = run;
  return 0;
}

static grub_err_t
decomp_nextvcn (struct grub_ntfs_comp *cc)
{
  if (cc->comp_head >= cc->comp_tail)
    return grub_error (GRUB_ERR_BAD_FS, "Compression block overflown");
  if (grub_disk_read
      (cc->disk,
       (cc->comp_table[cc->comp_head][1] -
	(cc->comp_table[cc->comp_head][0] - cc->cbuf_vcn)) * cc->spc, 0,
       cc->spc << BLK_SHR, cc->cbuf))
    return grub_errno;
  cc->cbuf_vcn++;
  if ((cc->cbuf_vcn >= cc->comp_table[cc->comp_head][0]))
    cc->comp_head++;
  cc->cbuf_ofs = 0;
  return 0;
}

static grub_err_t
decomp_getch (struct grub_ntfs_comp *cc, unsigned char *res)
{
  if (cc->cbuf_ofs >= (cc->spc << BLK_SHR))
    {
      if (decomp_nextvcn (cc))
	return grub_errno;
    }
  *res = (unsigned char) cc->cbuf[cc->cbuf_ofs++];
  return 0;
}

static grub_err_t
decomp_get16 (struct grub_ntfs_comp *cc, grub_uint16_t * res)
{
  unsigned char c1, c2;

  if ((decomp_getch (cc, &c1)) || (decomp_getch (cc, &c2)))
    return grub_errno;
  *res = ((grub_uint16_t) c2) * 256 + ((grub_uint16_t) c1);
  return 0;
}

/* Decompress a block (4096 bytes) */
static grub_err_t
decomp_block (struct grub_ntfs_comp *cc, char *dest)
{
  grub_uint16_t flg, cnt;

  if (decomp_get16 (cc, &flg))
    return grub_errno;
  cnt = (flg & 0xFFF) + 1;

  if (dest)
    {
      if (flg & 0x8000)
	{
	  unsigned char tag;
	  unsigned long bits, copied;

	  bits = copied = tag = 0;
	  while (cnt > 0)
	    {
	      if (copied > COM_LEN)
		return grub_error (GRUB_ERR_BAD_FS,
				   "Compression block too large");

	      if (!bits)
		{
		  if (decomp_getch (cc, &tag))
		    return grub_errno;

		  bits = 8;
		  cnt--;
		  if (cnt <= 0)
		    break;
		}
	      if (tag & 1)
		{
		  unsigned long i, len, delta, code, lmask, dshift;
		  grub_uint16_t word;

		  if (decomp_get16 (cc, &word))
		    return grub_errno;

		  code = word;
		  cnt -= 2;

		  if (!copied)
		    {
		      grub_error (GRUB_ERR_BAD_FS, "Context window empty");
		      return 0;
		    }

		  for (i = copied - 1, lmask = 0xFFF, dshift = 12; i >= 0x10;
		       i >>= 1)
		    {
		      lmask >>= 1;
		      dshift--;
		    }

		  delta = code >> dshift;
		  len = (code & lmask) + 3;

		  for (i = 0; i < len; i++)
		    {
		      dest[copied] = dest[copied - delta - 1];
		      copied++;
		    }
		}
	      else
		{
		  unsigned char ch;

		  if (decomp_getch (cc, &ch))
		    return grub_errno;
		  dest[copied++] = ch;
		  cnt--;
		}
	      tag >>= 1;
	      bits--;
	    }
	  return 0;
	}
      else
	{
	  if (cnt != COM_LEN)
	    return grub_error (GRUB_ERR_BAD_FS,
			       "Invalid compression block size");
	}
    }

  while (cnt > 0)
    {
      int n;

      n = (cc->spc << BLK_SHR) - cc->cbuf_ofs;
      if (n > cnt)
	n = cnt;
      if ((dest) && (n))
	{
	  memcpy (dest, &cc->cbuf[cc->cbuf_ofs], n);
	  dest += n;
	}
      cnt -= n;
      cc->cbuf_ofs += n;
      if ((cnt) && (decomp_nextvcn (cc)))
	return grub_errno;
    }
  return 0;
}

static grub_err_t
read_block (struct grub_ntfs_rlst *ctx, char *buf, int num)
{
  int cpb = COM_SEC / ctx->comp.spc;

  while (num)
    {
      int nn;

      if ((ctx->target_vcn & 0xF) == 0)
	{

	  if (ctx->comp.comp_head != ctx->comp.comp_tail)
	    return grub_error (GRUB_ERR_BAD_FS, "Invalid compression block");
	  ctx->comp.comp_head = ctx->comp.comp_tail = 0;
	  ctx->comp.cbuf_vcn = ctx->target_vcn;
	  ctx->comp.cbuf_ofs = (ctx->comp.spc << BLK_SHR);
	  if (ctx->target_vcn >= ctx->next_vcn)
	    {
	      if (read_run_list (ctx))
		return grub_errno;
	    }
	  while (ctx->target_vcn + 16 > ctx->next_vcn)
	    {
	      if (ctx->flags & RF_BLNK)
		break;
	      ctx->comp.comp_table[ctx->comp.comp_tail][0] = ctx->next_vcn;
	      ctx->comp.comp_table[ctx->comp.comp_tail][1] =
		ctx->curr_lcn + ctx->next_vcn - ctx->curr_vcn;
	      ctx->comp.comp_tail++;
	      if (read_run_list (ctx))
		return grub_errno;
	    }
	  if (ctx->target_vcn + 16 < ctx->next_vcn)
	    return grub_error (GRUB_ERR_BAD_FS,
			       "Compression block should be 16 sector long");
	}

      nn = (16 - (ctx->target_vcn & 0xF)) / cpb;
      if (nn > num)
	nn = num;
      num -= nn;

      if (ctx->flags & RF_BLNK)
	{
	  ctx->target_vcn += nn * cpb;
	  if (ctx->comp.comp_tail == 0)
	    {
	      if (buf)
		{
		  grub_memset (buf, 0, nn * COM_LEN);
		  buf += nn * COM_LEN;
		}
	    }
	  else
	    {
	      while (nn)
		{
		  if (decomp_block (&ctx->comp, buf))
		    return grub_errno;
		  if (buf)
		    buf += COM_LEN;
		  nn--;
		}
	    }
	}
      else
	{
	  nn *= cpb;
	  while ((ctx->comp.comp_head < ctx->comp.comp_tail) && (nn))
	    {
	      int tt;

	      tt =
		ctx->comp.comp_table[ctx->comp.comp_head][0] -
		ctx->target_vcn;
	      if (tt > nn)
		tt = nn;
	      ctx->target_vcn += tt;
	      if (buf)
		{
		  if (grub_disk_read
		      (ctx->comp.disk,
		       (ctx->comp.comp_table[ctx->comp.comp_head][1] -
			(ctx->comp.comp_table[ctx->comp.comp_head][0] -
			 ctx->target_vcn)) * ctx->comp.spc, 0,
		       tt * (ctx->comp.spc << BLK_SHR), buf))
		    return grub_errno;
		  buf += tt * (ctx->comp.spc << BLK_SHR);
		}
	      nn -= tt;
	      if (ctx->target_vcn >=
		  ctx->comp.comp_table[ctx->comp.comp_head][0])
		ctx->comp.comp_head++;
	    }
	  if (nn)
	    {
	      if (buf)
		{
		  if (grub_disk_read
		      (ctx->comp.disk, ctx->curr_lcn * ctx->comp.spc, 0,
		       nn * (ctx->comp.spc << BLK_SHR), buf))
		    return grub_errno;
		  buf += nn * (ctx->comp.spc << BLK_SHR);
		}
	      ctx->target_vcn += nn;
	    }
	}
    }
  return 0;
}

static int
grub_ntfs_read_block (grub_fshelp_node_t node, int block)
{
  struct grub_ntfs_rlst *ctx;

  ctx = (struct grub_ntfs_rlst *) node;
  if ((unsigned long) block >= ctx->next_vcn)
    {
      if (read_run_list (ctx))
	return -1;
      return ctx->curr_lcn;
    }
  else
    return (ctx->flags & RF_BLNK) ? 0 : ((unsigned long) block -
					 ctx->curr_vcn + ctx->curr_lcn);
}

static grub_err_t
read_data (struct grub_ntfs_attr *at, char *pa, char *dest, unsigned long ofs,
	   unsigned long len, int cached,
	   void NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t sector,
					       unsigned offset,
					       unsigned length))
{
  unsigned long vcn;
  struct grub_ntfs_rlst cc, *ctx;
  grub_err_t ret;

  if (len == 0)
    return 0;

  grub_memset (&cc, 0, sizeof (cc));
  ctx = &cc;
  ctx->attr = at;
  ctx->comp.spc = at->mft->data->spc;
  ctx->comp.disk = at->mft->data->disk;

  if (pa[8] == 0)
    {
      if (ofs + len > u32at (pa, 0x10))
	return grub_error (GRUB_ERR_BAD_FS, "Read out of range");
      grub_memcpy (dest, pa + u32at (pa, 0x14) + ofs, len);
      return 0;
    }

  if (u16at (pa, 0xC) & FLAG_COMPRESSED)
    ctx->flags |= RF_COMP;
  else
    ctx->flags &= ~RF_COMP;
  ctx->cur_run = pa + u16at (pa, 0x20);

  if (ctx->flags & RF_COMP)
    {
      if (!cached)
	return grub_error (GRUB_ERR_BAD_FS, "Attribute can\'t be compressed");

      if (at->sbuf)
	{
	  if ((ofs & (~(COM_LEN - 1))) == at->save_pos)
	    {
	      unsigned long n;

	      n = COM_LEN - (ofs - at->save_pos);
	      if (n > len)
		n = len;

	      grub_memcpy (dest, at->sbuf + ofs - at->save_pos, n);
	      if (n == len)
		return 0;

	      dest += n;
	      len -= n;
	      ofs += n;
	    }
	}
      else
	{
	  at->sbuf = grub_malloc (COM_LEN);
	  if (at->sbuf == NULL)
	    return grub_errno;
	  at->save_pos = 1;
	}

      vcn = ctx->target_vcn = (ofs / COM_LEN) * (COM_SEC / ctx->comp.spc);
      ctx->target_vcn &= ~0xF;
    }
  else
    vcn = ctx->target_vcn = (ofs >> BLK_SHR) / ctx->comp.spc;

  ctx->next_vcn = u32at (pa, 0x10);
  ctx->curr_lcn = 0;
  while (ctx->next_vcn <= ctx->target_vcn)
    {
      if (read_run_list (ctx))
	return grub_errno;
    }

  if (at->flags & AF_GPOS)
    {
      unsigned long st0, st1;

      st0 =
	(ctx->target_vcn - ctx->curr_vcn + ctx->curr_lcn) * ctx->comp.spc +
	((ofs >> BLK_SHR) % ctx->comp.spc);
      st1 = st0 + 1;
      if (st1 ==
	  (ctx->next_vcn - ctx->curr_vcn + ctx->curr_lcn) * ctx->comp.spc)
	{
	  if (read_run_list (ctx))
	    return grub_errno;
	  st1 = ctx->curr_lcn * ctx->comp.spc;
	}
      v32at (dest, 0) = st0;
      v32at (dest, 4) = st1;
      return 0;
    }

  if (!(ctx->flags & RF_COMP))
    {
      unsigned int pow;

      if (!grub_fshelp_log2blksize (ctx->comp.spc, &pow))
	grub_fshelp_read_file (ctx->comp.disk, (grub_fshelp_node_t) ctx,
			       read_hook, ofs, len, dest,
			       grub_ntfs_read_block, ofs + len, pow);
      return grub_errno;
    }

  ctx->comp.comp_head = ctx->comp.comp_tail = 0;
  ctx->comp.cbuf = grub_malloc ((ctx->comp.spc) << BLK_SHR);
  if (!ctx->comp.cbuf)
    return 0;

  ret = 0;

  //ctx->comp.disk->read_hook = read_hook;

  if ((vcn > ctx->target_vcn) &&
      (read_block
       (ctx, NULL, ((vcn - ctx->target_vcn) * ctx->comp.spc) / COM_SEC)))
    {
      ret = grub_errno;
      goto quit;
    }

  if (ofs % COM_LEN)
    {
      unsigned long t, n, o;

      t = ctx->target_vcn * (ctx->comp.spc << BLK_SHR);
      if (read_block (ctx, at->sbuf, 1))
	{
	  ret = grub_errno;
	  goto quit;
	}

      at->save_pos = t;

      o = ofs % COM_LEN;
      n = COM_LEN - o;
      if (n > len)
	n = len;
      grub_memcpy (dest, &at->sbuf[o], n);
      if (n == len)
	goto quit;
      dest += n;
      len -= n;
    }

  if (read_block (ctx, dest, len / COM_LEN))
    {
      ret = grub_errno;
      goto quit;
    }

  dest += (len / COM_LEN) * COM_LEN;
  len = len % COM_LEN;
  if (len)
    {
      unsigned long t;

      t = ctx->target_vcn * (ctx->comp.spc << BLK_SHR);
      if (read_block (ctx, at->sbuf, 1))
	{
	  ret = grub_errno;
	  goto quit;
	}

      at->save_pos = t;

      grub_memcpy (dest, at->sbuf, len);
    }

quit:
  //ctx->comp.disk->read_hook = 0;
  if (ctx->comp.cbuf)
    grub_free (ctx->comp.cbuf);
  return ret;
}

static grub_err_t
read_attr (struct grub_ntfs_attr *at, char *dest, unsigned long ofs,
	   unsigned long len, int cached,
	   void NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t sector,
					       unsigned offset,
					       unsigned length))
{
  char *save_cur;
  unsigned char attr;
  char *pp;
  grub_err_t ret;

  save_cur = at->attr_cur;
  at->attr_nxt = at->attr_cur;
  attr = (unsigned char) *at->attr_nxt;
  if (at->flags & AF_ALST)
    {
      char *pa;
      unsigned long vcn;

      vcn = ofs / (at->mft->data->spc << BLK_SHR);
      pa = at->attr_nxt + u16at (at->attr_nxt, 4);
      while (pa < at->attr_end)
	{
	  if ((unsigned char) *pa != attr)
	    break;
	  if (u32at (pa, 8) > vcn)
	    break;
	  at->attr_nxt = pa;
	  pa += u16at (pa, 4);
	}
    }
  pp = find_attr (at, attr);
  if (pp)
    ret = read_data (at, pp, dest, ofs, len, cached, read_hook);
  else
    ret =
      (grub_errno) ? grub_errno : grub_error (GRUB_ERR_BAD_FS,
					      "Attribute not found");
  at->attr_cur = save_cur;
  return ret;
}

static grub_err_t
read_mft (struct grub_ntfs_data *data, char *buf, unsigned long mftno)
{
  if (read_attr
      (&data->mmft.attr, buf, mftno * (data->mft_size << BLK_SHR),
       data->mft_size << BLK_SHR, 0, 0))
    return grub_error (GRUB_ERR_BAD_FS, "Read MFT 0x%X fails", mftno);
  return fixup (data, buf, data->mft_size, "FILE");
}

static grub_err_t
init_file (struct grub_ntfs_file *mft, grub_uint32_t mftno)
{
  unsigned short flag;

  mft->inode_read = 1;

  mft->buf = grub_malloc (mft->data->mft_size << BLK_SHR);
  if (mft->buf == NULL)
    return grub_errno;

  if (read_mft (mft->data, mft->buf, mftno))
    return grub_errno;

  flag = u16at (mft->buf, 0x16);
  if ((flag & 1) == 0)
    return grub_error (GRUB_ERR_BAD_FS, "MFT 0x%X is not in use", mftno);

  if ((flag & 2) == 0)
    {
      char *pa;

      pa = locate_attr (&mft->attr, mft, AT_DATA);
      if (pa == NULL)
	return grub_error (GRUB_ERR_BAD_FS, "No $DATA in MFT 0x%X", mftno);

      if (!pa[8])
	mft->size = u32at (pa, 0x10);
      else
	mft->size = u32at (pa, 0x30);

      if ((mft->attr.flags & AF_ALST) == 0)
	mft->attr.attr_end = 0;	/*  Don't jump to attribute list */
    }
  else
    init_attr (&mft->attr, mft);

  return 0;
}

static void
free_file (struct grub_ntfs_file *mft)
{
  free_attr (&mft->attr);
  grub_free (mft->buf);
}

static int
list_file (struct grub_ntfs_file *diro, char *pos,
	   int NESTED_FUNC_ATTR
	   (*hook) (const char *filename,
		    enum grub_fshelp_filetype filetype,
		    grub_fshelp_node_t node))
{
  char *np;
  int ns;

  while (1)
    {
      char *ustr;
      if (pos[0xC] & 2)		/* end signature */
	break;

      np = pos + 0x52;
      ns = (unsigned char) *(np - 2);
      if (ns)
	{
	  enum grub_fshelp_filetype type;
	  struct grub_ntfs_file *fdiro;

	  if (u16at (pos, 4))
	    {
	      grub_error (GRUB_ERR_BAD_FS, "64-bit MFT number");
	      return 0;
	    }

	  type =
	    (u32at (pos, 0x48) & ATTR_DIRECTORY) ? GRUB_FSHELP_DIR :
	    GRUB_FSHELP_REG;

	  fdiro = grub_malloc (sizeof (struct grub_ntfs_file));
	  if (!fdiro)
	    return 0;

	  grub_memset (fdiro, 0, sizeof (*fdiro));
	  fdiro->data = diro->data;
	  fdiro->ino = u32at (pos, 0);

	  ustr = grub_malloc (ns * 4 + 1);
	  if (ustr == NULL)
	    return 0;
	  *grub_utf16_to_utf8 ((grub_uint8_t *) ustr, (grub_uint16_t *) np,
			       ns) = '\0';

	  if (hook (ustr, type, fdiro))
	    {
	      grub_free (ustr);
	      return 1;
	    }

	  grub_free (ustr);
	}
      pos += u16at (pos, 8);
    }
  return 0;
}

static int
grub_ntfs_iterate_dir (grub_fshelp_node_t dir,
		       int NESTED_FUNC_ATTR
		       (*hook) (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node))
{
  unsigned char *bitmap;
  struct grub_ntfs_attr attr, *at;
  char *cur_pos, *indx, *bmp;
  int bitmap_len, ret = 0;
  struct grub_ntfs_file *mft;

  mft = (struct grub_ntfs_file *) dir;

  if (!mft->inode_read)
    {
      if (init_file (mft, mft->ino))
	return 0;
    }

  indx = NULL;
  bmp = NULL;

  at = &attr;
  init_attr (at, mft);
  while (1)
    {
      if ((cur_pos = find_attr (at, AT_INDEX_ROOT)) == NULL)
	{
	  grub_error (GRUB_ERR_BAD_FS, "No $INDEX_ROOT");
	  goto done;
	}

      /* Resident, Namelen=4, Offset=0x18, Flags=0x00, Name="$I30" */
      if ((u32at (cur_pos, 8) != 0x180400) ||
	  (u32at (cur_pos, 0x18) != 0x490024) ||
	  (u32at (cur_pos, 0x1C) != 0x300033))
	continue;
      cur_pos += u16at (cur_pos, 0x14);
      if (*cur_pos != 0x30)	/* Not filename index */
	continue;
      break;
    }

  cur_pos += 0x10;		/* Skip index root */
  ret = list_file (mft, cur_pos + u16at (cur_pos, 0), hook);
  if (ret)
    goto done;

  bitmap = NULL;
  bitmap_len = 0;
  free_attr (at);
  init_attr (at, mft);
  while ((cur_pos = find_attr (at, AT_BITMAP)) != NULL)
    {
      int ofs;

      ofs = (unsigned char) cur_pos[0xA];
      /* Namelen=4, Name="$I30" */
      if ((cur_pos[9] == 4) &&
	  (u32at (cur_pos, ofs) == 0x490024) &&
	  (u32at (cur_pos, ofs + 4) == 0x300033))
	{
	  if ((at->flags & AF_ALST) && (cur_pos[8] == 0))
	    {
	      grub_error (GRUB_ERR_BAD_FS,
			  "$BITMAP should be non-resident when in attribute list");
	      goto done;
	    }
	  if (cur_pos[8] == 0)
	    {
	      bitmap = (unsigned char *) (cur_pos + u16at (cur_pos, 0x14));
	      bitmap_len = u32at (cur_pos, 0x10);
	      break;
	    }
	  if (u32at (cur_pos, 0x28) > BMP_LEN)
	    {
	      grub_error (GRUB_ERR_BAD_FS, "Non-resident $BITMAP too large");
	      goto done;
	    }
	  bmp = grub_malloc (u32at (cur_pos, 0x28));
	  if (bmp == NULL)
	    goto done;

	  bitmap = (unsigned char *) bmp;
	  bitmap_len = u32at (cur_pos, 0x30);
	  if (read_data (at, cur_pos, bmp, 0, u32at (cur_pos, 0x28), 0, 0))
	    {
	      grub_error (GRUB_ERR_BAD_FS,
			  "Fails to read non-resident $BITMAP");
	      goto done;
	    }
	  break;
	}
    }

  free_attr (at);
  cur_pos = locate_attr (at, mft, AT_INDEX_ALLOCATION);
  while (cur_pos != NULL)
    {
      /* Non-resident, Namelen=4, Offset=0x40, Flags=0, Name="$I30" */
      if ((u32at (cur_pos, 8) == 0x400401) &&
	  (u32at (cur_pos, 0x40) == 0x490024) &&
	  (u32at (cur_pos, 0x44) == 0x300033))
	break;
      cur_pos = find_attr (at, AT_INDEX_ALLOCATION);
    }

  if ((!cur_pos) && (bitmap))
    {
      grub_error (GRUB_ERR_BAD_FS, "$BITMAP without $INDEX_ALLOCATION");
      goto done;
    }

  if (bitmap)
    {
      unsigned long v, i;

      indx = grub_malloc (mft->data->idx_size << BLK_SHR);
      if (indx == NULL)
	goto done;

      v = 1;
      for (i = 0; i < (unsigned long) bitmap_len * 8; i++)
	{
	  if (*bitmap & v)
	    {
	      if ((read_attr
		   (at, indx, i * (mft->data->idx_size << BLK_SHR),
		    (mft->data->idx_size << BLK_SHR), 0, 0))
		  || (fixup (mft->data, indx, mft->data->idx_size, "INDX")))
		goto done;
	      ret = list_file (mft, &indx[0x18 + u16at (indx, 0x18)], hook);
	      if (ret)
		goto done;
	    }
	  v <<= 1;
	  if (v >= 0x100)
	    {
	      v = 1;
	      bitmap++;
	    }
	}
    }

done:
  free_attr (at);
  grub_free (indx);
  grub_free (bmp);

  return ret;
}

static struct grub_ntfs_data *
grub_ntfs_mount (grub_disk_t disk)
{
  struct grub_ntfs_bpb bpb;
  struct grub_ntfs_data *data = 0;

  if (!disk)
    goto fail;

  data = (struct grub_ntfs_data *) grub_malloc (sizeof (*data));
  if (!data)
    goto fail;

  grub_memset (data, 0, sizeof (*data));

  data->disk = disk;

  /* Read the BPB.  */
  if (grub_disk_read (disk, 0, 0, sizeof (bpb), (char *) &bpb))
    goto fail;

  if (grub_memcmp ((char *) &bpb.oem_name, "NTFS", 4))
    goto fail;

  data->blocksize = grub_le_to_cpu16 (bpb.bytes_per_sector);
  data->spc = bpb.sectors_per_cluster * (data->blocksize >> BLK_SHR);

  if (bpb.clusters_per_mft > 0)
    data->mft_size = data->spc * bpb.clusters_per_mft;
  else
    data->mft_size = 1 << (-bpb.clusters_per_mft - BLK_SHR);

  if (bpb.clusters_per_index > 0)
    data->idx_size = data->spc * bpb.clusters_per_index;
  else
    data->idx_size = 1 << (-bpb.clusters_per_index - BLK_SHR);

  data->mft_start = grub_le_to_cpu64 (bpb.mft_lcn) * data->spc;

  if ((data->mft_size > MAX_MFT) || (data->idx_size > MAX_IDX) ||
      (data->spc > MAX_SPC) || (data->spc > data->idx_size))
    goto fail;

  data->mmft.data = data;
  data->cmft.data = data;

  data->mmft.buf = grub_malloc (data->mft_size << BLK_SHR);
  if (!data->mmft.buf)
    goto fail;

  if (grub_disk_read
      (disk, data->mft_start, 0, data->mft_size << BLK_SHR, data->mmft.buf))
    goto fail;

  if (fixup (data, data->mmft.buf, data->mft_size, "FILE"))
    goto fail;

  if (!locate_attr (&data->mmft.attr, &data->mmft, AT_DATA))
    goto fail;

  if (init_file (&data->cmft, FILE_ROOT))
    goto fail;

  return data;

fail:
  grub_error (GRUB_ERR_BAD_FS, "not an ntfs filesystem");

  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
    }
  return 0;
}

static grub_err_t
grub_ntfs_dir (grub_device_t device, const char *path,
	       int (*hook) (const char *filename, int dir))
{
  struct grub_ntfs_data *data = 0;
  struct grub_fshelp_node *fdiro = 0;

  auto int NESTED_FUNC_ATTR iterate (const char *filename,
				     enum grub_fshelp_filetype filetype,
				     grub_fshelp_node_t node);

  int NESTED_FUNC_ATTR iterate (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node)
  {
    grub_free (node);

    if (filetype == GRUB_FSHELP_DIR)
      return hook (filename, 1);
    else
      return hook (filename, 0);

    return 0;
  }

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif


  data = grub_ntfs_mount (device->disk);
  if (!data)
    goto fail;

  grub_fshelp_find_file (path, &data->cmft, &fdiro, grub_ntfs_iterate_dir,
			 0, GRUB_FSHELP_DIR);

  if (grub_errno)
    goto fail;

  grub_ntfs_iterate_dir (fdiro, iterate);

fail:
  if ((fdiro) && (fdiro != &data->cmft))
    {
      free_file (fdiro);
      grub_free (fdiro);
    }
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      grub_free (data);
    }

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_ntfs_open (grub_file_t file, const char *name)
{
  struct grub_ntfs_data *data = 0;
  struct grub_fshelp_node *mft = 0;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_ntfs_mount (file->device->disk);
  if (!data)
    goto fail;

  grub_fshelp_find_file (name, &data->cmft, &mft, grub_ntfs_iterate_dir,
			 0, GRUB_FSHELP_REG);

  if (grub_errno)
    goto fail;

  if (mft != &data->cmft)
    {
      free_file (&data->cmft);
      grub_memcpy (&data->cmft, mft, sizeof (*mft));
      grub_free (mft);
      if (!data->cmft.inode_read)
	{
	  if (init_file (&data->cmft, data->cmft.ino))
	    goto fail;
	}
    }

  file->size = data->cmft.size;
  file->data = data;
  file->offset = 0;

  return 0;

fail:
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      grub_free (data);
    }

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_ssize_t
grub_ntfs_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_ntfs_file *mft;

  mft = &((struct grub_ntfs_data *) file->data)->cmft;
  if (file->read_hook)
    mft->attr.save_pos = 1;

  if (file->offset > file->size)
    {
      grub_error (GRUB_ERR_BAD_FS, "Bad offset");
      return -1;
    }

  if (file->offset + len > file->size)
    len = file->size - file->offset;

  read_attr (&mft->attr, buf, file->offset, len, 1, file->read_hook);
  return (grub_errno) ? 0 : len;
}

static grub_err_t
grub_ntfs_close (grub_file_t file)
{
  struct grub_ntfs_data *data;

  data = file->data;

  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      grub_free (data);
    }

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_ntfs_label (grub_device_t device, char **label)
{
  struct grub_ntfs_data *data = 0;
  struct grub_fshelp_node *mft = 0;
  char *pa;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  *label = 0;

  data = grub_ntfs_mount (device->disk);
  if (!data)
    goto fail;

  grub_fshelp_find_file ("/$Volume", &data->cmft, &mft, grub_ntfs_iterate_dir,
			 0, GRUB_FSHELP_REG);

  if (grub_errno)
    goto fail;

  if (!mft->inode_read)
    {
      mft->buf = grub_malloc (mft->data->mft_size << BLK_SHR);
      if (mft->buf == NULL)
	goto fail;

      if (read_mft (mft->data, mft->buf, mft->ino))
	goto fail;
    }

  init_attr (&mft->attr, mft);
  pa = find_attr (&mft->attr, AT_VOLUME_NAME);
  if ((pa) && (pa[8] == 0) && (u32at (pa, 0x10)))
    {
      char *buf;
      int len;

      len = u32at (pa, 0x10) / 2;
      buf = grub_malloc (len * 4 + 1);
      pa += u16at (pa, 0x14);
      *grub_utf16_to_utf8 ((grub_uint8_t *) buf, (grub_uint16_t *) pa, len) =
	'\0';
      *label = buf;
    }

fail:
  if ((mft) && (mft != &data->cmft))
    {
      free_file (mft);
      grub_free (mft);
    }
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      grub_free (data);
    }

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static struct grub_fs grub_ntfs_fs = {
  .name = "ntfs",
  .dir = grub_ntfs_dir,
  .open = grub_ntfs_open,
  .read = grub_ntfs_read,
  .close = grub_ntfs_close,
  .label = grub_ntfs_label,
  .next = 0
};

GRUB_MOD_INIT (ntfs)
{
  grub_fs_register (&grub_ntfs_fs);
#ifndef GRUB_UTIL
  my_mod = mod;
#endif
}

GRUB_MOD_FINI (ntfs)
{
  grub_fs_unregister (&grub_ntfs_fs);
}
