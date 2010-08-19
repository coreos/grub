/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/dl.h>
#include <grub/keyboard_layouts.h>
#include <grub/command.h>
#include <grub/gzio.h>
#include <grub/i18n.h>

static grub_err_t
grub_cmd_keymap (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  char *filename;
  grub_file_t file;
  grub_uint32_t version;
  grub_uint8_t magic[GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE];
  struct grub_keyboard_layout *newmap = NULL;
  unsigned i;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file or layout name required");
  if (argv[0][0] != '(' && argv[0][0] != '/' && argv[0][0] != '+')
    {
      const char *prefix = grub_env_get ("prefix");
      if (!prefix)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "No prefix set");	
      filename = grub_xasprintf ("%s/layouts/%s.gkb", prefix, argv[0]);
      if (!filename)
	return grub_errno;
    }
  else
    filename = argv[0];

  file = grub_gzfile_open (filename, 1);
  if (! file)
    goto fail;

  if (grub_file_read (file, magic, sizeof (magic)) != sizeof (magic))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  if (grub_memcmp (magic, GRUB_KEYBOARD_LAYOUTS_FILEMAGIC,
		   GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE) != 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid magic");
      goto fail;
    }

  if (grub_file_read (file, &version, sizeof (version)) != sizeof (version))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  if (grub_le_to_cpu32 (version) != GRUB_KEYBOARD_LAYOUTS_VERSION)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid version");
      goto fail;
    }

  newmap = grub_malloc (sizeof (*newmap));
  if (!newmap)
    goto fail;

  if (grub_file_read (file, newmap, sizeof (*newmap)) != sizeof (*newmap))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  for (i = 0; i < ARRAY_SIZE (newmap->at.keyboard_map); i++)
    newmap->at.keyboard_map[i] = grub_le_to_cpu32(newmap->at.keyboard_map[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->at.keyboard_map_shift); i++)
    newmap->at.keyboard_map_shift[i]
      = grub_le_to_cpu32(newmap->at.keyboard_map_shift[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->at.keyboard_map_l3); i++)
    newmap->at.keyboard_map_l3[i]
      = grub_le_to_cpu32(newmap->at.keyboard_map_l3[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->at.keyboard_map_shift_l3); i++)
    newmap->at.keyboard_map_shift_l3[i]
      = grub_le_to_cpu32(newmap->at.keyboard_map_shift_l3[i]);

  return GRUB_ERR_NONE;

 fail:
  if (filename != argv[0])
    grub_free (filename);
  grub_free (newmap);
  if (file)
    grub_file_close (file);
  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(keylayouts)
{
  cmd = grub_register_command ("keymap", grub_cmd_keymap,
			       0, N_("Load a keyboard layout."));
}

GRUB_MOD_FINI(keylayouts)
{
  grub_current_layout = NULL;
  grub_unregister_command (cmd);
}
