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

/* Scope for grub script functions.  */
struct grub_script_scope
{
  struct grub_script_argv argv;
};
static struct grub_script_scope *scope = 0;

/* Wildcard translator for GRUB script.  */
struct grub_script_wildcard_translator *wildcard_translator;

static int
grub_env_special (const char *name)
{
  if (grub_isdigit (name[0]) ||
      grub_strcmp (name, "#") == 0 ||
      grub_strcmp (name, "*") == 0 ||
      grub_strcmp (name, "@") == 0)
    return 1;
  return 0;
}

static char **
grub_script_env_get (const char *name, grub_script_arg_type_t type)
{
  struct grub_script_argv result = { 0, 0 };

  if (grub_script_argv_next (&result))
    goto fail;

  if (! grub_env_special (name))
    {
      char *v = grub_env_get (name);
      if (v && v[0])
	{
	  if (type == GRUB_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (grub_script_argv_split_append (&result, v))
		goto fail;
	    }
	  else
	    if (grub_script_argv_append (&result, v))
	      goto fail;
	}
    }
  else if (! scope)
    {
      if (grub_script_argv_append (&result, 0))
	goto fail;
    }
  else if (grub_strcmp (name, "#") == 0)
    {
      char buffer[ERRNO_DIGITS_MAX + 1];
      grub_snprintf (buffer, sizeof (buffer), "%u", scope->argv.argc);
      if (grub_script_argv_append (&result, buffer))
	goto fail;
    }
  else if (grub_strcmp (name, "*") == 0)
    {
      unsigned i;

      for (i = 0; i < scope->argv.argc; i++)
	if (type == GRUB_SCRIPT_ARG_TYPE_VAR)
	  {
	    if (i != 0 && grub_script_argv_next (&result))
	      goto fail;

	    if (grub_script_argv_split_append (&result, scope->argv.args[i]))
	      goto fail;
	  }
	else
	  {
	    if (i != 0 && grub_script_argv_append (&result, " "))
	      goto fail;

	    if (grub_script_argv_append (&result, scope->argv.args[i]))
	      goto fail;
	  }
    }
  else if (grub_strcmp (name, "@") == 0)
    {
      unsigned i;

      for (i = 0; i < scope->argv.argc; i++)
	{
	  if (i != 0 && grub_script_argv_next (&result))
	    goto fail;

	  if (type == GRUB_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (grub_script_argv_split_append (&result, scope->argv.args[i]))
		goto fail;
	    }
	  else
	    if (grub_script_argv_append (&result, scope->argv.args[i]))
	      goto fail;
	}
    }
  else
    {
      unsigned long num = grub_strtoul (name, 0, 10);
      if (num == 0)
	; /* XXX no file name, for now.  */

      else if (num <= scope->argv.argc)
	{
	  if (type == GRUB_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (grub_script_argv_split_append (&result,
						 scope->argv.args[num - 1]))
		goto fail;
	    }
	  else
	    if (grub_script_argv_append (&result, scope->argv.args[num - 1]))
	      goto fail;
	}
    }

  return result.args;

 fail:

  grub_script_argv_free (&result);
  return 0;
}

static grub_err_t
grub_script_env_set (const char *name, const char *val)
{
  if (grub_env_special (name))
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad variable name");

  return grub_env_set (name, val);
}

/* Convert arguments in ARGLIST into ARGV form.  */
static int
grub_script_arglist_to_argv (struct grub_script_arglist *arglist,
			     struct grub_script_argv *argv)
{
  int i;
  char **values = 0;
  struct grub_script_arg *arg = 0;
  struct grub_script_argv result = { 0, 0 };

  auto int append (char *s, int escape_type);
  int append (char *s, int escape_type)
  {
    int r;
    char *p = 0;

    if (! wildcard_translator || escape_type == 0)
      return grub_script_argv_append (&result, s);

    if (escape_type > 0)
      p = wildcard_translator->escape (s);
    else if (escape_type < 0)
      p = wildcard_translator->unescape (s);

    if (! p)
      return 1;

    r = grub_script_argv_append (&result, p);
    grub_free (p);
    return r;
  }

  for (; arglist && arglist->arg; arglist = arglist->next)
    {
      if (grub_script_argv_next (&result))
	goto fail;

      arg = arglist->arg;
      while (arg)
	{
	  switch (arg->type)
	    {
	    case GRUB_SCRIPT_ARG_TYPE_VAR:
	    case GRUB_SCRIPT_ARG_TYPE_DQVAR:
	      values = grub_script_env_get (arg->str, arg->type);
	      for (i = 0; values && values[i]; i++)
		{
		  if (i != 0 && grub_script_argv_next (&result))
		    goto fail;

		  if (arg->type == GRUB_SCRIPT_ARG_TYPE_VAR)
		    {
		      if (grub_script_argv_append (&result, values[i]))
			goto fail;
		    }
		  else
		    {
		      if (append (values[i], 1))
			goto fail;
		    }

		  grub_free (values[i]);
		}
	      grub_free (values);
	      break;

	    case GRUB_SCRIPT_ARG_TYPE_TEXT:
	      if (grub_strlen (arg->str) &&
		  grub_script_argv_append (&result, arg->str))
		goto fail;
	      break;

	    case GRUB_SCRIPT_ARG_TYPE_DQSTR:
	    case GRUB_SCRIPT_ARG_TYPE_SQSTR:
	      if (grub_script_argv_append (&result, arg->str))
		goto fail;
	      break;
	    }
	  arg = arg->next;
	}
    }

