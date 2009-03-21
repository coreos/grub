/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/dl.h>
#include <grub/parser.h>
#include <grub/script.h>
#include <grub/list.h>
#include <grub/command.h>

int
grub_command_execute (char *cmdline, int interactive)
{
  auto grub_err_t cmdline_get (char **s);
  grub_err_t cmdline_get (char **s)
    {
      *s = grub_malloc (GRUB_MAX_CMDLINE);
      *s[0] = '\0';
      return grub_cmdline_get (">", *s, GRUB_MAX_CMDLINE, 0, 1);
    }

  grub_err_t ret = 0;
  char *pager;
  struct grub_script *parsed_script;
  
  /* Enable the pager if the environment pager is set to 1.  */
  if (interactive)
    pager = grub_env_get ("pager");
  else
    pager = NULL;
  if (pager && (! grub_strcmp (pager, "1")))
    grub_set_more (1);

  /* Parse the script.  */
  parsed_script = grub_script_parse (cmdline, cmdline_get);

  if (parsed_script)
    {
      /* Execute the command(s).  */
      grub_script_execute (parsed_script);

      /* The parsed script was executed, throw it away.  */
      grub_script_free (parsed_script);
    }

  if (pager && (! grub_strcmp (pager, "1")))
    grub_set_more (0);

  return ret;
}