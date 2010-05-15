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

  grub_uint8_t *ptr;
  int mapped;
  grub_uint32_t base;
  grub_pci_device_t dev;
} framebuffer;

#define CIRRUS_APERTURE_SIZE 0x200000

#define GR_INDEX 0x3ce
#define GR_DATA 0x3cf
#define CR_INDEX 0x3d4
#define CR_DATA 0x3d5
#define SR_INDEX 0x3c4
#define SR_DATA 0x3c5

#define CIRRUS_MAX_WIDTH 0x800
#define CIRRUS_WIDTH_DIVISOR 8
#define CIRRUS_MAX_HEIGHT 0x800
#define CIRRUS_MAX_PITCH (0x1ff * CIRRUS_WIDTH_DIVISOR)

enum
  {
    CIRRUS_GR_MODE = 5,
    CIRRUS_GR_GR6 = 6,
    CIRRUS_GR_MAX
  };

#define CIRRUS_GR_GR6_GRAPHICS_MODE 1

#define CIRRUS_GR_MODE_256_COLOR 0x40
#define CIRRUS_GR_MODE_READ_MODE1 0x08

enum
  {
    CIRRUS_CR_WIDTH = 0x01,
    CIRRUS_CR_OVERFLOW = 0x07,
    CIRRUS_CR_CELL_HEIGHT = 0x09,
    CIRRUS_CR_VSYNC_END = 0x11,
    CIRRUS_CR_HEIGHT = 0x12,
    CIRRUS_CR_PITCH = 0x13,
    CIRRUS_CR_MODE = 0x17,
    CIRRUS_CR_LINE_COMPARE = 0x18,
    CIRRUS_CR_EXTENDED_DISPLAY = 0x1b,
    CIRRUS_CR_MAX
  };

#define CIRRUS_CR_CELL_HEIGHT_LINE_COMPARE_MASK 0x40
#define CIRRUS_CR_CELL_HEIGHT_LINE_COMPARE_SHIFT 3

#define CIRRUS_CR_OVERFLOW_HEIGHT1_SHIFT 7
#define CIRRUS_CR_OVERFLOW_HEIGHT1_MASK 0x02
#define CIRRUS_CR_OVERFLOW_HEIGHT2_SHIFT 3
#define CIRRUS_CR_OVERFLOW_HEIGHT2_MASK 0xc0
#define CIRRUS_CR_OVERFLOW_LINE_COMPARE_MASK 0x10
#define CIRRUS_CR_OVERFLOW_LINE_COMPARE_SHIFT 4

#define CIRRUS_CR_EXTENDED_DISPLAY_PITCH_MASK 0x10
#define CIRRUS_CR_EXTENDED_DISPLAY_PITCH_SHIFT 4

#define CIRRUS_CR_MODE_TIMING_ENABLE 0x80
#define CIRRUS_CR_MODE_BYTE_MODE 0x40
#define CIRRUS_CR_MODE_NO_HERCULES 0x02
#define CIRRUS_CR_MODE_NO_CGA 0x01

enum
  {
    CIRRUS_SR_MEMORY_MODE = 4,
    CIRRUS_SR_EXTENDED_MODE = 7,
    CIRRUS_SR_MAX
  };
#define CIRRUS_SR_MEMORY_MODE_CHAIN4 8
#define CIRRUS_SR_MEMORY_MODE_NORMAL 0
#define CIRRUS_SR_EXTENDED_MODE_LFB_ENABLE 0xf0
#define CIRRUS_SR_EXTENDED_MODE_ENABLE_EXT 0x01
#define CIRRUS_SR_EXTENDED_MODE_24BPP      0x04
#define CIRRUS_SR_EXTENDED_MODE_16BPP      0x06
#define CIRRUS_SR_EXTENDED_MODE_32BPP      0x08

static void
gr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, GR_INDEX);
  grub_outb (val, GR_DATA);
}

static grub_uint8_t
gr_read (grub_uint8_t addr)
{
  grub_outb (addr, GR_INDEX);
  return grub_inb (GR_DATA);
}

static void
cr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, CR_INDEX);
  grub_outb (val, CR_DATA);
}

