/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

/* All tests need to include test.h for GRUB testing framework.  */
#include <grub/test.h>
#include <grub/dl.h>
#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/command.h>
#include <grub/font.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct
{
  struct grub_video_mode_info mode_info;
} tests[] = {
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 640,
	.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR,
	.bpp = 8,
	.bytes_per_pixel = 1,
	.number_of_colors = GRUB_VIDEO_FBSTD_NUMCOLORS
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 800,
	.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR,
	.bpp = 8,
	.bytes_per_pixel = 1,
	.number_of_colors = GRUB_VIDEO_FBSTD_NUMCOLORS
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 1024,
	.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR,
	.bpp = 8,
	.bytes_per_pixel = 1,
	.number_of_colors = GRUB_VIDEO_FBSTD_NUMCOLORS
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 1280,
	GRUB_VIDEO_MI_RGB555 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 1600,
	GRUB_VIDEO_MI_RGB555 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 2048,
	GRUB_VIDEO_MI_RGB555 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 1280,
	GRUB_VIDEO_MI_RGB565 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 1600,
	GRUB_VIDEO_MI_RGB565 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 2048,
	GRUB_VIDEO_MI_RGB565 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 640 * 3,
	GRUB_VIDEO_MI_RGB888 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 800 * 3,
	GRUB_VIDEO_MI_RGB888 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 1024 * 3,
	GRUB_VIDEO_MI_RGB888 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 640 * 4,
	GRUB_VIDEO_MI_RGBA8888()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 800 * 4,
	GRUB_VIDEO_MI_RGBA8888()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 1024 * 4,
	GRUB_VIDEO_MI_RGBA8888()
      },
    },

    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 1280,
	GRUB_VIDEO_MI_BGR555 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 1600,
	GRUB_VIDEO_MI_BGR555 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 2048,
	GRUB_VIDEO_MI_BGR555 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 1280,
	GRUB_VIDEO_MI_BGR565 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 1600,
	GRUB_VIDEO_MI_BGR565 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 2048,
	GRUB_VIDEO_MI_BGR565 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 640 * 3,
	GRUB_VIDEO_MI_BGR888 ()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 800 * 3,
	GRUB_VIDEO_MI_BGR888 ()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 1024 * 3,
	GRUB_VIDEO_MI_BGR888 ()
      },
    },
    {
      .mode_info = {
	.width = 640,
	.height = 480,
	.pitch = 640 * 4,
	GRUB_VIDEO_MI_BGRA8888()
      },
    },
    {
      .mode_info = {
	.width = 800,
	.height = 600,
	.pitch = 800 * 4,
	GRUB_VIDEO_MI_BGRA8888()
      },
    },
    {
      .mode_info = {
	.width = 1024,
	.height = 768,
	.pitch = 1024 * 4,
	GRUB_VIDEO_MI_BGRA8888()
      },
    },

  };
      

/* Functional test main method.  */
static void
videotest_checksum (void)
{
  unsigned i;
  grub_font_load ("unicode");
  for (i = 0; i < ARRAY_SIZE (tests); i++)
    {
      grub_video_capture_start (&tests[i].mode_info,
				grub_video_fbstd_colors,
				GRUB_VIDEO_FBSTD_NUMCOLORS);
      grub_terminal_input_fake_sequence ((int []) { '\n' }, 1);

      grub_video_checksum ("videotest");

      char *args[] = { 0 };
      grub_command_execute ("videotest", 0, args);
      grub_video_checksum_end ();
      grub_video_capture_end ();
    }
}

/* Register example_test method as a functional test.  */
GRUB_FUNCTIONAL_TEST (videotest_checksum, videotest_checksum);
