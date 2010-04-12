/* execute.c -- Execute a GRUB script.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/script_sh.h>
#include <grub/command.h>
#include <grub/menu.h>
#include <grub/lib/arg.h>
#include <grub/normal.h>

/* Max digits for a char is 3 (0xFF is 255), similarly for an int it
   is sizeof (int) * 3, and one extra for a possible -ve sign.  */
#define ERRNO_DIGITS_MAX  (sizeof (int) * 3 + 1)

static grub_err_t
grub_script_execute_cmd (struct grub_script_cmd *cmd)
{
  int ret;
  char errnobuf[ERRNO_DIGITS_MAX + 1];

  if (cmd == 0)
    return 0;

  ret = cmd->exec (cmd);

  grub_snprintf (errnobuf, sizeof (errnobuf), "%d", ret);
  grub_env_set ("?", errnobuf);
  return ret;
}

#define ARG_ALLOCATION_UNIT  (32 * sizeof (char))
#define ARGV_ALLOCATION_UNIT (8 * sizeof (void*))

/* Expand arguments in ARGLIST into multiple arguments.  */
char **
grub_script_execute_arglist_to_argv (struct grub_script_arglist *arglist, int *count)
{
  int i;
  int oom;
  int argc;
  int empty;
  char *ptr;
  char **argv;
  char *value;
  struct grub_script_arg *arg;

  auto void push (char *str);
  void push (char *str)
  {
    char **p;

    if (oom)
      return;

    p = grub_realloc (argv, ALIGN_UP (sizeof(char*) * (argc + 1), ARGV_ALLOCATION_UNIT));
    if (!p)
      oom = 1;
    else
      {
	p[argc++] = str;
	argv = p;
      }
  }

  auto char* append (const char *str, grub_size_t nchar);
  char* append (const char *str, grub_size_t nchar)
  {
    int len;
    int old;
    char *p;

    if (oom || !str)
      return 0;

    len = nchar ?: grub_strlen (str);
    old = argv[argc - 1] ? grub_strlen (argv[argc - 1]) : 0;
    p = grub_realloc (argv[argc - 1], ALIGN_UP(old + len + 1, ARG_ALLOCATION_UNIT));

    if (p)
      {
	grub_strncpy (p + old, str, len);
	p[old + len] = '\0';
      }
    else
      {
	oom = 1;
	grub_free (argv[argc - 1]);
      }
    argv[argc - 1] = p;
    return argv[argc - 1];
  }

  /* Move *STR to the begining of next word, but return current word.  */
  auto char* move_to_next (char **str);
  char* move_to_next (char **str)
  {
    char *end;
    char *start;

    if (oom || !str || !*str)
      return 0;

    start = *str;
    while (*start && grub_isspace (*start)) start++;
    if (*start == '\0')
      return 0;

    end = start + 1;
    while (*end && !grub_isspace (*end)) end++;

    *str = end;
    return start;
  }

  oom = 0;
  argv = 0;
  argc = 0;
  push (0);
  for (; arglist; arglist = arglist->next)
    {
      empty = 1;
      arg = arglist->arg;
      while (arg)
	{
	  switch (arg->type)
	    {
	    case GRUB_SCRIPT_ARG_TYPE_VAR:
	      value = grub_env_get (arg->str);
	      while (value && *value && (ptr = move_to_next(&value)))
		{
		  empty = 0;
		  append (ptr, value - ptr);
		  if (*value) push(0);
		}
	      break;

	    case GRUB_SCRIPT_ARG_TYPE_TEXT:
	      if (grub_strlen (arg->str) > 0)
		{
		  empty = 0;
		  append (arg->str, 0);
		}
	      break;

	    case GRUB_SCRIPT_ARG_TYPE_DQSTR:
	    case GRUB_SCRIPT_ARG_TYPE_SQSTR:
	      empty = 0;
	      append (arg->str, 0);
	      break;

	    case GRUB_SCRIPT_ARG_TYPE_DQVAR:
	      empty = 0;
	      append (grub_env_get (arg->str), 0);
	      break;
	    }
	  arg = arg->next;
	}
      if (!empty)
	push (0);
    }

  if (oom)
    {
      for (i = 0; i < argc; i++)
	grub_free (argv[i]);
      grub_free (argv);
      argv = 0;
    }

  if (argv)
    *count = argc - 1;

  return argv;
}

