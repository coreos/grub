/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/normal.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/term.h>
#include <pupa/env.h>
#include <pupa/dl.h>

static pupa_command_t pupa_command_list;

void
pupa_register_command (const char *name,
		       pupa_err_t (*func) (struct pupa_arg_list *state,
					   int argc, char **args),
		       unsigned flags,
		       const char *summary,
		       const char *description,
		       const struct pupa_arg_option *options)
{
  pupa_command_t cmd, *p;

  cmd = (pupa_command_t) pupa_malloc (sizeof (*cmd));
  if (! cmd)
    return;

  cmd->name = name;
  cmd->func = func;
  cmd->flags = flags;
  cmd->summary = summary;
  cmd->description = description;
  cmd->options = options;

  /* Keep the list sorted for simplicity.  */
  p = &pupa_command_list;
  while (*p)
    {
      if (pupa_strcmp ((*p)->name, name) > 0)
	break;

      p = &((*p)->next);
    }

  cmd->next = *p;
  *p = cmd;
}

void
pupa_unregister_command (const char *name)
{
  pupa_command_t *p, q;

  for (p = &pupa_command_list, q = *p; q; p = &(q->next), q = q->next)
    if (pupa_strcmp (name, q->name) == 0)
      {
        *p = q->next;
        pupa_free (q);
        break;
      }
}

pupa_command_t
pupa_command_find (char *cmdline)
{
  char *first_space;
  pupa_command_t cmd;

  first_space = pupa_strchr (cmdline, ' ');
  if (first_space)
    *first_space = '\0';

  for (cmd = pupa_command_list; cmd; cmd = cmd->next)
    if (pupa_strcmp (cmdline, cmd->name) == 0)
      break;

  if (! cmd)
    pupa_error (PUPA_ERR_UNKNOWN_COMMAND, "unknown command `%s'", cmdline);
  
  if (first_space)
    *first_space = ' ';

  return cmd;
}

int
pupa_iterate_commands (int (*iterate) (pupa_command_t))
{
  pupa_command_t cmd;
  for (cmd = pupa_command_list; cmd; cmd = cmd->next)
    iterate (cmd);
  return 0;
}

int
pupa_command_execute (char *cmdline)
{
  auto pupa_err_t cmdline_get (char **s);
  pupa_err_t cmdline_get (char **s)
    {
      *s = pupa_malloc (PUPA_MAX_CMDLINE);
      *s[0] = '\0';
      return pupa_cmdline_get (">", *s, PUPA_MAX_CMDLINE, 0, 1);
    }

  pupa_command_t cmd;
  pupa_err_t ret = 0;
  char *pager;
  int num;
  char **args;
  struct pupa_arg_list *state;
  struct pupa_arg_option *parser;
  int maxargs = 0;
  char **arglist;
  int numargs;

  if (pupa_split_cmdline (cmdline, cmdline_get, &num, &args))
    return 0;
  
  /* In case of an assignment set the environment accordingly instead
     of calling a function.  */
  if (num == 0 && pupa_strchr (args[0], '='))
    {
      char *val = pupa_strchr (args[0], '=');
      val[0] = 0;
      pupa_env_set (args[0], val + 1);
      val[0] = '=';
      return 0;
    }
  
  cmd = pupa_command_find (args[0]);
  if (! cmd)
    return -1;

  /* Enable the pager if the environment pager is set to 1.  */
  pager = pupa_env_get ("pager");
  if (pager && (! pupa_strcmp (pager, "1")))
    pupa_set_more (1);
  
  parser = (struct pupa_arg_option *) cmd->options;
  while (parser && (parser++)->doc)
    maxargs++;

  state = pupa_malloc (sizeof (struct pupa_arg_list) * maxargs);
  pupa_memset (state, 0, sizeof (struct pupa_arg_list) * maxargs);
  if (pupa_arg_parse (cmd, num, &args[1], state, &arglist, &numargs))
    ret = (cmd->func) (state, numargs, arglist);
  pupa_free (state);

  if (pager && (! pupa_strcmp (pager, "1")))
    pupa_set_more (0);
  
  pupa_free (args);
  return ret;
}

