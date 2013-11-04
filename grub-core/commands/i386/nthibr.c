/* nthibr.c - tests whether an MS Windows system partition is hibernated */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Peter Lustig
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

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/err.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_err_t
grub_cmd_nthibr (grub_command_t cmd __attribute__ ((unused)),
                 int argc, char **args)
{
  grub_uint8_t hibr_file_magic[4];
  grub_file_t hibr_file = 0;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  hibr_file = grub_file_open (args[0]);
  if (!hibr_file)
    return grub_errno;

  /* Try to read magic number of 'hiberfil.sys' */
  if (grub_file_read (hibr_file, hibr_file_magic,
		      sizeof (hibr_file_magic))
      != (grub_ssize_t) sizeof (hibr_file_magic))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_TEST_FAILURE, "false");
      goto exit;
    }

  if (!(grub_memcmp ("hibr", hibr_file_magic, sizeof (hibr_file_magic)) == 0
	|| grub_memcmp ("HIBR", hibr_file_magic, sizeof (hibr_file_magic)) == 0))
    grub_error (GRUB_ERR_TEST_FAILURE, "false");

 exit:
  grub_file_close (hibr_file);

  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT (check_nt_hiberfil)
{
  cmd = grub_register_command ("check_nt_hiberfil", grub_cmd_nthibr,
                               N_("FILE"),
                               N_("Test whether a hiberfil.sys is "
                                  "in hibernated state."));
}

GRUB_MOD_FINI (check_nt_hiberfil)
{
  grub_unregister_command (cmd);
}
