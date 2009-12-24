/* help.c - command to show a help text.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/extcmd.h>

static grub_err_t
grub_cmd_help (grub_extcmd_t ext __attribute__ ((unused)), int argc,
	       char **args)
{
  int cnt = 0;
  char *currarg;

  auto int print_command_info (grub_command_t cmd);
  auto int print_command_help (grub_command_t cmd);

  int print_command_info (grub_command_t cmd)
    {
      if ((cmd->prio & GRUB_PRIO_LIST_FLAG_ACTIVE) &&
	  (cmd->flags & GRUB_COMMAND_FLAG_CMDLINE))
	{
	  struct grub_term_output *cur;
	  int desclen = grub_strlen (cmd->summary);
	  FOR_ACTIVE_TERM_OUTPUTS(cur)
	  {
	    int width = grub_term_width(cur);
	    char description[width / 2];

	    /* Make a string with a length of GRUB_TERM_WIDTH / 2 - 1 filled
	       with the description followed by spaces.  */
	    grub_memset (description, ' ', width / 2 - 1);
	    description[width / 2 - 1] = '\0';
	    grub_memcpy (description, cmd->summary,
			 (desclen < width / 2 - 1
			  ? desclen : width / 2 - 1));
	    grub_puts_terminal (description, cur);
	  }
	  if ((cnt++) % 2)
	    grub_printf ("\n");
	  else
	    grub_printf (" ");
	}
      return 0;
    }

  int print_command_help (grub_command_t cmd)
    {
      if (cmd->prio & GRUB_PRIO_LIST_FLAG_ACTIVE)
	{
	  if (! grub_strncmp (cmd->name, currarg, grub_strlen (currarg)))
	    {
	      if (cnt++ > 0)
		grub_printf ("\n\n");

	      if (cmd->flags & GRUB_COMMAND_FLAG_EXTCMD)
		grub_arg_show_help ((grub_extcmd_t) cmd->data);
	      else
		grub_printf ("Usage: %s\n%s\b", cmd->summary,
			     cmd->description);
	    }
	}
      return 0;
    }

  if (argc == 0)
    {
      grub_command_iterate (print_command_info);
      if (!(cnt % 2))
	grub_printf ("\n");
    }
  else
    {
      int i;

      for (i = 0; i < argc; i++)
	{
	  currarg = args[i];
	  grub_command_iterate (print_command_help);
	}
    }

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(help)
{
  cmd = grub_register_extcmd ("help", grub_cmd_help,
			      GRUB_COMMAND_FLAG_CMDLINE,
			      "help [PATTERN ...]",
			      "Show a help message.", 0);
}

GRUB_MOD_FINI(help)
{
  grub_unregister_extcmd (cmd);
}
