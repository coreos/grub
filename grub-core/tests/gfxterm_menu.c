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
#include <grub/procfs.h>
#include <grub/env.h>
#include <grub/normal.h>

GRUB_MOD_LICENSE ("GPLv3+");


static const char testfile[] =
  "terminal_output gfxterm\n"
  "menuentry \"test\" {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"s̛ ơ t o̒ s̒ u o̕̚ 8.04 m̂ñåh̊z̆x̣ a̡ b̢g̢ u᷎ô᷎ ô᷎ O̷ a̖̣ ȃ̐\" --class ubuntu --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \" הַרמלל(טוֹבָ) לֶךְ\" --class opensuse --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"الرملل جِداً لِكَ\" --class gentoo --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"ὑπόγυͅον\" --class kubuntu --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"سَّ نِّ نَّ نٌّ نّْ\" --class linuxmint --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  /* Chinese & UTF-8 test from Carbon Jiao. */
  "menuentry \"从硬盘的第一主分区启动\" --class \"windows xp\" --class windows --class os {\n"
  "\ttrue\n"
  "}\n"
  "timeout=3\n";

static char *
get_test_cfg (void)
{
  return grub_strdup (testfile);
}

struct grub_procfs_entry test_cfg = 
{
  .name = "test.cfg",
  .get_contents = get_test_cfg
};


/* Functional test main method.  */
static void
gfxterm_menu (void)
{
  unsigned i, j;
  if (grub_font_load ("unicode") == 0)
    {
      grub_test_assert (0, "unicode font not found: %s", grub_errmsg);
      return;
    }

  grub_procfs_register ("test.cfg", &test_cfg);
  
  for (j = 0; j < 2; j++)
    for (i = 0; i < ARRAY_SIZE (grub_test_video_modes); i++)
      {
	grub_video_capture_start (&grub_test_video_modes[i],
				  grub_video_fbstd_colors,
				  grub_test_video_modes[i].number_of_colors);
	grub_terminal_input_fake_sequence ((int []) { -1, -1, -1, GRUB_TERM_KEY_DOWN, -1, '\e' }, 6);

	grub_video_checksum (j ? "gfxmenu" : "gfxterm_menu");

	grub_env_context_open ();
	if (j)
	  grub_env_set ("theme", "starfield/theme.txt");
	grub_normal_execute ("(proc)/test.cfg", 1, 0);
	grub_env_context_close ();

	char *args[] = { (char *) "console", 0 };
	grub_command_execute ("terminal_output", 1, args);

	grub_terminal_input_fake_sequence_end ();
	grub_video_checksum_end ();
	grub_video_capture_end ();
      }

  grub_procfs_unregister (&test_cfg);
}

/* Register example_test method as a functional test.  */
GRUB_FUNCTIONAL_TEST (gfxterm_menu, gfxterm_menu);
