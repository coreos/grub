/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/command.h>
#include <grub/misc.h>
#include <grub/cmos.h>

static grub_err_t
grub_cmd_cmostest (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  int byte, bit;
  char *rest;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Address required.");

  byte = grub_strtoul (argv[0], &rest, 0);
  if (*rest != ':')
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Address required.");

  bit = grub_strtoul (rest + 1, 0, 0);

  if (grub_cmos_read (byte) & (1 << bit))
    return GRUB_ERR_NONE;

  return grub_error (GRUB_ERR_TEST_FAILURE, "false");
}

static grub_command_t cmd;


GRUB_MOD_INIT(cmostest)
{
  cmd = grub_register_command ("cmostest", grub_cmd_cmostest,
			       "cmostest BYTE:BIT",
			       "Test bit at BYTE:BIT in CMOS.");
}

GRUB_MOD_FINI(cmostest)
{
  grub_unregister_command (cmd);
}
