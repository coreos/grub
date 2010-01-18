/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/test.h>

static grub_err_t
grub_functional_test (struct grub_extcmd *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char **args __attribute__ ((unused)))
{
  auto int run_test (grub_test_t test);
  int run_test (grub_test_t test)
  {
    grub_test_run (test);
    return 0;
  }

  grub_list_iterate (GRUB_AS_LIST (grub_test_list),
		     (grub_list_hook_t) run_test);
  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (functional_test)
{
  cmd = grub_register_extcmd ("functional_test", grub_functional_test,
			      GRUB_COMMAND_FLAG_CMDLINE, 0,
			      "Run all functional tests.", 0);
}

GRUB_MOD_FINI (functional_test)
{
  grub_unregister_extcmd (cmd);
}
