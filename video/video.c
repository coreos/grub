/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006  Free Software Foundation, Inc.
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

#include <grub/video.h>
#include <grub/types.h>
#include <grub/dl.h>

/* The list of video adapters registerd to system.  */
static grub_video_adapter_t grub_video_adapter_list;

/* Active video adapter.  */
static grub_video_adapter_t grub_video_adapter_active;

void
grub_video_register (grub_video_adapter_t adapter)
{
  adapter->next = grub_video_adapter_list;
  grub_video_adapter_list = adapter;
}

void
grub_video_unregister (grub_video_adapter_t adapter)
{
  grub_video_adapter_t *p, q;
  
  for (p = &grub_video_adapter_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == adapter)
      {
        *p = q->next;
        break;
      }                                    
}

void
grub_video_iterate (int (*hook) (grub_video_adapter_t adapter))
{
  grub_video_adapter_t p;
  
  for (p = grub_video_adapter_list; p; p = p->next)
    if (hook (p))
      break;
}

grub_err_t
grub_video_setup (unsigned int width, unsigned int height,
                  unsigned int mode_type)
{
  grub_video_adapter_t p;

  /* De-activate last set video adapter.  */
  if (grub_video_adapter_active)
    {
      /* Finalize adapter.  */
      grub_video_adapter_active->fini ();
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;
        
      /* Mark active adapter as not set.  */
      grub_video_adapter_active = 0;
    }
  
  /* Loop thru all possible video adapter trying to find requested mode.  */
  for (p = grub_video_adapter_list; p; p = p->next)
    {
      /* Try to initialize adapter, if can't skip to next.  */
      p->init ();
      if (grub_errno != GRUB_ERR_NONE)
        {
          grub_errno = GRUB_ERR_NONE;
          continue;
        }

      /* Try to initialize video mode.  */      
      p->setup (width, height, mode_type);
      if (grub_errno == GRUB_ERR_NONE)
        {
          /* Valid mode found from adapter, and it has been activated.
             Specify it as active adapter.  */
          grub_video_adapter_active = p;
          return GRUB_ERR_NONE;
        }
      else
        grub_errno = GRUB_ERR_NONE;        

      /* No valid mode found in this adapter, finalize adapter.  */
      p->fini ();
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;
    }
  
  /* We couldn't find suitable adapter for specified mode.  */
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, 
                     "Can't locate valid adapter for mode");
}

grub_err_t
grub_video_restore (void)
{
  if (grub_video_adapter_active)
    {
      grub_video_adapter_active->fini ();
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;
      
      grub_video_adapter_active = 0;
    }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_get_info (struct grub_video_mode_info *mode_info)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");
  
  return grub_video_adapter_active->get_info (mode_info);
}

grub_err_t
grub_video_set_palette (unsigned int start, unsigned int count,
                        struct grub_video_palette_data *palette_data)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");
  
  return grub_video_adapter_active->set_palette (start, count, palette_data);
}

grub_err_t
grub_video_get_palette (unsigned int start, unsigned int count,
                        struct grub_video_palette_data *palette_data)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");
  
  return grub_video_adapter_active->get_palette (start, count, palette_data);
}

grub_err_t
grub_video_set_viewport (unsigned int x, unsigned int y,
                         unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->set_viewport (x, y, width, height);
}

grub_err_t
grub_video_get_viewport (unsigned int *x, unsigned int *y,
                         unsigned int *width, unsigned int *height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->get_viewport (x, y, width, height);
}

grub_video_color_t
grub_video_map_color (grub_uint32_t color_name)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_color (color_name);
}

grub_video_color_t
grub_video_map_rgb (grub_uint8_t red, grub_uint8_t green, grub_uint8_t blue)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_rgb (red, green, blue);
}

grub_video_color_t
grub_video_map_rgba (grub_uint8_t red, grub_uint8_t green, grub_uint8_t blue,
                     grub_uint8_t alpha)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_rgba (red, green, blue, alpha);
}

grub_err_t
grub_video_fill_rect (grub_video_color_t color, int x, int y,
                      unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->fill_rect (color, x, y, width, height);
}

grub_err_t
grub_video_blit_glyph (struct grub_font_glyph *glyph,
                       grub_video_color_t color, int x, int y)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->blit_glyph (glyph, color, x, y);
}

grub_err_t
grub_video_blit_bitmap (struct grub_video_bitmap *bitmap,
                        int x, int y, int offset_x, int offset_y,
                        unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->blit_bitmap (bitmap, x, y,
                                                 offset_x, offset_y,
                                                 width, height);
}

grub_err_t
grub_video_blit_render_target (struct grub_video_render_target *target,
                               int x, int y, int offset_x, int offset_y,
                               unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->blit_render_target (target, x, y,
                                                        offset_x, offset_y, 
                                                        width, height);
}

grub_err_t
grub_video_scroll (grub_video_color_t color, int dx, int dy)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->scroll (color, dx, dy);
}

grub_err_t
grub_video_swap_buffers (void)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->swap_buffers ();
}

grub_err_t
grub_video_create_render_target (struct grub_video_render_target **result,
                                 unsigned int width, unsigned int height,
                                 unsigned int mode_type)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->create_render_target (result,
                                                          width, height,
                                                          mode_type);
}

grub_err_t
grub_video_delete_render_target (struct grub_video_render_target *target)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->delete_render_target (target);
}

grub_err_t
grub_video_set_active_render_target (struct grub_video_render_target *target)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->set_active_render_target (target);
}

GRUB_MOD_INIT(video_video)
{
  grub_video_adapter_active = 0;
  grub_video_adapter_list = 0;
}

GRUB_MOD_FINI(video_video)
{
}