static grub_uint8_t
cr_read (grub_uint8_t addr)
{
  grub_outb (addr, CR_INDEX);
  return grub_inb (CR_DATA);
}

static void
sr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, SR_INDEX);
  grub_outb (val, SR_DATA);
}

static grub_uint8_t
sr_read (grub_uint8_t addr)
{
  grub_outb (addr, SR_INDEX);
  return grub_inb (SR_DATA);
}

static void
write_hidden_dac (grub_uint8_t data)
{
  grub_inb (0x3c8);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  grub_outb (data, 0x3c6);
}

static grub_uint8_t
read_hidden_dac (void)
{
  grub_inb (0x3c8);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  grub_inb (0x3c6);
  return grub_inb (0x3c6);
}

struct saved_state
{
  grub_uint8_t cr[CIRRUS_CR_MAX];
  grub_uint8_t gr[CIRRUS_GR_MAX];
  grub_uint8_t sr[CIRRUS_SR_MAX];
  grub_uint8_t hidden_dac;
  /* We need to preserve VGA font and VGA text. */
  grub_uint8_t vram[32 * 4 * 256];
};

static struct saved_state initial_state;
static int state_saved = 0;

static void
save_state (struct saved_state *st)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (st->cr); i++)
    st->cr[i] = cr_read (i);
  for (i = 0; i < ARRAY_SIZE (st->sr); i++)
    st->sr[i] = sr_read (i);
  for (i = 0; i < ARRAY_SIZE (st->gr); i++)
    st->gr[i] = gr_read (i);
  st->hidden_dac = read_hidden_dac ();
  sr_write (CIRRUS_SR_MEMORY_MODE_CHAIN4, CIRRUS_SR_MEMORY_MODE);
  grub_memcpy (st->vram, framebuffer.ptr, sizeof (st->vram));
}

static void
restore_state (struct saved_state *st)
{
  unsigned i;
  sr_write (CIRRUS_SR_MEMORY_MODE_CHAIN4, CIRRUS_SR_MEMORY_MODE);
  grub_memcpy (framebuffer.ptr, st->vram, sizeof (st->vram));
  for (i = 0; i < ARRAY_SIZE (st->cr); i++)
    cr_write (st->cr[i], i);
  for (i = 0; i < ARRAY_SIZE (st->sr); i++)
    sr_write (st->sr[i], i);
  for (i = 0; i < ARRAY_SIZE (st->gr); i++)
    gr_write (st->gr[i], i);
  write_hidden_dac (st->hidden_dac);
}

static grub_err_t
grub_video_cirrus_video_init (void)
{
  /* Reset frame buffer.  */
  grub_memset (&framebuffer, 0, sizeof(framebuffer));

  return grub_video_fb_init ();
}

static grub_err_t
grub_video_cirrus_video_fini (void)
{
  if (framebuffer.mapped)
    grub_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				 CIRRUS_APERTURE_SIZE);

  if (state_saved)
    {
      restore_state (&initial_state);
      state_saved = 0;
    }

  return grub_video_fb_fini ();
}

