/* configfile.c - command to manually load config file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2009  Free Software Foundation, Inc.
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
#include <grub/term.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/command.h>
#include <grub/i18n.h>

static grub_err_t
grub_cmd_source (grub_command_t cmd, int argc, char **args)
{
  int new_env, jail;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  jail = (cmd->name[0] == 'j');
  new_env = (cmd->name[jail ? 5 : 0] == 'c');

  if (new_env)
    grub_cls ();

  if (new_env && !jail)
    grub_env_context_open ();
  if (jail)
    grub_env_jail_open (!new_env);

  grub_normal_execute (args[0], 1, ! new_env);

  if (new_env && !jail)
    grub_env_context_close ();
  if (jail)
    grub_env_jail_close (!new_env);

  return 0;
}

static grub_command_t cmd_configfile, cmd_source, cmd_dot;
static grub_command_t cmd_jail_source, cmd_jail_configfile;

GRUB_MOD_INIT(configfile)
{
  cmd_configfile =
    grub_register_command ("configfile", grub_cmd_source,
			   N_("FILE"), N_("Load another config file."));
  cmd_source =
    grub_register_command ("source", grub_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context.")
			   );

  cmd_jail_source =
    grub_register_command ("jail_source", grub_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context but take only menuentries.")
			   );

  cmd_jail_configfile =
    grub_register_command ("jail_configfile", grub_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context but take only menuentries.")
			   );

  cmd_dot =
    grub_register_command (".", grub_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context.")
			   );
}

GRUB_MOD_FINI(configfile)
{
  grub_unregister_command (cmd_configfile);
  grub_unregister_command (cmd_source);
  grub_unregister_command (cmd_jail_configfile);
  grub_unregister_command (cmd_jail_source);
  grub_unregister_command (cmd_dot);
}
