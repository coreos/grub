/* hello.c - test module for dynamic loading */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007  Free Software Foundation, Inc.
 *  Copyright (C) 2003  NIIBE Yutaka <gniibe@m17n.org>
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
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

static struct grub_script *script;

static grub_err_t
grub_cmd_hello (grub_extcmd_context_t ctxt,
		int argc, char **args __attribute__ ((unused)))
{
  if (argc == 0 && script == 0)
    grub_printf ("Hello World\n");

  else if (argc == 0)
    grub_script_execute (script);

  else
    {
      if (! ctxt->script_params || ! ctxt->script_params[0])
	return 1;

      if (script)
	grub_script_free (script);

      script = grub_malloc (sizeof (*script));
      if (! script)
	return 1;

      script->cmd = ctxt->script_params[0]->cmd;
      script->mem = ctxt->script_params[0]->mem;
      ctxt->script_params[0]->cmd = 0;
      ctxt->script_params[0]->mem = 0;
    }

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(hello)
{
  cmd = grub_register_extcmd ("hello", grub_cmd_hello,
			      GRUB_COMMAND_FLAG_BOTH | GRUB_COMMAND_FLAG_BLOCKS,
			      N_("[BLOCK]"), N_("Say \"Hello World\"."), 0);
}

GRUB_MOD_FINI(hello)
{
  if (script)
    grub_script_free (script);

  grub_unregister_extcmd (cmd);
}
