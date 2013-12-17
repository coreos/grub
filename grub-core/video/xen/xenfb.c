/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#define grub_video_render_target grub_video_fbrender_target

#include <grub/err.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/xen.h>
#include <xen/io/fbif.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct virtfb
{
  int handle;
  char *fullname;
  char *backend_dir;
  char *frontend_dir;
  struct xenfb_page *fbpage;
  grub_xen_grant_t grant;
  grub_xen_evtchn_t evtchn;
  char *framebuffer;
  grub_xen_mfn_t *page_directory;
  int width, height;
};

struct virtfb vfb;

static grub_xen_mfn_t
grub_xen_ptr2mfn (void *ptr)
{
  grub_xen_mfn_t *mfn_list =
    (grub_xen_mfn_t *) grub_xen_start_page_addr->mfn_list;
  return mfn_list[(grub_addr_t) ptr >> GRUB_XEN_LOG_PAGE_SIZE];
}

static int
fill (const char *dir, void *data __attribute__ ((unused)))
{
  domid_t dom;
  /* "dir" is just a number, at most 19 characters. */
  char fdir[200];
  char num[20];
  grub_err_t err;
  void *buf;
  struct evtchn_alloc_unbound alloc_unbound;

  if (vfb.fbpage)
    return 1;
  vfb.handle = grub_strtoul (dir, 0, 10);
  if (grub_errno)
    {
      grub_errno = 0;
      return 0;
    }
  vfb.fullname = 0;
  vfb.backend_dir = 0;

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/backend", dir);
  vfb.backend_dir = grub_xenstore_get_file (fdir, NULL);
  if (!vfb.backend_dir)
    goto out_fail_1;

  grub_snprintf (fdir, sizeof (fdir), "%s/dev",
		 vfb.backend_dir);

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/backend-id", dir);
  buf = grub_xenstore_get_file (fdir, NULL);
  if (!buf)
    goto out_fail_1;

  dom = grub_strtoul (buf, 0, 10);
  grub_free (buf);
  if (grub_errno)
    goto out_fail_1;

  vfb.fbpage =
    grub_xen_alloc_shared_page (dom, &vfb.grant);
  if (!vfb.fbpage)
    goto out_fail_1;

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/page-ref", dir);
  grub_snprintf (num, sizeof (num), "%llu", (unsigned long long) grub_xen_ptr2mfn (vfb.fbpage));
  err = grub_xenstore_write_file (fdir, num, grub_strlen (num));
  if (err)
    goto out_fail_3;

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/protocol", dir);
  err = grub_xenstore_write_file (fdir, XEN_IO_PROTO_ABI_NATIVE,
				  grub_strlen (XEN_IO_PROTO_ABI_NATIVE));
  if (err)
    goto out_fail_3;

  alloc_unbound.dom = DOMID_SELF;
  alloc_unbound.remote_dom = dom;

  grub_xen_event_channel_op (EVTCHNOP_alloc_unbound, &alloc_unbound);
  vfb.evtchn = alloc_unbound.port;

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/event-channel", dir);
  grub_snprintf (num, sizeof (num), "%u", vfb.evtchn);
  err = grub_xenstore_write_file (fdir, num, grub_strlen (num));
  if (err)
    goto out_fail_3;


  struct gnttab_dump_table dt;
  dt.dom = DOMID_SELF;
  grub_xen_grant_table_op (GNTTABOP_dump_table, (void *) &dt, 1);

  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s", dir);

  vfb.frontend_dir = grub_strdup (fdir);

  grub_size_t i;

  const unsigned pages_per_pd = (GRUB_XEN_PAGE_SIZE / sizeof (grub_xen_mfn_t));

  vfb.fbpage->in_cons = 0;
  vfb.fbpage->in_prod = 0;
  vfb.fbpage->out_cons = 0;
  vfb.fbpage->out_prod = 0;

  vfb.fbpage->width = vfb.width;
  vfb.fbpage->height = vfb.height;
  vfb.fbpage->line_length = vfb.width * 4;
  vfb.fbpage->depth = 32;

  grub_size_t fbsize = ALIGN_UP (vfb.width * vfb.height * 4, GRUB_XEN_PAGE_SIZE);

  vfb.fbpage->mem_length = fbsize;

  grub_size_t fbpages = fbsize / GRUB_XEN_PAGE_SIZE;
  grub_size_t pd_size = ALIGN_UP (fbpages, pages_per_pd);

  vfb.framebuffer = grub_memalign (GRUB_XEN_PAGE_SIZE, fbsize);
  if (!vfb.framebuffer)
    goto out_fail_3;
  vfb.page_directory = grub_memalign (GRUB_XEN_PAGE_SIZE, pd_size * sizeof (grub_xen_mfn_t));
  if (!vfb.page_directory)
    goto out_fail_3;

  for (i = 0; i < fbpages; i++)
    vfb.page_directory[i] = grub_xen_ptr2mfn (vfb.framebuffer + GRUB_XEN_PAGE_SIZE * i);
  for (; i < pd_size; i++)
    vfb.page_directory[i] = 0;

  for (i = 0; i < pd_size / pages_per_pd; i++)
    vfb.fbpage->pd[i] = grub_xen_ptr2mfn (vfb.page_directory + (GRUB_XEN_PAGE_SIZE / sizeof (vfb.page_directory[0])) * i);


  grub_snprintf (fdir, sizeof (fdir), "device/vfb/%s/state", dir);
  err = grub_xenstore_write_file (fdir, "3", 1);
  if (err)
    goto out_fail_3;

  while (1)
    {
      grub_snprintf (fdir, sizeof (fdir), "%s/state",
		     vfb.backend_dir);
      buf = grub_xenstore_get_file (fdir, NULL);
      if (!buf)
	goto out_fail_3;
      if (grub_strcmp (buf, "2") != 0)
	break;
      grub_free (buf);
      grub_xen_sched_op (SCHEDOP_yield, 0);
    }
  grub_dprintf ("xen", "state=%s\n", (char *) buf);
  grub_free (buf);

  return 1;

out_fail_3:
  grub_xen_free_shared_page (vfb.fbpage);
out_fail_1:
  grub_free (vfb.framebuffer);
  vfb.framebuffer = 0;
  grub_free (vfb.page_directory);
  vfb.page_directory = 0;

  vfb.fbpage = 0;
  grub_free (vfb.backend_dir);
  grub_free (vfb.fullname);

  grub_errno = 0;
  return 0;
}