static grub_err_t
grub_video_cirrus_setup (unsigned int width, unsigned int height,
			unsigned int mode_type, unsigned int mode_mask __attribute__ ((unused)))
{
  int depth;
  grub_err_t err;
  int found = 0;
  int pitch, bytes_per_pixel;

  auto int NESTED_FUNC_ATTR find_card (grub_pci_device_t dev, grub_pci_id_t pciid __attribute__ ((unused)));
  int NESTED_FUNC_ATTR find_card (grub_pci_device_t dev, grub_pci_id_t pciid)
    {
      grub_pci_address_t addr;
      grub_uint32_t class;

      addr = grub_pci_make_address (dev, GRUB_PCI_REG_CLASS);
      class = grub_pci_read (addr);

      if (((class >> 16) & 0xffff) != 0x0300 || pciid != 0x00b81013)
	return 0;
      
      found = 1;

      addr = grub_pci_make_address (dev, GRUB_PCI_REG_ADDRESS_REG0);
      framebuffer.base = grub_pci_read (addr) & GRUB_PCI_ADDR_MEM_MASK;
      framebuffer.dev = dev;

      return 1;
    }

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> GRUB_VIDEO_MODE_TYPE_DEPTH_POS;

  if (width == 0 || height == 0)
    {
      width = 800;
      height = 600;
    }

  if (width & (CIRRUS_WIDTH_DIVISOR - 1))
    return grub_error (GRUB_ERR_IO,
		       "screen width must be a multiple of %d",
		       CIRRUS_WIDTH_DIVISOR);

  if (width > CIRRUS_MAX_WIDTH)
    return grub_error (GRUB_ERR_IO,
		       "screen width must be at most %d", CIRRUS_MAX_WIDTH);

  if (height > CIRRUS_MAX_HEIGHT)
    return grub_error (GRUB_ERR_IO,
		       "screen height must be at most %d", CIRRUS_MAX_HEIGHT);

  if (depth == 0)
    depth = 24;

  if (depth != 32 && depth != 24 && depth != 16 && depth != 15)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "only 32, 24, 16 and 15-bit bpp are supported");

  bytes_per_pixel = (depth + 7) / 8;
  pitch = width * bytes_per_pixel;

  if (pitch > CIRRUS_MAX_PITCH)
    return grub_error (GRUB_ERR_IO,
		       "screen width must be at most %d at bitdepth %d",
		       CIRRUS_MAX_PITCH / bytes_per_pixel, depth);

  grub_pci_iterate (find_card);
  if (!found)
    return grub_error (GRUB_ERR_IO, "Couldn't find graphics card");

  if (found && framebuffer.base == 0)
    {
      /* FIXME: change framebuffer base */
    }

  /* We can safely discard volatile attribute.  */
  framebuffer.ptr = (void *) grub_pci_device_map_range (framebuffer.dev,
							framebuffer.base,
							CIRRUS_APERTURE_SIZE);
  framebuffer.mapped = 1;

  if (!state_saved)
    {
      save_state (&initial_state);
      state_saved = 1;
    }

  {
    int pitch_reg, overflow_reg = 0, line_compare = 0x3ff;
    grub_uint8_t sr_ext = 0;

    pitch_reg = pitch / CIRRUS_WIDTH_DIVISOR;

    gr_write (CIRRUS_GR_MODE_256_COLOR | CIRRUS_GR_MODE_READ_MODE1,
	      CIRRUS_GR_MODE);
    gr_write (CIRRUS_GR_GR6_GRAPHICS_MODE, CIRRUS_GR_GR6);
    
    sr_write (CIRRUS_SR_MEMORY_MODE_NORMAL, CIRRUS_SR_MEMORY_MODE);

    /* Disable CR0-7 write protection.  */
    cr_write (0, CIRRUS_CR_VSYNC_END);

    cr_write (width / CIRRUS_WIDTH_DIVISOR - 1, CIRRUS_CR_WIDTH);
    cr_write ((height - 1) & 0xff, CIRRUS_CR_HEIGHT);
    overflow_reg |= (((height - 1) >> CIRRUS_CR_OVERFLOW_HEIGHT1_SHIFT) & 
		     CIRRUS_CR_OVERFLOW_HEIGHT1_MASK)
      | (((height - 1) >> CIRRUS_CR_OVERFLOW_HEIGHT2_SHIFT) & 
	 CIRRUS_CR_OVERFLOW_HEIGHT2_MASK);

    cr_write (pitch_reg & 0xff, CIRRUS_CR_PITCH);

    cr_write (line_compare & 0xff, CIRRUS_CR_LINE_COMPARE);
    overflow_reg |= (line_compare >> CIRRUS_CR_OVERFLOW_LINE_COMPARE_SHIFT)
      & CIRRUS_CR_OVERFLOW_LINE_COMPARE_MASK;

    cr_write (overflow_reg, CIRRUS_CR_OVERFLOW);

    cr_write ((pitch_reg >> CIRRUS_CR_EXTENDED_DISPLAY_PITCH_SHIFT)
	      & CIRRUS_CR_EXTENDED_DISPLAY_PITCH_MASK,
	      CIRRUS_CR_EXTENDED_DISPLAY);

    cr_write ((line_compare >> CIRRUS_CR_CELL_HEIGHT_LINE_COMPARE_SHIFT)
	      & CIRRUS_CR_CELL_HEIGHT_LINE_COMPARE_MASK, CIRRUS_CR_CELL_HEIGHT);

    cr_write (CIRRUS_CR_MODE_TIMING_ENABLE | CIRRUS_CR_MODE_BYTE_MODE
	      | CIRRUS_CR_MODE_NO_HERCULES | CIRRUS_CR_MODE_NO_CGA,
	      CIRRUS_CR_MODE);

    sr_ext = CIRRUS_SR_EXTENDED_MODE_LFB_ENABLE
      | CIRRUS_SR_EXTENDED_MODE_ENABLE_EXT;
    switch (depth)
      {
      case 32:
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_32BPP;
	break;
      case 24:
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_24BPP;
	break;
      case 16:
      case 15:
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_16BPP;
	break;
      }
    sr_write (sr_ext, CIRRUS_SR_EXTENDED_MODE);
    write_hidden_dac (depth == 16);
  }

  /* Fill mode info details.  */
  framebuffer.mode_info.width = width;
  framebuffer.mode_info.height = height;
  framebuffer.mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_RGB;
  framebuffer.mode_info.bpp = depth;
  framebuffer.mode_info.bytes_per_pixel = bytes_per_pixel;
  framebuffer.mode_info.pitch = pitch;
  framebuffer.mode_info.number_of_colors = 256;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;

  switch (depth)
    {
    case 16:
      framebuffer.mode_info.red_mask_size = 5;
      framebuffer.mode_info.red_field_pos = 11;
      framebuffer.mode_info.green_mask_size = 6;
      framebuffer.mode_info.green_field_pos = 5;
      framebuffer.mode_info.blue_mask_size = 5;
      framebuffer.mode_info.blue_field_pos = 0;
      break;

    case 15:
      framebuffer.mode_info.red_mask_size = 5;
      framebuffer.mode_info.red_field_pos = 10;
      framebuffer.mode_info.green_mask_size = 5;
      framebuffer.mode_info.green_field_pos = 5;
      framebuffer.mode_info.blue_mask_size = 5;
      framebuffer.mode_info.blue_field_pos = 0;
      break;

    case 32:
      framebuffer.mode_info.reserved_mask_size = 8;
      framebuffer.mode_info.reserved_field_pos = 24;

    case 24:
      framebuffer.mode_info.red_mask_size = 8;
      framebuffer.mode_info.red_field_pos = 16;
      framebuffer.mode_info.green_mask_size = 8;
      framebuffer.mode_info.green_field_pos = 8;
      framebuffer.mode_info.blue_mask_size = 8;
      framebuffer.mode_info.blue_field_pos = 0;
      break;
    }

  framebuffer.mode_info.blit_format = grub_video_get_blit_format (&framebuffer.mode_info);

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
grub_video_cirrus_set_palette (unsigned int start, unsigned int count,
			       struct grub_video_palette_data *palette_data)
{
  /*  if (framebuffer.index_color_mode) */
    {
      /* TODO: Implement setting indexed color mode palette to hardware.  */
    }

  /* Then set color to emulated palette.  */
  return grub_video_fb_set_palette (start, count, palette_data);
}

static grub_err_t
grub_video_cirrus_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_cirrus_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_DISPLAY)
      target = framebuffer.render_target;

  return grub_video_fb_set_active_render_target (target);
}

static grub_err_t
grub_video_cirrus_get_info_and_fini (struct grub_video_mode_info *mode_info,
				    void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}


static struct grub_video_adapter grub_video_cirrus_adapter =
  {
    .name = "Cirrus CLGD 5446 PCI Video Driver",
    .id = GRUB_VIDEO_DRIVER_CIRRUS,

    .init = grub_video_cirrus_video_init,
    .fini = grub_video_cirrus_video_fini,
    .setup = grub_video_cirrus_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_cirrus_get_info_and_fini,
    .set_palette = grub_video_cirrus_set_palette,
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
    .swap_buffers = grub_video_cirrus_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_cirrus_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

GRUB_MOD_INIT(video_cirrus)
{
  grub_video_register (&grub_video_cirrus_adapter);
}

GRUB_MOD_FINI(video_cirrus)
{
  grub_video_unregister (&grub_video_cirrus_adapter);
}
