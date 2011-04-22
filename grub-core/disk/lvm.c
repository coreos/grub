/* lvm.c - module to read Logical Volumes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/lvm.h>

#ifdef GRUB_UTIL
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");

static struct grub_lvm_vg *vg_list;
static int lv_count;
static int scan_depth = 0;

static int is_lv_readable (struct grub_lvm_lv *lv);

static int
grub_lvm_scan_device (const char *name);

/* Go the string STR and return the number after STR.  *P will point
   at the number.  In case STR is not found, *P will be NULL and the
   return value will be 0.  */
static int
grub_lvm_getvalue (char **p, char *str)
{
  *p = grub_strstr (*p, str);
  if (! *p)
    return 0;
  *p += grub_strlen (str);
  return grub_strtoul (*p, NULL, 10);
}

#if 0
static int
grub_lvm_checkvalue (char **p, char *str, char *tmpl)
{
  int tmpllen = grub_strlen (tmpl);
  *p = grub_strstr (*p, str);
  if (! *p)
    return 0;
  *p += grub_strlen (str);
  if (**p != '"')
    return 0;
  return (grub_memcmp (*p + 1, tmpl, tmpllen) == 0 && (*p)[tmpllen + 1] == '"');
}
#endif

static int
grub_lvm_check_flag (char *p, char *str, char *flag)
{
  int len_str = grub_strlen (str), len_flag = grub_strlen (flag);
  while (1)
    {
      char *q;
      p = grub_strstr (p, str);
      if (! p)
	return 0;
      p += len_str;
      if (grub_memcmp (p, " = [", sizeof (" = [") - 1) != 0)
	continue;
      q = p + sizeof (" = [") - 1;
      while (1)
	{
	  while (grub_isspace (*q))
	    q++;
	  if (*q != '"')
	    return 0;
	  q++;
	  if (grub_memcmp (q, flag, len_flag) == 0 && q[len_flag] == '"')
	    return 1;
	  while (*q != '"')
	    q++;
	  q++;
	  if (*q == ']')
	    return 0;
	  q++;
	}
    }
}

static int
grub_lvm_iterate (int (*hook) (const char *name),
		  grub_disk_pull_t pull)
{
  struct grub_lvm_vg *vg;
  unsigned old_count = 0;
  if (pull == GRUB_DISK_PULL_RESCAN && scan_depth)
    return 0;

  if (pull == GRUB_DISK_PULL_RESCAN)
    {
      old_count = lv_count;
      if (!scan_depth)
	{
	  scan_depth++;
	  grub_device_iterate (&grub_lvm_scan_device);
	  scan_depth--;
	}
    }
  if (pull != GRUB_DISK_PULL_RESCAN && pull != GRUB_DISK_PULL_NONE)
    return GRUB_ERR_NONE;
  for (vg = vg_list; vg; vg = vg->next)
    {
      struct grub_lvm_lv *lv;
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (lv->visible && lv->number >= old_count)
	    {
	      char lvname[sizeof ("lvm/") + grub_strlen (lv->name)];
	      grub_memcpy (lvname, "lvm/", sizeof ("lvm/") - 1);
	      grub_strcpy (lvname + sizeof ("lvm/") - 1, lv->name);
	      if (hook (lvname))
		return 1;
	    }
    }

  return 0;
}

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_lvm_memberlist (grub_disk_t disk)
{
  struct grub_lvm_lv *lv = disk->data;
  grub_disk_memberlist_t list = NULL, tmp;
  struct grub_lvm_pv *pv;

  if (lv->vg->pvs)
    for (pv = lv->vg->pvs; pv; pv = pv->next)
      {
	if (!pv->disk)
	  grub_util_error ("Couldn't find PV %s. Check your device.map",
			   pv->name);
	tmp = grub_malloc (sizeof (*tmp));
	tmp->disk = pv->disk;
	tmp->next = list;
	list = tmp;
      }

  return list;
}
#endif

static struct grub_lvm_lv *
find_lv (const char *name)
{
  struct grub_lvm_vg *vg;
  struct grub_lvm_lv *lv = NULL;
  for (vg = vg_list; vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if ((grub_strcmp (lv->fullname, name) == 0
	       || grub_strcmp (lv->compatname, name) == 0)
	      && is_lv_readable (lv))
	    return lv;
    }
  return NULL;
}

static const char *scan_for = NULL;

