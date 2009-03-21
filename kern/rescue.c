/* rescue.c - rescue mode */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/rescue.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/partition.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/list.h>
#include <grub/command.h>

#define GRUB_RESCUE_BUF_SIZE	256
#define GRUB_RESCUE_MAX_ARGS	20

static char linebuf[GRUB_RESCUE_BUF_SIZE];

/* Prompt to input a command and read the line.  */
static void
grub_rescue_get_command_line (const char *prompt)
{
  int c;
  int pos = 0;
  
  grub_printf (prompt);
  grub_memset (linebuf, 0, GRUB_RESCUE_BUF_SIZE);
  
  while ((c = GRUB_TERM_ASCII_CHAR (grub_getkey ())) != '\n' && c != '\r')
    {
      if (grub_isprint (c))
	{
	  if (pos < GRUB_RESCUE_BUF_SIZE - 1)
	    {
	      linebuf[pos++] = c;
	      grub_putchar (c);
	    }
	}
      else if (c == '\b')
	{
	  if (pos > 0)
	    {
	      linebuf[--pos] = 0;
	      grub_putchar (c);
	      grub_putchar (' ');
	      grub_putchar (c);
	    }
	}
      grub_refresh ();
    }

  grub_putchar ('\n');
  grub_refresh ();
}

static void
attempt_normal_mode (void)
{
  grub_command_t cmd;

  cmd = grub_command_find ("normal");
  if (cmd)
    (cmd->func) (cmd, 0, 0);
}

/* Enter the rescue mode.  */
void
grub_enter_rescue_mode (void)
{
  auto grub_err_t getline (char **line);
  
  grub_err_t getline (char **line)
    {
      grub_rescue_get_command_line ("> ");
      *line = linebuf;
      return 0;
    }

  grub_register_core_commands ();

  /* First of all, attempt to execute the normal mode.  */
  attempt_normal_mode ();

  grub_printf ("Entering rescue mode...\n");
  
  while (1)
    {
      char *line = linebuf;
      char *name;
      int n;
      grub_command_t cmd;
      char **args;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      /* Get a command line.  */
      grub_rescue_get_command_line ("grub rescue> ");
      if (line[0] == 0)
	continue;

      if (grub_parser_split_cmdline (line, getline, &n, &args) || n < 0)
	continue;

      /* In case of an assignment set the environment accordingly
	 instead of calling a function.  */
      if (n == 0 && grub_strchr (line, '='))
	{
	  char *val = grub_strchr (args[0], '=');
	  val[0] = 0;
	  grub_env_set (args[0], val + 1);
	  val[0] = '=';
          grub_free (args[0]);
	  continue;
	}

      /* Get the command name.  */
      name = args[0];

      /* If nothing is specified, restart.  */
      if (*name == '\0')
	{
	  grub_free (args[0]);
	  continue;
	}
      
      cmd = grub_command_find (name);
      if (cmd)
	{
	  (cmd->func) (cmd, n, &args[1]);
	}
      else
	{
	  grub_printf ("Unknown command `%s'\n", name);
	  grub_printf ("Try `help' for usage\n");
	}

      grub_free (args[0]);
    }
}