static void
vfb_fini (void)
{
  char fdir[200];

  char *buf;
  struct evtchn_close close_op = {.port = vfb.evtchn };

  if (!vfb.fbpage)
    return;

  grub_snprintf (fdir, sizeof (fdir), "%s/state",
		 vfb.frontend_dir);
  grub_xenstore_write_file (fdir, "6", 1);

  while (1)
    {
      grub_snprintf (fdir, sizeof (fdir), "%s/state",
		     vfb.backend_dir);
      buf = grub_xenstore_get_file (fdir, NULL);
      grub_dprintf ("xen", "state=%s\n", (char *) buf);

      if (!buf || grub_strcmp (buf, "6") == 0)
	break;
      grub_free (buf);
      grub_xen_sched_op (SCHEDOP_yield, 0);
    }
  grub_free (buf);

  grub_snprintf (fdir, sizeof (fdir), "%s/page-ref",
		 vfb.frontend_dir);
  grub_xenstore_write_file (fdir, NULL, 0);

  grub_snprintf (fdir, sizeof (fdir), "%s/event-channel",
		 vfb.frontend_dir);
  grub_xenstore_write_file (fdir, NULL, 0);

  grub_xen_free_shared_page (vfb.fbpage);
  vfb.fbpage = 0;

  grub_xen_event_channel_op (EVTCHNOP_close, &close_op);

  /* Prepare for handoff.  */
  grub_snprintf (fdir, sizeof (fdir), "%s/state",
		 vfb.frontend_dir);
  grub_xenstore_write_file (fdir, "1", 1);

  grub_free (vfb.framebuffer);
  vfb.framebuffer = 0;
  grub_free (vfb.page_directory);
  vfb.page_directory = 0;
}