static grub_err_t
grub_lvm_open (const char *name, grub_disk_t disk,
	       grub_disk_pull_t pull)
{
  struct grub_lvm_lv *lv = NULL;
  int explicit = 0;

  if (grub_memcmp (name, "lvm/", sizeof ("lvm/") - 1) == 0)
    explicit = 1;

  lv = find_lv (name);

  if (! lv && !scan_depth &&
      pull == (explicit ? GRUB_DISK_PULL_RESCAN : GRUB_DISK_PULL_RESCAN_UNTYPED))
    {
      scan_for = name;
      scan_depth++;
      grub_device_iterate (&grub_lvm_scan_device);
      scan_depth--;
      scan_for = NULL;
      if (grub_errno)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	}
      lv = find_lv (name);
    }

  if (! lv)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown LVM device %s", name);

  disk->id = lv->number;
  disk->data = lv;
  disk->total_sectors = lv->size;

  return 0;
}

static void
grub_lvm_close (grub_disk_t disk __attribute ((unused)))
{
  return;
}

static grub_err_t
read_lv (struct grub_lvm_lv *lv, grub_disk_addr_t sector,
	 grub_size_t size, char *buf);

static grub_err_t
read_node (const struct grub_lvm_node *node, grub_disk_addr_t sector,
	   grub_size_t size, char *buf)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    {
      if (node->pv->disk)
	return grub_disk_read (node->pv->disk, sector + node->pv->start, 0,
			       size << GRUB_DISK_SECTOR_BITS, buf);
      else
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			   "physical volume %s not found", node->pv->name);

    }
  if (node->lv)
    return read_lv (node->lv, sector, size, buf);
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown node '%s'", node->name);
}

static grub_err_t
read_lv (struct grub_lvm_lv *lv, grub_disk_addr_t sector,
	 grub_size_t size, char *buf)
{
  grub_err_t err = 0;
  struct grub_lvm_vg *vg = lv->vg;
  struct grub_lvm_segment *seg = lv->segments;
  struct grub_lvm_node *node;
  grub_uint64_t offset;
  grub_uint64_t extent;
  unsigned int i;

  if (!lv)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown volume");

  extent = grub_divmod64 (sector, vg->extent_size, NULL);

  /* Find the right segment.  */
  for (i = 0; i < lv->segment_count; i++)
    {
      if ((seg->start_extent <= extent)
	  && ((seg->start_extent + seg->extent_count) > extent))
	{
	  break;
	}

      seg++;
    }

  if (i == lv->segment_count)
    return grub_error (GRUB_ERR_READ_ERROR, "incorrect segment");

  switch (seg->type)
    {
    case GRUB_LVM_STRIPED:
      if (seg->node_count == 1)
	{
	  /* This segment is linear, so that's easy.  We just need to find
	     out the offset in the physical volume and read SIZE bytes
	     from that.  */
	  struct grub_lvm_node *stripe = seg->nodes;
	  grub_uint64_t seg_offset; /* Offset of the segment in PV device.  */

	  node = stripe;
	  seg_offset = ((grub_uint64_t) stripe->start
			* (grub_uint64_t) vg->extent_size);

	  offset = sector - ((grub_uint64_t) seg->start_extent
			     * (grub_uint64_t) vg->extent_size) + seg_offset;
	}
      else
	{
	  /* This is a striped segment. We have to find the right PV
	     similar to RAID0. */
	  struct grub_lvm_node *stripe = seg->nodes;
	  grub_uint32_t a, b;
	  grub_uint64_t seg_offset; /* Offset of the segment in PV device.  */
	  unsigned int stripenr;

	  offset = sector - ((grub_uint64_t) seg->start_extent
			     * (grub_uint64_t) vg->extent_size);

	  a = grub_divmod64 (offset, seg->stripe_size, NULL);
	  grub_divmod64 (a, seg->node_count, &stripenr);

	  a = grub_divmod64 (offset, seg->stripe_size * seg->node_count, NULL);
	  grub_divmod64 (offset, seg->stripe_size, &b);
	  offset = a * seg->stripe_size + b;

	  stripe += stripenr;
	  node = stripe;

	  seg_offset = ((grub_uint64_t) stripe->start
			* (grub_uint64_t) vg->extent_size);

	  offset += seg_offset;
	}
      return read_node (node, offset, size, buf);
    case GRUB_LVM_MIRROR:
      i = 0;
      while (1)
	{
	  err = read_node (&seg->nodes[i], sector, size, buf);
	  if (!err)
	    return err;
	  if (++i >= seg->node_count)
	    return err;
	  grub_errno = GRUB_ERR_NONE;
	}
    }
  return grub_error (GRUB_ERR_IO, "unknown LVM segment");
}