  if (! result.args[result.argc - 1])
    result.argc--;

  /* Perform wildcard expansion.  */

  if (wildcard_translator)
    {
      int j;
      int failed = 0;
      char **expansions = 0;
      struct grub_script_argv unexpanded = result;

      result.argc = 0;
      result.args = 0;
      for (i = 0; unexpanded.args[i]; i++)
	{
	  if (wildcard_translator->expand (unexpanded.args[i], &expansions))
	    {
	      grub_script_argv_free (&unexpanded);
	      goto fail;
	    }

	  if (! expansions)
	    {
	      grub_script_argv_next (&result);
	      append (unexpanded.args[i], -1);
	    }
	  else
	    {
	      for (j = 0; expansions[j]; j++)
		{
		  failed = (failed || grub_script_argv_next (&result) ||
			    append (expansions[j], -1));
		  grub_free (expansions[j]);
		}
	      grub_free (expansions);

	      if (failed)
		{
		  grub_script_argv_free (&unexpanded);
		  goto fail;
		}
	    }
	}
      grub_script_argv_free (&unexpanded);
    }

  *argv = result;
  return 0;

 fail:

  grub_script_argv_free (&result);
  return 1;
}

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

/* Execute a function call.  */
grub_err_t
grub_script_function_call (grub_script_function_t func, int argc, char **args)
{
  grub_err_t ret = 0;
  struct grub_script_scope *old_scope;
  struct grub_script_scope new_scope;

  new_scope.argv.argc = argc;
  new_scope.argv.args = args;

  old_scope = scope;
  scope = &new_scope;

  ret = grub_script_execute (func->func);

  scope = old_scope;
  return ret;
}

/* Execute a single command line.  */
grub_err_t
grub_script_execute_cmdline (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdline *cmdline = (struct grub_script_cmdline *) cmd;
  grub_command_t grubcmd;
  grub_err_t ret = 0;
  grub_script_function_t func = 0;
  char errnobuf[18];
  char *cmdname;
  struct grub_script_argv argv = { 0, 0 };

  /* Lookup the command.  */
  if (grub_script_arglist_to_argv (cmdline->arglist, &argv))
    return grub_errno;

  cmdname = argv.args[0];
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
	      grub_script_env_set (assign, eq);
	    }
	  grub_free (assign);

	  grub_snprintf (errnobuf, sizeof (errnobuf), "%d", grub_errno);
	  grub_env_set ("?", errnobuf);

	  grub_script_argv_free (&argv);
	  grub_print_error ();

	  return 0;
	}
    }

  /* Execute the GRUB command or function.  */
  if (grubcmd)
    ret = (grubcmd->func) (grubcmd, argv.argc - 1, argv.args + 1);
  else
    ret = grub_script_function_call (func, argv.argc - 1, argv.args + 1);

  /* Free arguments.  */
  grub_script_argv_free (&argv);

  if (grub_errno == GRUB_ERR_TEST_FAILURE)
    grub_errno = GRUB_ERR_NONE;

  grub_print_error ();

  grub_snprintf (errnobuf, sizeof (errnobuf), "%d", ret);
  grub_env_set ("?", errnobuf);

  return ret;
}

/* Execute a block of one or more commands.  */
grub_err_t
grub_script_execute_cmdlist (struct grub_script_cmd *list)
{
  int ret = 0;
  struct grub_script_cmd *cmd;

  /* Loop over every command and execute it.  */
  for (cmd = list->next; cmd; cmd = cmd->next)
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
  unsigned i;
  grub_err_t result;
  struct grub_script_argv argv = { 0, 0 };
  struct grub_script_cmdfor *cmdfor = (struct grub_script_cmdfor *) cmd;

  if (grub_script_arglist_to_argv (cmdfor->words, &argv))
    return grub_errno;

  result = 0;
  for (i = 0; i < argv.argc; i++)
    {
      grub_script_env_set (cmdfor->name->str, argv.args[i]);
      result = grub_script_execute_cmd (cmdfor->list);
    }

  grub_script_argv_free (&argv);
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
  struct grub_script_argv argv = { 0, 0 };

  cmd_menuentry = (struct grub_script_cmd_menuentry *) cmd;

  if (cmd_menuentry->arglist)
    {
      if (grub_script_arglist_to_argv (cmd_menuentry->arglist, &argv))
	return grub_errno;
    }

  grub_normal_add_menu_entry (argv.argc, (const char **) argv.args,
			      cmd_menuentry->sourcecode);

  grub_script_argv_free (&argv);
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