/* Execute a single command line.  */
grub_err_t
grub_script_execute_cmdline (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdline *cmdline = (struct grub_script_cmdline *) cmd;
  char **args = 0;
  int i = 0;
  grub_command_t grubcmd;
  grub_err_t ret = 0;
  int argcount = 0;
  grub_script_function_t func = 0;
  char errnobuf[18];
  char *cmdname;

  /* Lookup the command.  */
  args = grub_script_execute_arglist_to_argv (cmdline->arglist, &argcount);
  if (!args)
    return grub_errno;

  cmdname = args[0];
  grubcmd = grub_command_find (cmdname);
  if (! grubcmd)
    {
      grub_errno = GRUB_ERR_NONE;

      /* It's not a GRUB command, try all functions.  */
      func = grub_script_function_find (cmdname);
      if (! func)
	{
	  /* As a last resort, try if it is an assignment.  */
	  char *assign = grub_strdup (cmdname);
	  char *eq = grub_strchr (assign, '=');

	  if (eq)
	    {
	      /* This was set because the command was not found.  */
	      grub_errno = GRUB_ERR_NONE;

	      /* Create two strings and set the variable.  */
	      *eq = '\0';
	      eq++;
	      grub_env_set (assign, eq);
	    }
	  grub_free (assign);

	  grub_snprintf (errnobuf, sizeof (errnobuf), "%d", grub_errno);
	  grub_env_set ("?", errnobuf);

	  grub_print_error ();

	  return 0;
	}
    }

  /* Execute the GRUB command or function.  */
  if (grubcmd)
    ret = (grubcmd->func) (grubcmd, argcount - 1, args + 1);
  else
    ret = grub_script_function_call (func, argcount - 1, args + 1);

  /* Free arguments.  */
  for (i = 0; i < argcount; i++)
    grub_free (args[i]);
  grub_free (args);

  if (grub_errno == GRUB_ERR_TEST_FAILURE)
    grub_errno = GRUB_ERR_NONE;

  grub_print_error ();

  grub_snprintf (errnobuf, sizeof (errnobuf), "%d", ret);
  grub_env_set ("?", errnobuf);

  return ret;
}

/* Execute a block of one or more commands.  */
grub_err_t
grub_script_execute_cmdblock (struct grub_script_cmd *cmd)
{
  int ret = 0;
  struct grub_script_cmdblock *cmdblock = (struct grub_script_cmdblock *) cmd;

  /* Loop over every command and execute it.  */
  for (cmd = cmdblock->cmdlist; cmd; cmd = cmd->next)
    ret = grub_script_execute_cmd (cmd);

  return ret;
}

/* Execute an if statement.  */
grub_err_t
grub_script_execute_cmdif (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdif *cmdif = (struct grub_script_cmdif *) cmd;
  char *result;

  /* Check if the commands results in a true or a false.  The value is
     read from the env variable `?'.  */
  grub_script_execute_cmd (cmdif->exec_to_evaluate);
  result = grub_env_get ("?");

  grub_errno = GRUB_ERR_NONE;

  /* Execute the `if' or the `else' part depending on the value of
     `?'.  */
  if (result && ! grub_strcmp (result, "0"))
    return grub_script_execute_cmd (cmdif->exec_on_true);
  else
    return grub_script_execute_cmd (cmdif->exec_on_false);
}

/* Execute a for statement.  */
grub_err_t
grub_script_execute_cmdfor (struct grub_script_cmd *cmd)
{
  int i;
  int result;
  char **args;
  int argcount;
  struct grub_script_cmdfor *cmdfor = (struct grub_script_cmdfor *) cmd;

  args = grub_script_execute_arglist_to_argv (cmdfor->words, &argcount);
  if (!args)
    return grub_errno;

  result = 0;
  for (i = 0; i < argcount; i++)
    {
      grub_env_set (cmdfor->name->str, args[i]);
      result = grub_script_execute_cmd (cmdfor->list);
      grub_free (args[i]);
    }

  grub_free (args);
  return result;
}

/* Execute a "while" or "until" command.  */
grub_err_t
grub_script_execute_cmdwhile (struct grub_script_cmd *cmd)
{
  int cond;
  int result;
  struct grub_script_cmdwhile *cmdwhile = (struct grub_script_cmdwhile *) cmd;

  result = 0;
  do {
    cond = grub_script_execute_cmd (cmdwhile->cond);
    if (cmdwhile->until ? !cond : cond)
      break;

    result = grub_script_execute_cmd (cmdwhile->list);
  } while (1); /* XXX Put a check for ^C here */

  return result;
}

/* Execute the menu entry generate statement.  */
grub_err_t
grub_script_execute_menuentry (struct grub_script_cmd *cmd)
{
  struct grub_script_cmd_menuentry *cmd_menuentry;
  char **args = 0;
  int argcount = 0;
  int i = 0;

  cmd_menuentry = (struct grub_script_cmd_menuentry *) cmd;

  if (cmd_menuentry->arglist)
    {
      args = grub_script_execute_arglist_to_argv (cmd_menuentry->arglist, &argcount);
      if (!args)
	return grub_errno;
    }

  grub_normal_add_menu_entry (argcount, (const char **) args,
			      cmd_menuentry->sourcecode);

  /* Free arguments.  */
  for (i = 0; i < argcount; i++)
    grub_free (args[i]);
  grub_free (args);

  return grub_errno;
}



/* Execute any GRUB pre-parsed command or script.  */
grub_err_t
grub_script_execute (struct grub_script *script)
{
  if (script == 0)
    return 0;

  return grub_script_execute_cmd (script->cmd);
}

