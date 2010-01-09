/* grub-script-check.c - check grub script file for syntax errors */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/parser.h>
#include <grub/script_sh.h>

#include <grub_script_check_init.h>

#define _GNU_SOURCE	1

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "progname.h"

void
grub_putchar (int c)
{
  putchar (c);
}

int
grub_getkey (void)
{
  return -1;
}

void
grub_refresh (void)
{
  fflush (stdout);
}

struct grub_handler_class grub_term_input_class;
struct grub_handler_class grub_term_output_class;

char *
grub_script_execute_argument_to_string (struct grub_script_arg *arg __attribute__ ((unused)))
{
  return 0;
}

grub_err_t
grub_script_execute_cmdline (struct grub_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

grub_err_t
grub_script_execute_cmdblock (struct grub_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

grub_err_t
grub_script_execute_cmdif (struct grub_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

grub_err_t
grub_script_execute_menuentry (struct grub_script_cmd *cmd)
{
  struct grub_script_cmd_menuentry *menu;
  menu = (struct grub_script_cmd_menuentry *)cmd;

  if (menu->sourcecode)
    {
      grub_free (menu->sourcecode);
      menu->sourcecode = 0;
    }
  return 0;
}

grub_err_t
grub_script_execute (struct grub_script *script)
{
  if (script == 0 || script->cmd == 0)
    return 0;

  return script->cmd->exec (script->cmd);
}

static struct option options[] =
  {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr,
	     "Try ``%s --help'' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [PATH]\n\
\n\
Checks GRUB script configuration file for syntax errors.\n\
\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print script being processed\n\
\n\
Report bugs to <%s>.\n\
", program_name,
	    PACKAGE_BUGREPORT);
  exit (status);
}

int
main (int argc, char *argv[])
{
  char *argument;
  char *input;
  FILE *file = 0;
  int verbose = 0;
  struct grub_script *script;

  auto grub_err_t get_config_line (char **line, int cont);
  grub_err_t get_config_line (char **line, int cont __attribute__ ((unused)))
  {
    char *cmdline = 0;
    size_t len = 0;
    ssize_t read;

    read = getline(&cmdline, &len, (file ?: stdin));
    if (read == -1)
      {
	*line = 0;
	grub_errno = GRUB_ERR_READ_ERROR;

	if (cmdline)
	  free (cmdline);
	return grub_errno;
      }

    if (verbose)
      grub_printf("%s", cmdline);

    *line = grub_strdup (cmdline);

    free (cmdline);
    return 0;
  }

  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "hvV", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbose = 1;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain ARGUMENT.  */
  if (optind >= argc)
    {
      file = 0; // read from stdin
    }
  else if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }
  else
    {
      argument = argv[optind];
      file = fopen (argument, "r");
      if (! file)
	{
	  fprintf (stderr, "%s: %s: %s\n", program_name, argument, strerror(errno));
	  usage (1);
	}
    }

  /* Initialize all modules.  */
  grub_init_all ();

  do
    {
      input = 0;
      get_config_line(&input, 0);
      if (! input) 
	break;

      script = grub_script_parse (input, get_config_line);
      if (script)
	{
	  grub_script_execute (script);
	  grub_script_free (script);
	}

      grub_free (input);
    } while (script != 0);

  /* Free resources.  */
  grub_fini_all ();
  if (file) fclose (file);

  return (script == 0);
}