static grub_err_t
is_node_readable (const struct grub_lvm_node *node)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    return !!(node->pv->disk);
  if (node->lv)
    return is_lv_readable (node->lv);
  return 0;
}

static int
is_lv_readable (struct grub_lvm_lv *lv)
{
  unsigned int i, j;

  if (!lv)
    return 0;

  /* Find the right segment.  */
  for (i = 0; i < lv->segment_count; i++)
    switch (lv->segments[i].type)
      {
      case GRUB_LVM_STRIPED:
	for (j = 0; j < lv->segments[i].node_count; j++)
	  if (!is_node_readable (lv->segments[i].nodes + j))
	    return 0;
	break;
      case GRUB_LVM_MIRROR:
	for (j = 0; j < lv->segments[i].node_count; j++)
	  if (is_node_readable (lv->segments[i].nodes + j))
	    break;
	if (j == lv->segments[i].node_count)
	  return 0;
      default:
	return 0;
      }

  return 1;
}


static grub_err_t
grub_lvm_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  return read_lv (disk->data, sector, size, buf);
}

static grub_err_t
grub_lvm_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static int
grub_lvm_scan_device (const char *name)
{
  grub_err_t err;
  grub_disk_t disk;
  grub_uint64_t mda_offset, mda_size;
  char buf[GRUB_LVM_LABEL_SIZE];
  char vg_id[GRUB_LVM_ID_STRLEN+1];
  char pv_id[GRUB_LVM_ID_STRLEN+1];
  char *metadatabuf, *p, *q, *vgname;
  struct grub_lvm_label_header *lh = (struct grub_lvm_label_header *) buf;
  struct grub_lvm_pv_header *pvh;
  struct grub_lvm_disk_locn *dlocn;
  struct grub_lvm_mda_header *mdah;
  struct grub_lvm_raw_locn *rlocn;
  unsigned int i, j, vgname_len;
  struct grub_lvm_vg *vg;
  struct grub_lvm_pv *pv;

#ifdef GRUB_UTIL
  grub_util_info ("scanning %s for LVM", name);
#endif

  disk = grub_disk_open (name);
  if (!disk)
    {
      if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
	grub_errno = GRUB_ERR_NONE;
      return 0;
    }

  for (vg = vg_list; vg; vg = vg->next)
    for (pv = vg->pvs; pv; pv = pv->next)
      if (pv->disk && pv->disk->id == disk->id
	  && pv->disk->dev->id == disk->dev->id)
	{
	  grub_disk_close (disk);
	  return 0;
	}

  /* Search for label. */
  for (i = 0; i < GRUB_LVM_LABEL_SCAN_SECTORS; i++)
    {
      err = grub_disk_read (disk, i, 0, sizeof(buf), buf);
      if (err)
	goto fail;

      if ((! grub_strncmp ((char *)lh->id, GRUB_LVM_LABEL_ID,
			   sizeof (lh->id)))
	  && (! grub_strncmp ((char *)lh->type, GRUB_LVM_LVM2_LABEL,
			      sizeof (lh->type))))
	break;
    }

  /* Return if we didn't find a label. */
  if (i == GRUB_LVM_LABEL_SCAN_SECTORS)
    {
#ifdef GRUB_UTIL
      grub_util_info ("no LVM signature found");
#endif
      goto fail;
    }

  pvh = (struct grub_lvm_pv_header *) (buf + grub_le_to_cpu32(lh->offset_xl));

  for (i = 0, j = 0; i < GRUB_LVM_ID_LEN; i++)
    {
      pv_id[j++] = pvh->pv_uuid[i];
      if ((i != 1) && (i != 29) && (i % 4 == 1))
	pv_id[j++] = '-';
    }
  pv_id[j] = '\0';

  dlocn = pvh->disk_areas_xl;

  dlocn++;
  /* Is it possible to have multiple data/metadata areas? I haven't
     seen devices that have it. */
  if (dlocn->offset)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "we don't support multiple LVM data areas");

#ifdef GRUB_UTIL
      grub_util_info ("we don't support multiple LVM data areas\n");
