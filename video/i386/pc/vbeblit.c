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
#include <grub/machine/vbeblit.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/font.h>
#include <grub/mm.h>
#include <grub/video.h>

void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8A8 (struct grub_video_render_target *dst,
                                           struct grub_video_render_target *src,
                                           int x, int y, int width, int height,
                                           int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint32_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned int dr;
  unsigned int dg;
  unsigned int db;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                              j + offset_y);

      dstptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr++;
              continue;
            }

          if (a == 255)
            {
              *dstptr++ = color;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          color = *dstptr;

          dr = (color >> 0) & 0xFF;
          dg = (color >> 8) & 0xFF;
          db = (color >> 16) & 0xFF;

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          color = (a << 24) | (db << 16) | (dg << 8) | dr;

          *dstptr++ = color;
        }
    }
}

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8A8 (struct grub_video_render_target *dst,
                                         struct grub_video_render_target *src,
                                         int x, int y, int width, int height,
                                         int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned int dr;
  unsigned int dg;
  unsigned int db;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                              j + offset_y);

      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr += 3;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              *dstptr++ = sr;
              *dstptr++ = sg;
              *dstptr++ = sb;

              continue;
            }

          dr = dstptr[0];
          dg = dstptr[1];
          db = dstptr[2];

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          *dstptr++ = dr;
          *dstptr++ = dg;
          *dstptr++ = db;
        }
    }
}

void
grub_video_i386_vbeblit_index_R8G8B8A8 (struct grub_video_render_target *dst,
                                        struct grub_video_render_target *src,
                                        int x, int y, int width, int height,
                                        int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned char dr;
  unsigned char dg;
  unsigned char db;
  unsigned char da;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                              j + offset_y);

      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr++;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              color = grub_video_vbe_map_rgb(sr, sg, sb);
              *dstptr++ = color & 0xFF;
              continue;
            }

          grub_video_vbe_unmap_color (dst, *dstptr, &dr, &dg, &db, &da);

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          color = grub_video_vbe_map_rgb(dr, dg, db);

          *dstptr++ = color & 0xFF;
        }
    }
}

void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8 (struct grub_video_render_target *dst,
                                         struct grub_video_render_target *src,
                                         int x, int y, int width, int height,
                                         int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint32_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                             j + offset_y);

      dstptr = (grub_uint32_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          sr = *srcptr++;
          sg = *srcptr++;
          sb = *srcptr++;

          color = 0xFF000000 | (sb << 16) | (sg << 8) | sr;

          *dstptr++ = color;
        }
    }
}

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8 (struct grub_video_render_target *dst,
                                       struct grub_video_render_target *src,
                                       int x, int y, int width, int height,
                                       int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (src, 
                                                             offset_x,
                                                             j + offset_y);

      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst,
                                                             x, 
                                                             y + j);

      for (i = 0; i < width; i++)
        {
          *dstptr ++ = *srcptr++;
          *dstptr ++ = *srcptr++;
          *dstptr ++ = *srcptr++;
        }
    }
}

void
grub_video_i386_vbeblit_index_R8G8B8 (struct grub_video_render_target *dst,
                                      struct grub_video_render_target *src,
                                      int x, int y, int width, int height,
                                      int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                             j + offset_y);

      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          sr = *srcptr++;
          sg = *srcptr++;
          sb = *srcptr++;

          color = grub_video_vbe_map_rgb(sr, sg, sb);

          *dstptr++ = color & 0xFF;
        }
    }
}

void
grub_video_i386_vbeblit_index_index (struct grub_video_render_target *dst,
                                     struct grub_video_render_target *src,
                                     int x, int y, int width, int height,
                                     int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (src, offset_x,
                                                             j + offset_y);

      dstptr = (grub_uint8_t *)grub_video_vbe_get_video_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
          *dstptr++ = *srcptr++;
    }
}
