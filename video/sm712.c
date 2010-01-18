/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/pci.h>

static struct
{
  struct grub_video_mode_info mode_info;
  struct grub_video_render_target *render_target;

  unsigned int bytes_per_scan_line;
  unsigned int bytes_per_pixel;
  grub_uint8_t *ptr;
  int index_color_mode;
  int mapped;
  grub_uint32_t base;
  grub_pci_device_t dev;
} framebuffer;

static grub_err_t
grub_video_sm712_video_init (void)
{
  /* Reset frame buffer.  */
  grub_memset (&framebuffer, 0, sizeof(framebuffer));

  return grub_video_fb_init ();
}

static grub_err_t
grub_video_sm712_video_fini (void)
{
  if (framebuffer.mapped)
    grub_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				 1024 * 600 * 2);

  return grub_video_fb_fini ();
}

static grub_err_t
grub_video_sm712_setup (unsigned int width, unsigned int height,
			unsigned int mode_type, unsigned int mode_mask __attribute__ ((unused)))
{
  int depth;
  grub_err_t err;
  int found = 0;

  auto int NESTED_FUNC_ATTR find_card (grub_pci_device_t dev, grub_pci_id_t pciid __attribute__ ((unused)));
  int NESTED_FUNC_ATTR find_card (grub_pci_device_t dev, grub_pci_id_t pciid __attribute__ ((unused)))
    {
      grub_pci_address_t addr;
      grub_uint32_t class;

      addr = grub_pci_make_address (dev, 2);
      class = grub_pci_read (addr);

      if (((class >> 16) & 0xffff) != 0x0300 || pciid != 0x0712126f)
	return 0;
      
      found = 1;

      addr = grub_pci_make_address (dev, 4);
      framebuffer.base = grub_pci_read (addr);
      framebuffer.dev = dev;

      return 1;
    }

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> GRUB_VIDEO_MODE_TYPE_DEPTH_POS;

  if ((width != 1024 && width != 0) || (height != 600 && height != 0)
      || (depth != 16 && depth != 0))
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "Only 1024x600x16 is supported");

  grub_pci_iterate (find_card);
  if (!found)
    return grub_error (GRUB_ERR_IO, "Couldn't find graphics card");

  if (found && framebuffer.base == 0)
    {
      /* FIXME: change framebuffer base */
    }

  /* Fill mode info details.  */
  framebuffer.mode_info.width = 1024;
  framebuffer.mode_info.height = 600;
  framebuffer.mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_RGB;
  framebuffer.mode_info.bpp = 16;
  framebuffer.mode_info.bytes_per_pixel = 2;
  framebuffer.mode_info.pitch = 1024 * 2;
  framebuffer.mode_info.number_of_colors = 256;
  framebuffer.mode_info.red_mask_size = 5;
  framebuffer.mode_info.red_field_pos = 11;
  framebuffer.mode_info.green_mask_size = 6;
  framebuffer.mode_info.green_field_pos = 5;
  framebuffer.mode_info.blue_mask_size = 5;
  framebuffer.mode_info.blue_field_pos = 0;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;
  framebuffer.mode_info.blit_format = grub_video_get_blit_format (&framebuffer.mode_info);
  /* We can safely discard volatile attribute.  */
  framebuffer.ptr = (void *) grub_pci_device_map_range (framebuffer.dev,
							framebuffer.base,
							1024 * 600 * 2);
  framebuffer.mapped = 1;

  err = grub_video_fb_create_render_target_from_pointer (&framebuffer.render_target, &framebuffer.mode_info, framebuffer.ptr);

  if (err)
    return err;

  err = grub_video_fb_set_active_render_target (framebuffer.render_target);
  
  if (err)
    return err;

  /* Copy default palette to initialize emulated palette.  */
  err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				   grub_video_fbstd_colors);
  return err;
}

static grub_err_t
grub_video_sm712_set_palette (unsigned int start, unsigned int count,
                            struct grub_video_palette_data *palette_data)
{
  if (framebuffer.index_color_mode)
    {
      /* TODO: Implement setting indexed color mode palette to hardware.  */
    }

  /* Then set color to emulated palette.  */
  return grub_video_fb_set_palette (start, count, palette_data);
}

static grub_err_t
grub_video_sm712_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_sm712_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_DISPLAY)
      target = framebuffer.render_target;

  return grub_video_fb_set_active_render_target (target);
}

static grub_err_t
grub_video_sm712_get_info_and_fini (struct grub_video_mode_info *mode_info,
				    void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}


static struct grub_video_adapter grub_video_sm712_adapter =
  {
    .name = "SM712 Video Driver",

    .init = grub_video_sm712_video_init,
    .fini = grub_video_sm712_video_fini,
    .setup = grub_video_sm712_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_sm712_get_info_and_fini,
    .set_palette = grub_video_sm712_set_palette,
    .get_palette = grub_video_fb_get_palette,
    .set_viewport = grub_video_fb_set_viewport,
    .get_viewport = grub_video_fb_get_viewport,
    .map_color = grub_video_fb_map_color,
    .map_rgb = grub_video_fb_map_rgb,
    .map_rgba = grub_video_fb_map_rgba,
    .unmap_color = grub_video_fb_unmap_color,
    .fill_rect = grub_video_fb_fill_rect,
    .blit_bitmap = grub_video_fb_blit_bitmap,
    .blit_render_target = grub_video_fb_blit_render_target,
    .scroll = grub_video_fb_scroll,
    .swap_buffers = grub_video_sm712_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_sm712_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

GRUB_MOD_INIT(video_sm712)
{
  grub_video_register (&grub_video_sm712_adapter);
}

GRUB_MOD_FINI(video_sm712)
{
  grub_video_unregister (&grub_video_sm712_adapter);
}
