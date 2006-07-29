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

#include <grub/err.h>
#include <grub/machine/memory.h>
#include <grub/machine/vbe.h>
#include <grub/machine/vbefill.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/font.h>
#include <grub/mm.h>
#include <grub/video.h>

void
grub_video_i386_vbefill_R8G8B8A8 (struct grub_video_render_target *dst,
                                  grub_video_color_t color, int x, int y,
                                  int width, int height)
{
  int i;
  int j;
  grub_uint32_t *dstptr;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      dstptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        *dstptr++ = color;
    }
}

void
grub_video_i386_vbefill_R8G8B8 (struct grub_video_render_target *dst,
                                grub_video_color_t color, int x, int y,
                                int width, int height)
{
  int i;
  int j;
  grub_uint8_t *dstptr;
  grub_uint8_t fillr = (grub_uint8_t)((color >> 0) & 0xFF);
  grub_uint8_t fillg = (grub_uint8_t)((color >> 8) & 0xFF);
  grub_uint8_t fillb = (grub_uint8_t)((color >> 16) & 0xFF);

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          *dstptr++ = fillr;
          *dstptr++ = fillg;
          *dstptr++ = fillb;
        }
    }
}

void
grub_video_i386_vbefill_index (struct grub_video_render_target *dst,
                               grub_video_color_t color, int x, int y,
                               int width, int height)
{
  int i;
  int j;
  grub_uint8_t *dstptr;
  grub_uint8_t fill = (grub_uint8_t)color & 0xFF;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        *dstptr++ = fill;
    }
}
