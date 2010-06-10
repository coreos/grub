/* menuentry.c - menuentry command */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/normal.h>

static grub_err_t
grub_cmd_menuentry (grub_extcmd_context_t ctxt, int argc, char **args)
{
  char *src;
  grub_err_t r;

  /* XXX Rewrite to make use of already parsed menu definition.  */
  if (! argc || ! ctxt->script_params || ! ctxt->script_params[argc - 1])
    return GRUB_ERR_BAD_ARGUMENT;

  src = args[argc - 1];
  args[argc - 1] = '\0';

  r = grub_normal_add_menu_entry (argc - 1, (const char **) args, src);

  args[argc - 1] = src;
  return r;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(menuentry)
{
  cmd = grub_register_extcmd ("menuentry", grub_cmd_menuentry,
			      GRUB_COMMAND_FLAG_BOTH | GRUB_COMMAND_FLAG_BLOCKS,
			      N_("BLOCK"), N_("Define a menuentry."), 0);
}

GRUB_MOD_FINI(menuentry)
{
  grub_unregister_extcmd (cmd);
}
