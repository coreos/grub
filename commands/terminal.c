/* terminal.c - command to show and select a terminal */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/normal.h>
#include <pupa/dl.h>
#include <pupa/arg.h>
#include <pupa/misc.h>
#include <pupa/term.h>

static pupa_err_t
pupa_cmd_terminal (struct pupa_arg_list *state __attribute__ ((unused)),
		   int argc, char **args)
{
  pupa_term_t term = 0;
  
  auto int print_terminal (pupa_term_t);
  auto int find_terminal (pupa_term_t);
  
  int print_terminal (pupa_term_t t)
    {
      pupa_printf (" %s", t->name);
      return 0;
    }

  int find_terminal (pupa_term_t t)
    {
      if (pupa_strcmp (t->name, args[0]) == 0)
	{
	  term = t;
	  return 1;
	}

      return 0;
    }
  
  if (argc == 0)
    {
      pupa_printf ("Available terminal(s):");
      pupa_term_iterate (print_terminal);
      pupa_putchar ('\n');
      
      pupa_printf ("Current terminal: %s\n", pupa_term_get_current ()->name);
    }
  else
    {
      pupa_term_iterate (find_terminal);
      if (! term)
	return pupa_error (PUPA_ERR_BAD_ARGUMENT, "no such terminal");

      pupa_term_set_current (term);
    }

  return PUPA_ERR_NONE;
}


#ifdef PUPA_UTIL
void
pupa_terminal_init (void)
{
  pupa_register_command ("terminal", pupa_cmd_terminal, PUPA_COMMAND_FLAG_BOTH,
			 "terminal [TERM...]", "Select a terminal.", 0);
}

void
pupa_terminal_fini (void)
{
  pupa_unregister_command ("terminal");
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("terminal", pupa_cmd_terminal, PUPA_COMMAND_FLAG_BOTH,
			 "terminal [TERM...]", "Select a terminal.", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("terminal");
}
#endif /* ! PUPA_UTIL */
