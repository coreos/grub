/* help.c - command to show a help text.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/normal.h>
#include <grub/charset.h>

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
	  struct grub_term_output *term;
	  const char *summary_translated = _(cmd->summary);
	  char *command_help;
	  grub_uint32_t *unicode_command_help;
	  grub_uint32_t *unicode_last_position;

	  command_help = grub_malloc (grub_strlen (cmd->name) +
	  			      sizeof (" ") - 1 +
				      grub_strlen (summary_translated));
	  			      
	  grub_sprintf(command_help, "%s %s", cmd->name, summary_translated);

	  grub_utf8_to_ucs4_alloc (command_help, &unicode_command_help,
	  			   &unicode_last_position);

	  FOR_ACTIVE_TERM_OUTPUTS(term)
	  {
	    unsigned stringwidth;
	    grub_uint32_t *unicode_last_screen_position;

	    unicode_last_screen_position = unicode_command_help;

	    stringwidth = 0;

	    while (unicode_last_screen_position < unicode_last_position && 
		   stringwidth < ((grub_term_width (term) / 2) - 2))
	      {
		stringwidth
		  += grub_term_getcharwidth (term,
					     *unicode_last_screen_position);
		unicode_last_screen_position++;
	      }

	    grub_print_ucs4 (unicode_command_help,
			     unicode_last_screen_position, term);
	    if (!(cnt % 2))
	      grub_print_spaces (term, grub_term_width (term) / 2
				 - stringwidth);
	  }
	  if (cnt % 2)
	    grub_printf ("\n");
	  cnt++;
	  
	  grub_free (command_help);
	  grub_free (unicode_command_help);
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
		grub_printf ("%s %s %s\n%s\b", _("Usage:"), cmd->name, _(cmd->summary),
			     _(cmd->description));
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
			      N_("[PATTERN ...]"),
			      N_("Show a help message."), 0);
}

GRUB_MOD_FINI(help)
{
  grub_unregister_extcmd (cmd);
}