#endif
      goto fail;
    }

  dlocn++;
  mda_offset = grub_le_to_cpu64 (dlocn->offset);
  mda_size = grub_le_to_cpu64 (dlocn->size);

  /* It's possible to have multiple copies of metadata areas, we just use the
     first one.  */

  /* Allocate buffer space for the circular worst-case scenario. */
  metadatabuf = grub_malloc (2 * mda_size);
  if (! metadatabuf)
    goto fail;

  err = grub_disk_read (disk, 0, mda_offset, mda_size, metadatabuf);
  if (err)
    goto fail2;

  mdah = (struct grub_lvm_mda_header *) metadatabuf;
  if ((grub_strncmp ((char *)mdah->magic, GRUB_LVM_FMTT_MAGIC,
		     sizeof (mdah->magic)))
      || (grub_le_to_cpu32 (mdah->version) != GRUB_LVM_FMTT_VERSION))
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "unknown LVM metadata header");
#ifdef GRUB_UTIL
      grub_util_info ("unknown LVM metadata header\n");
#endif
      goto fail2;
    }

  rlocn = mdah->raw_locns;
  if (grub_le_to_cpu64 (rlocn->offset) + grub_le_to_cpu64 (rlocn->size) >
      grub_le_to_cpu64 (mdah->size))
    {
      /* Metadata is circular. Copy the wrap in place. */
      grub_memcpy (metadatabuf + mda_size,
                   metadatabuf + GRUB_LVM_MDA_HEADER_SIZE,
                   grub_le_to_cpu64 (rlocn->offset) +
                   grub_le_to_cpu64 (rlocn->size) -
                   grub_le_to_cpu64 (mdah->size));
    }
  p = q = metadatabuf + grub_le_to_cpu64 (rlocn->offset);

  while (*q != ' ' && q < metadatabuf + mda_size)
    q++;

  if (q == metadatabuf + mda_size)
    {
#ifdef GRUB_UTIL
      grub_util_info ("error parsing metadata\n");
#endif
      goto fail2;
    }

  vgname_len = q - p;
  vgname = grub_malloc (vgname_len + 1);
  if (!vgname)
    goto fail2;

  grub_memcpy (vgname, p, vgname_len);
  vgname[vgname_len] = '\0';

  p = grub_strstr (q, "id = \"");
  if (p == NULL)
    {
#ifdef GRUB_UTIL
      grub_util_info ("couldn't find ID\n");
#endif
      goto fail3;
    }
  p += sizeof ("id = \"") - 1;
  grub_memcpy (vg_id, p, GRUB_LVM_ID_STRLEN);
  vg_id[GRUB_LVM_ID_STRLEN] = '\0';

  for (vg = vg_list; vg; vg = vg->next)
    {
      if (! grub_memcmp(vg_id, vg->id, GRUB_LVM_ID_STRLEN))
	break;
    }

  if (! vg)
    {
      /* First time we see this volume group. We've to create the
	 whole volume group structure. */
      vg = grub_malloc (sizeof (*vg));
      if (! vg)
	goto fail3;
      vg->name = vgname;
      grub_memcpy (vg->id, vg_id, GRUB_LVM_ID_STRLEN+1);

      vg->extent_size = grub_lvm_getvalue (&p, "extent_size = ");
      if (p == NULL)
	{
#ifdef GRUB_UTIL
	  grub_util_info ("unknown extent size\n");
#endif
	  goto fail4;
	}

      vg->lvs = NULL;
      vg->pvs = NULL;

      p = grub_strstr (p, "physical_volumes {");
      if (p)
	{
	  p += sizeof ("physical_volumes {") - 1;

	  /* Add all the pvs to the volume group. */
	  while (1)
	    {
	      int s;
	      while (grub_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      pv = grub_malloc (sizeof (*pv));
	      q = p;
	      while (*q != ' ')
		q++;

	      s = q - p;
	      pv->name = grub_malloc (s + 1);
	      grub_memcpy (pv->name, p, s);
	      pv->name[s] = '\0';

	      p = grub_strstr (p, "id = \"");
	      if (p == NULL)
		goto pvs_fail;
	      p += sizeof("id = \"") - 1;

	      grub_memcpy (pv->id, p, GRUB_LVM_ID_STRLEN);
	      pv->id[GRUB_LVM_ID_STRLEN] = '\0';

	      pv->start = grub_lvm_getvalue (&p, "pe_start = ");
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("unknown pe_start\n");
#endif
		  goto pvs_fail;
		}

	      p = grub_strchr (p, '}');
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("error parsing pe_start\n");
#endif
		  goto pvs_fail;
		}
	      p++;

	      pv->disk = NULL;
	      pv->next = vg->pvs;
	      vg->pvs = pv;

	      continue;
	    pvs_fail:
	      grub_free (pv->name);
	      grub_free (pv);
	      goto fail4;
	    }
	}

      p = grub_strstr (p, "logical_volumes");
      if (p)
	{
	  p += 18;

	  /* And add all the lvs to the volume group. */
	  while (1)
	    {
	      int s;
	      int skip_lv = 0;
	      struct grub_lvm_lv *lv;
	      struct grub_lvm_segment *seg;
	      int is_pvmove;

	      while (grub_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      lv = grub_malloc (sizeof (*lv));

	      q = p;
	      while (*q != ' ')
		q++;

	      s = q - p;
	      lv->name = grub_strndup (p, s);
	      if (!lv->name)
		goto lvs_fail;
	      lv->compatname = grub_malloc (vgname_len + 1 + s + 1);
	      if (!lv->compatname)
		goto lvs_fail;
	      grub_memcpy (lv->compatname, vgname, vgname_len);
	      lv->compatname[vgname_len] = '-';
	      grub_memcpy (lv->compatname + vgname_len + 1, p, s);
	      lv->compatname[vgname_len + 1 + s] = '\0';

	      {
		const char *iptr;
		char *optr;
		lv->fullname = grub_malloc (sizeof("lvm/") + 2 * vgname_len
					    + 1 + 2 * s + 1);
		if (!lv->fullname)
		  goto lvs_fail;

		optr = lv->fullname;
		grub_memcpy (optr, "lvm/", sizeof ("lvm/") - 1);
		optr += sizeof ("lvm/") - 1;
		for (iptr = vgname; iptr < vgname + vgname_len; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = '-';
		for (iptr = p; iptr < p + s; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = 0;
	      }

	      lv->size = 0;

	      lv->visible = grub_lvm_check_flag (p, "status", "VISIBLE");
	      is_pvmove = grub_lvm_check_flag (p, "status", "PVMOVE");

	      lv->segment_count = grub_lvm_getvalue (&p, "segment_count = ");
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("unknown segment_count\n");
#endif
		  goto lvs_fail;
		}
	      lv->segments = grub_malloc (sizeof (*seg) * lv->segment_count);
	      seg = lv->segments;

	      for (i = 0; i < lv->segment_count; i++)
		{

		  p = grub_strstr (p, "segment");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown segment\n");
#endif
		      goto lvs_segment_fail;
		    }

		  seg->start_extent = grub_lvm_getvalue (&p, "start_extent = ");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown start_extent\n");
#endif
		      goto lvs_segment_fail;
		    }
		  seg->extent_count = grub_lvm_getvalue (&p, "extent_count = ");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown extent_count\n");
#endif
		      goto lvs_segment_fail;
		    }

		  p = grub_strstr (p, "type = \"");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  p += sizeof("type = \"") - 1;

		  lv->size += seg->extent_count * vg->extent_size;

		  if (grub_memcmp (p, "striped\"",
				   sizeof ("striped\"") - 1) == 0)
		    {
		      struct grub_lvm_node *stripe;

		      seg->type = GRUB_LVM_STRIPED;
		      seg->node_count = grub_lvm_getvalue (&p, "stripe_count = ");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown stripe_count\n");
#endif
			  goto lvs_segment_fail;
			}

		      if (seg->node_count != 1)
			seg->stripe_size = grub_lvm_getvalue (&p, "stripe_size = ");

		      seg->nodes = grub_zalloc (sizeof (*stripe)
						* seg->node_count);
		      stripe = seg->nodes;

		      p = grub_strstr (p, "stripes = [");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown stripes\n");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("stripes = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  p = grub_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  stripe->name = grub_malloc (s + 1);
			  if (stripe->name == NULL)
			    goto lvs_segment_fail2;

			  grub_memcpy (stripe->name, p, s);
			  stripe->name[s] = '\0';

			  stripe->start = grub_lvm_getvalue (&p, ",");
			  if (p == NULL)
			    continue;

			  stripe++;
			}
		    }
		  else if (grub_memcmp (p, "mirror\"", sizeof ("mirror\"") - 1)
			   == 0)
		    {
		      seg->type = GRUB_LVM_MIRROR;
		      seg->node_count = grub_lvm_getvalue (&p, "mirror_count = ");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown mirror_count\n");
#endif
			  goto lvs_segment_fail;
			}

		      seg->nodes = grub_zalloc (sizeof (seg->nodes[0])
						* seg->node_count);

		      p = grub_strstr (p, "mirrors = [");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown mirrors\n");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("mirrors = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  char *lvname;

			  p = grub_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  lvname = grub_malloc (s + 1);
			  if (lvname == NULL)
			    goto lvs_segment_fail2;

			  grub_memcpy (lvname, p, s);
			  lvname[s] = '\0';
			  seg->nodes[j].name = lvname;
			  p = q + 1;
			}
		      /* Only first (original) is ok with in progress pvmove.  */
		      if (is_pvmove)
			seg->node_count = 1;
		    }
		  else
		    {
#ifdef GRUB_UTIL
		      char *p2;
		      p2 = grub_strchr (p, '"');
		      if (p2)
			*p2 = 0;
		      grub_util_info ("unknown LVM type %s\n", p);
		      if (p2)
			*p2 ='"';
#endif
		      /* Found a non-supported type, give up and move on. */
		      skip_lv = 1;
		      break;
		    }

		  seg++;

		  continue;
		lvs_segment_fail2:
		  grub_free (seg->nodes);
		lvs_segment_fail:
		  goto fail4;
		}

	      if (p != NULL)
		p = grub_strchr (p, '}');
	      if (p == NULL)
		goto lvs_fail;
	      p += 3;

	      if (skip_lv)
		{
		  grub_free (lv->name);
		  grub_free (lv);
		  continue;
		}

	      lv->number = lv_count++;
	      lv->vg = vg;
	      lv->next = vg->lvs;
	      vg->lvs = lv;

	      continue;
	    lvs_fail:
	      grub_free (lv->name);
	      grub_free (lv);
	      goto fail4;
	    }
	}

      /* Match lvs.  */
      {
	struct grub_lvm_lv *lv1;
	struct grub_lvm_lv *lv2;
	for (lv1 = vg->lvs; lv1; lv1 = lv1->next)
	  for (i = 0; i < lv1->segment_count; i++)
	    for (j = 0; j < lv1->segments[i].node_count; j++)
	      {
		if (vg->pvs)
		  for (pv = vg->pvs; pv; pv = pv->next)
		    {
		      if (! grub_strcmp (pv->name,
					 lv1->segments[i].nodes[j].name))
			{
			  lv1->segments[i].nodes[j].pv = pv;
			  break;
			}
		    }
		if (lv1->segments[i].nodes[j].pv == NULL)
		  for (lv2 = vg->lvs; lv2; lv2 = lv2->next)
		    if (grub_strcmp (lv2->name + grub_strlen (vg->name) + 1,
				     lv1->segments[i].nodes[j].name) == 0)
		      lv1->segments[i].nodes[j].lv = lv2;
	      }
	
      }

	vg->next = vg_list;
	vg_list = vg;
    }
  else
    {
      grub_free (vgname);
    }

  /* Match the device we are currently reading from with the right
     PV. */
  if (vg->pvs)
    for (pv = vg->pvs; pv; pv = pv->next)
      {
	if (! grub_memcmp (pv->id, pv_id, GRUB_LVM_ID_STRLEN))
	  {
	    /* This could happen to LVM on RAID, pv->disk points to the
	       raid device, we shouldn't change it.  */
	    if (! pv->disk)
	      pv->disk = grub_disk_open (name);
	    break;
	  }
      }

  goto fail2;

  /* Failure path.  */
 fail4:
  grub_free (vg);
 fail3:
  grub_free (vgname);

  /* Normal exit path.  */
 fail2:
  grub_free (metadatabuf);
 fail:
  grub_disk_close (disk);
  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_errno = GRUB_ERR_NONE;
  grub_print_error ();
  if (scan_for && find_lv (scan_for))
    return 1;
  return 0;
}

static struct grub_disk_dev grub_lvm_dev =
  {
    .name = "lvm",
    .id = GRUB_DISK_DEVICE_LVM_ID,
    .iterate = grub_lvm_iterate,
    .open = grub_lvm_open,
    .close = grub_lvm_close,
    .read = grub_lvm_read,
    .write = grub_lvm_write,
#ifdef GRUB_UTIL
    .memberlist = grub_lvm_memberlist,
#endif
    .next = 0
  };


GRUB_MOD_INIT(lvm)
{
  grub_disk_dev_register (&grub_lvm_dev);
}

GRUB_MOD_FINI(lvm)
{
  grub_disk_dev_unregister (&grub_lvm_dev);
  vg_list = NULL;
  /* FIXME: free the lvm list. */
}
