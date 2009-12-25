/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/term.h>
#include <grub/i18n.h>
#include <grub/misc.h>

struct grub_term_autoload *grub_term_input_autoload = NULL;
struct grub_term_autoload *grub_term_output_autoload = NULL;

grub_err_t
grub_cmd_terminal_input (grub_command_t cmd __attribute__ ((unused)),
			 int argc, char **args)
{
  int i;
  grub_term_input_t term;
  struct grub_term_autoload *aut;

  if (argc == 0)
    {
      grub_puts_ (N_ ("Active input terminals:"));
      FOR_ACTIVE_TERM_INPUTS(term)
	grub_printf ("%s ", term->name);
      grub_printf ("\n");
      grub_puts_ (N_ ("Available input terminals:"));
      FOR_DISABLED_TERM_INPUTS(term)
	grub_printf ("%s ", term->name);
      /* This is quadratic but we don't expect mode than 30 terminal
	 modules ever.  */
      for (aut = grub_term_input_autoload; aut; aut = aut->next)
	{
	  FOR_DISABLED_TERM_INPUTS(term)
	    if (grub_strcmp (term->name, aut->name) == 0)
	      break;
	  if (!term)
	    FOR_ACTIVE_TERM_INPUTS(term)
	      if (grub_strcmp (term->name, aut->name) == 0)
		break;
	  if (!term)
	    grub_printf ("%s ", aut->name);
	}
      grub_printf ("\n");
      return GRUB_ERR_NONE;
    }
  i = 0;

  if (grub_strcmp (args[0], "--append") == 0
      || grub_strcmp (args[0], "--remove") == 0)
    i++;

  if (i == argc)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_ ("no terminal specified"));

  for (; i < argc; i++)
    {
      int again = 0;
      while (1)
	{
	  FOR_DISABLED_TERM_INPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term == 0)
	    FOR_ACTIVE_TERM_INPUTS(term)
	      if (grub_strcmp (args[i], term->name) == 0)
		break;
	  if (term)
	    break;
	  if (again)
	    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminal '%s'\n",
			       args[i]);
	  for (aut = grub_term_input_autoload; aut; aut = aut->next)
	    if (grub_strcmp (args[i], aut->name) == 0)
	      {
		grub_dl_t mod;
		mod = grub_dl_load (aut->modname);
		if (mod)
		  grub_dl_ref (mod);
		grub_errno = GRUB_ERR_NONE;
		break;
	      }
	  if (!aut)
	    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminal '%s'\n",
			       args[i]);
	  again = 1;
	}
    }

  if (grub_strcmp (args[0], "--append") == 0)
    {
      for (i = 1; i < argc; i++)
	{
	  FOR_DISABLED_TERM_INPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term)
	    {
	      grub_list_remove (GRUB_AS_LIST_P (&(grub_term_inputs_disabled)),
				GRUB_AS_LIST (term));
	      if (term->init)
		term->init ();
	      grub_list_push (GRUB_AS_LIST_P (&grub_term_inputs),
			      GRUB_AS_LIST (term));
	    }
	}
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[0], "--remove") == 0)
    {
      for (i = 1; i < argc; i++)
	{
	  FOR_ACTIVE_TERM_INPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term)
	    {
	      if (!term->next && term == grub_term_inputs)
		return grub_error (GRUB_ERR_BAD_ARGUMENT,
				   "can't remove the last terminal");
	      grub_list_remove (GRUB_AS_LIST_P (&(grub_term_inputs)),
				GRUB_AS_LIST (term));
	      if (term->fini)
		term->fini ();
	      grub_list_push (GRUB_AS_LIST_P (&grub_term_inputs_disabled),
			      GRUB_AS_LIST (term));
	    }
	}
      return GRUB_ERR_NONE;
    }
  for (i = 0; i < argc; i++)
    {
      FOR_DISABLED_TERM_INPUTS(term)
	if (grub_strcmp (args[i], term->name) == 0)
	  break;
      if (term)
	{
	  grub_list_remove (GRUB_AS_LIST_P (&(grub_term_inputs_disabled)),
			    GRUB_AS_LIST (term));
	  if (term->init)
	    term->init ();
	  grub_list_push (GRUB_AS_LIST_P (&grub_term_inputs),
			  GRUB_AS_LIST (term));
	}	
    }

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    for (i = 0; i < argc; i++)
      if (grub_strcmp (args[i], term->name) == 0)
	break;
    if (i == argc)
      {
	if (!term->next && term == grub_term_inputs)
	  return grub_error (GRUB_ERR_BAD_ARGUMENT,
			     "can't remove the last terminal");
	grub_list_remove (GRUB_AS_LIST_P (&(grub_term_inputs)),
			  GRUB_AS_LIST (term));
	if (term->fini)
	  term->fini ();
	grub_list_push (GRUB_AS_LIST_P (&grub_term_inputs_disabled),
			GRUB_AS_LIST (term));
      }
  }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_cmd_terminal_output (grub_command_t cmd __attribute__ ((unused)),
			 int argc, char **args)
{
  int i;
  grub_term_output_t term;
  struct grub_term_autoload *aut;

  if (argc == 0)
    {
      grub_puts_ (N_ ("Active output terminals:"));
      FOR_ACTIVE_TERM_OUTPUTS(term)
	grub_printf ("%s ", term->name);
      grub_printf ("\n");
      grub_puts_ (N_ ("Available output terminals:"));
      FOR_DISABLED_TERM_OUTPUTS(term)
	grub_printf ("%s ", term->name);
      /* This is quadratic but we don't expect mode than 30 terminal
	 modules ever.  */
      for (aut = grub_term_output_autoload; aut; aut = aut->next)
	{
	  FOR_DISABLED_TERM_OUTPUTS(term)
	    if (grub_strcmp (term->name, aut->name) == 0)
	      break;
	  if (!term)
	    FOR_ACTIVE_TERM_OUTPUTS(term)
	      if (grub_strcmp (term->name, aut->name) == 0)
		break;
	  if (!term)
	    grub_printf ("%s ", aut->name);
	}
      grub_printf ("\n");
      return GRUB_ERR_NONE;
    }
  i = 0;

  if (grub_strcmp (args[0], "--append") == 0
      || grub_strcmp (args[0], "--remove") == 0)
    i++;

  if (i == argc)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_ ("no terminal specified"));

  for (; i < argc; i++)
    {
      int again = 0;
      while (1)
	{
	  FOR_DISABLED_TERM_OUTPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term == 0)
	    FOR_ACTIVE_TERM_OUTPUTS(term)
	      if (grub_strcmp (args[i], term->name) == 0)
		break;
	  if (term)
	    break;
	  if (again)
	    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminal '%s'\n",
			       args[i]);
	  for (aut = grub_term_output_autoload; aut; aut = aut->next)
	    if (grub_strcmp (args[i], aut->name) == 0)
	      {
		grub_dl_t mod;
		mod = grub_dl_load (aut->modname);
		if (mod)
		  grub_dl_ref (mod);
		grub_errno = GRUB_ERR_NONE;
		break;
	      }
	  if (!aut)
	    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminal '%s'\n",
			       args[i]);
	  again = 1;
	}
    }

  if (grub_strcmp (args[0], "--append") == 0)
    {
      for (i = 1; i < argc; i++)
	{
	  FOR_DISABLED_TERM_OUTPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term)
	    {
	      grub_list_remove (GRUB_AS_LIST_P (&(grub_term_outputs_disabled)),
				GRUB_AS_LIST (term));
	      if (term->init)
		term->init ();
	      grub_list_push (GRUB_AS_LIST_P (&grub_term_outputs),
			      GRUB_AS_LIST (term));
	    }
	}
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[0], "--remove") == 0)
    {
      for (i = 1; i < argc; i++)
	{
	  FOR_ACTIVE_TERM_OUTPUTS(term)
	    if (grub_strcmp (args[i], term->name) == 0)
	      break;
	  if (term)
	    {
	      if (!term->next && term == grub_term_outputs)
		return grub_error (GRUB_ERR_BAD_ARGUMENT,
				   "can't remove the last terminal");
	      grub_list_remove (GRUB_AS_LIST_P (&(grub_term_outputs)),
				GRUB_AS_LIST (term));
	      if (term->fini)
		term->fini ();
	      grub_list_push (GRUB_AS_LIST_P (&grub_term_outputs_disabled),
			      GRUB_AS_LIST (term));
	    }
	}
      return GRUB_ERR_NONE;
    }

  for (i = 0; i < argc; i++)
    {
      FOR_DISABLED_TERM_OUTPUTS(term)
	if (grub_strcmp (args[i], term->name) == 0)
	  break;
      if (term)
	{
	  grub_list_remove (GRUB_AS_LIST_P (&(grub_term_outputs_disabled)),
			    GRUB_AS_LIST (term));
	  if (term->init)
	    term->init ();
	  grub_list_push (GRUB_AS_LIST_P (&grub_term_outputs),
			  GRUB_AS_LIST (term));
	}	
    }

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    for (i = 0; i < argc; i++)
      if (grub_strcmp (args[i], term->name) == 0)
	break;
    if (i == argc)
      {
	if (!term->next && term == grub_term_outputs)
	  return grub_error (GRUB_ERR_BAD_ARGUMENT,
			     "can't remove the last terminal");
	grub_list_remove (GRUB_AS_LIST_P (&(grub_term_outputs)),
			  GRUB_AS_LIST (term));
	if (term->fini)
	  term->fini ();
	grub_list_push (GRUB_AS_LIST_P (&grub_term_outputs_disabled),
			GRUB_AS_LIST (term));
      }
  }

  return GRUB_ERR_NONE;
}

static grub_command_t cmd_terminal_input, cmd_terminal_output;

GRUB_MOD_INIT(terminal)
{
  cmd_terminal_input =
    grub_register_command ("terminal_input", grub_cmd_terminal_input,
			   "terminal_input [--append|--remove] "
			   "[TERMINAL1] [TERMINAL2] ...",
			   "List or select an input terminal.");
  cmd_terminal_output =
    grub_register_command ("terminal_output", grub_cmd_terminal_output,
			   "terminal_output [--append|--remove] "
			   "[TERMINAL1] [TERMINAL2] ...",
			   "List or select an output terminal.");
}

GRUB_MOD_FINI(terminal)
{
  grub_unregister_command (cmd_terminal_input);
  grub_unregister_command (cmd_terminal_output);
}
