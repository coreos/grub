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

#ifndef GRUB_VBEBLIT_MACHINE_HEADER
#define GRUB_VBEBLIT_MACHINE_HEADER	1

void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8A8 (struct grub_video_render_target *dst,
                                           struct grub_video_render_target *src,
                                           int x, int y, int width, int height,
                                           int offset_x, int offset_y);

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8A8 (struct grub_video_render_target *dst,
                                         struct grub_video_render_target *src,
                                         int x, int y, int width, int height,
                                         int offset_x, int offset_y);

void
grub_video_i386_vbeblit_index_R8G8B8A8 (struct grub_video_render_target *dst,
                                        struct grub_video_render_target *src,
                                        int x, int y, int width, int height,
                                        int offset_x, int offset_y);


void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8 (struct grub_video_render_target *dst,
                                         struct grub_video_render_target *src,
                                         int x, int y, int width, int height,
                                         int offset_x, int offset_y);

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8 (struct grub_video_render_target *dst,
                                       struct grub_video_render_target *src,
                                       int x, int y, int width, int height,
                                       int offset_x, int offset_y);

void
grub_video_i386_vbeblit_index_R8G8B8 (struct grub_video_render_target *dst,
                                      struct grub_video_render_target *src,
                                      int x, int y, int width, int height,
                                      int offset_x, int offset_y);

void
grub_video_i386_vbeblit_index_index (struct grub_video_render_target *dst,
                                     struct grub_video_render_target *src,
                                     int x, int y, int width, int height,
                                     int offset_x, int offset_y);

#endif /* ! GRUB_VBEBLIT_MACHINE_HEADER */