static grub_err_t
vfb_init (int width, int height)
{
  if (vfb.fbpage)
    vfb_fini ();
  vfb.width = width;
  vfb.height = height;
  grub_xenstore_dir ("device/vfb", fill, NULL);
  if (vfb.fbpage)
    grub_errno = 0;
  if (!vfb.fbpage)
    return grub_error (GRUB_ERR_IO, "couldn't init vfb");
  return GRUB_ERR_NONE;
}

static void
grub_video_xenfb_fill_mode_info (struct grub_video_mode_info *out)
{
  grub_memset (out, 0, sizeof (*out));

  out->pitch = vfb.width * 4;
  out->width = vfb.width;
  out->height = vfb.height;

  out->mode_type = GRUB_VIDEO_MODE_TYPE_RGB;
  out->bpp = 32;
  out->bytes_per_pixel = 4;
  out->number_of_colors = 256;
  out->reserved_mask_size = 8;
  out->reserved_field_pos = 24;
  out->red_mask_size = 8;
  out->red_field_pos = 16;
  out->green_mask_size = 8;
  out->green_field_pos = 8;
  out->blue_mask_size = 8;
  out->blue_field_pos = 0;

  out->blit_format = grub_video_get_blit_format (out);
}

static grub_err_t
grub_video_ieee1275_setup (unsigned int width, unsigned int height,
			   unsigned int mode_type __attribute__ ((unused)),
			   unsigned int mode_mask __attribute__ ((unused)))
{
  struct grub_video_mode_info modeinfo;
  grub_err_t err;

  err = vfb_init (width ? : 800, height ? : 600);
  if (err)
    return err;
  grub_video_xenfb_fill_mode_info (&modeinfo);

  err = grub_video_fb_setup (mode_type, mode_mask,
			     &modeinfo,
			     vfb.framebuffer, NULL, NULL);
  if (err)
    return err;

  err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				   grub_video_fbstd_colors);
    
  return err;
}

static grub_err_t
grub_video_xenfb_get_info_and_fini (struct grub_video_mode_info *mode_info,
				  void **framebuf)
{
  grub_video_xenfb_fill_mode_info (mode_info);
  *framebuf = vfb.framebuffer;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vfb_fini (void)
{
  vfb_fini ();
  return grub_video_fb_fini ();
}


static struct grub_video_adapter grub_video_xen_adapter =
  {
    .name = "XEN video driver",

    .prio = GRUB_VIDEO_ADAPTER_PRIO_NATIVE,
    .id = GRUB_VIDEO_DRIVER_XEN,

    .init = grub_video_fb_init,
    .fini = grub_video_vfb_fini,
    .setup = grub_video_ieee1275_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_xenfb_get_info_and_fini,
    .set_palette = grub_video_fb_set_palette,
    .get_palette = grub_video_fb_get_palette,
    .set_viewport = grub_video_fb_set_viewport,
    .get_viewport = grub_video_fb_get_viewport,
    .set_region = grub_video_fb_set_region,
    .get_region = grub_video_fb_get_region,
    .set_area_status = grub_video_fb_set_area_status,
    .get_area_status = grub_video_fb_get_area_status,
    .map_color = grub_video_fb_map_color,
    .map_rgb = grub_video_fb_map_rgb,
    .map_rgba = grub_video_fb_map_rgba,
    .unmap_color = grub_video_fb_unmap_color,
    .fill_rect = grub_video_fb_fill_rect,
    .blit_bitmap = grub_video_fb_blit_bitmap,
    .blit_render_target = grub_video_fb_blit_render_target,
    .scroll = grub_video_fb_scroll,
    .swap_buffers = grub_video_fb_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_fb_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

void
grub_video_xen_init (void)
{
  grub_video_register (&grub_video_xen_adapter);
}

void
grub_video_xen_fini (void)
{
  vfb_fini ();
  grub_video_unregister (&grub_video_xen_adapter);
}