static pupa_err_t
rescue_command (struct pupa_arg_list *state __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  pupa_longjmp (pupa_exit_env, 0);

  /* Never reach here.  */
  return 0;
}


static pupa_err_t
set_command (struct pupa_arg_list *state __attribute__ ((unused)),
	     int argc, char **args)
{
  char *var;
  char *val;

  auto int print_env (struct pupa_env_var *env);
  int print_env (struct pupa_env_var *env)
    {
      pupa_printf ("%s=%s\n", env->name, env->value);
      return 0;
    }
  
  if (! argc)
    {
      pupa_env_iterate (print_env);
      return 0;
    }
  
  var = args[0];
  val = pupa_strchr (var, '=');
  if (! val)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "not an assignment");
      return pupa_errno;
    }
  
  val[0] = 0;
  pupa_env_set (var, val + 1);
  val[0] = '=';
  return 0;
}

static pupa_err_t
unset_command (struct pupa_arg_list *state __attribute__ ((unused)),
	       int argc, char **args)
{
  if (argc < 1)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT,
		       "no environment variable specified");

  pupa_env_unset (args[0]);
  return 0;
}

static pupa_err_t
insmod_command (struct pupa_arg_list *state __attribute__ ((unused)),
		int argc, char **args)
{
  char *p;
  pupa_dl_t mod;
  
  if (argc == 0)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "no module specified");

  p = pupa_strchr (args[0], '/');
  if (! p)
    mod = pupa_dl_load (args[0]);
  else
    mod = pupa_dl_load_file (args[0]);

  if (mod)
    pupa_dl_ref (mod);

  return 0;
}

static pupa_err_t
rmmod_command (struct pupa_arg_list *state __attribute__ ((unused)),
		int argc, char **args)
{
  pupa_dl_t mod;
  
  if (argc == 0)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "no module specified");
  
  mod = pupa_dl_get (args[0]);
  if (! mod)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "no such module");

  if (! pupa_dl_unref (mod))
    pupa_dl_unload (mod);

  return 0;
}

static pupa_err_t
lsmod_command (struct pupa_arg_list *state __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  auto int print_module (pupa_dl_t mod);

  int print_module (pupa_dl_t mod)
    {
      pupa_dl_dep_t dep;
      
      pupa_printf ("%s\t%d\t\t", mod->name, mod->ref_count);
      for (dep = mod->dep; dep; dep = dep->next)
	{
	  if (dep != mod->dep)
	    pupa_putchar (',');

	  pupa_printf ("%s", dep->mod->name);
	}
      pupa_putchar ('\n');
      pupa_refresh ();

      return 0;
    }

  pupa_printf ("Name\tRef Count\tDependencies\n");
  pupa_dl_iterate (print_module);
  return 0;
}

void
pupa_command_init (void)
{
  /* This is a special command, because this never be called actually.  */
  pupa_register_command ("title", 0, PUPA_COMMAND_FLAG_TITLE, 0, 0, 0);

  pupa_register_command ("rescue", rescue_command, PUPA_COMMAND_FLAG_BOTH,
			 "rescue", "Enter into the rescue mode.", 0);

  pupa_register_command ("set", set_command, PUPA_COMMAND_FLAG_BOTH,
			 "unset ENVVAR", "Set an environment variable.", 0);

  pupa_register_command ("unset", unset_command, PUPA_COMMAND_FLAG_BOTH,
			 "set [ENVVAR=VALUE]", "Remove an environment variable.", 0);

  pupa_register_command ("insmod", insmod_command, PUPA_COMMAND_FLAG_BOTH,
			 "insmod MODULE|FILE", "Insert a module.", 0);

  pupa_register_command ("rmmod", rmmod_command, PUPA_COMMAND_FLAG_BOTH,
			 "rmmod MODULE", "Remove a module.", 0);

  pupa_register_command ("lsmod", lsmod_command, PUPA_COMMAND_FLAG_BOTH,
			 "lsmod", "Show loaded modules.", 0);
}
