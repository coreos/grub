/* main.c - the normal mode main routine */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2002,2003  Yoshinori K. Okuji <okuji@enbug.org>
 *  Copyright (C) 2003  Marco Gerards <metgerards@student.han.nl>.
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

#include <pupa/kernel.h>
#include <pupa/normal.h>
#include <pupa/dl.h>
#include <pupa/rescue.h>
#include <pupa/misc.h>
#include <pupa/file.h>
#include <pupa/mm.h>
#include <pupa/term.h>

pupa_jmp_buf pupa_exit_env;

#define PUPA_DEFAULT_HISTORY_SIZE	50

/* Read a line from the file FILE.  */
static int
get_line (pupa_file_t file, char cmdline[], int max_len)
{
  char c;
  int pos = 0;
  int literal = 0;
  int comment = 0;

  while (1)
    {
      if (pupa_file_read (file, &c, 1) != 1)
	break;

      /* Skip all carriage returns.  */
      if (c == '\r')
	continue;

      /* Replace tabs with spaces.  */
      if (c == '\t')
	c = ' ';

      /* The previous is a backslash, then...  */
      if (literal)
	{
	  /* If it is a newline, replace it with a space and continue.  */
	  if (c == '\n')
	    {
	      c = ' ';

	      /* Go back to overwrite the backslash.  */
	      if (pos > 0)
		pos--;
	    }

	  literal = 0;
	}

      if (c == '\\')
	literal = 1;

      if (comment)
	{
	  if (c == '\n')
	    comment = 0;
	}
      else if (pos == 0)
	{
	  if (c == '#')
	    comment = 1;
	  else if (! pupa_isspace (c))
	    cmdline[pos++] = c;
	}
      else
	{
	  if (c == '\n')
	    break;

	  if (pos < max_len)
	    cmdline[pos++] = c;
	}
    }

  cmdline[pos] = '\0';
  
  return pos;
}

static void
free_menu (pupa_menu_t menu)
{
  pupa_menu_entry_t entry = menu->entry_list;
  
  while (entry)
    {
      pupa_menu_entry_t next_entry = entry->next;
      pupa_command_list_t cmd = entry->command_list;
      
      while (cmd)
	{
	  pupa_command_list_t next_cmd = cmd->next;

	  pupa_free ((void *) cmd->command);
	  cmd = next_cmd;
	}

      pupa_free ((void *) entry->title);
      entry = next_entry;
    }

  pupa_free (menu);
}

/* Read the config file CONFIG and return a menu. If no entry is present,
   return NULL.  */
static pupa_menu_t
read_config_file (const char *config)
{
  pupa_file_t file;
  static char cmdline[PUPA_MAX_CMDLINE];
  pupa_menu_t menu;
  pupa_menu_entry_t *next_entry, cur_entry = 0;
  pupa_command_list_t *next_cmd, cur_cmd;
  
  /* Try to open the config file.  */
  file = pupa_file_open (config);
  if (! file)
    return 0;

  /* Initialize the menu.  */
  menu = (pupa_menu_t) pupa_malloc (sizeof (*menu));
  if (! menu)
    {
      pupa_file_close (file);
      return 0;
    }
  menu->default_entry = 0;
  menu->fallback_entry = -1;
  menu->timeout = -1;
  menu->size = 0;
  menu->entry_list = 0;

  next_entry = &(menu->entry_list);
  next_cmd = 0;
  
  /* Read each line.  */
  while (get_line (file, cmdline, sizeof (cmdline)))
    {
      pupa_command_t cmd;
      
      cmd = pupa_command_find (cmdline);
      pupa_errno = PUPA_ERR_NONE;
      if (! cmd)
	{
	  pupa_printf ("Unknown command `%s' is ignored.\n", cmdline);
	  continue;
	}

      if (cmd->flags & PUPA_COMMAND_FLAG_TITLE)
	{
	  char *p;
	  
	  cur_entry = (pupa_menu_entry_t) pupa_malloc (sizeof (*cur_entry));
	  if (! cur_entry)
	    goto fail;

	  p = pupa_strchr (cmdline, ' ');
	  if (p)
	    cur_entry->title = pupa_strdup (p);
	  else
	    cur_entry->title = pupa_strdup ("");
	  
	  if (! cur_entry->title)
	    {
	      pupa_free (cur_entry);
	      goto fail;
	    }
	  
	  cur_entry->num = 0;
	  cur_entry->command_list = 0;
	  cur_entry->next = 0;
	  
	  *next_entry = cur_entry;
	  next_entry = &(cur_entry->next);

	  next_cmd = &(cur_entry->command_list);
	  
	  menu->size++;
	}
      else if (! cur_entry)
	{
	  /* Run the command if possible.  */
	  if (cmd->flags & PUPA_COMMAND_FLAG_MENU)
	    {
	      pupa_command_execute (cmdline);
	      pupa_print_error ();
	      pupa_errno = PUPA_ERR_NONE;
	    }
	  else
	    {
	      pupa_printf ("Invalid command `%s' is ignored.\n", cmdline);
	      continue;
	    }
	}
      else
	{
	  cur_cmd = (pupa_command_list_t) pupa_malloc (sizeof (*cur_cmd));
	  if (! cur_cmd)
	    goto fail;

	  cur_cmd->command = pupa_strdup (cmdline);
	  if (! cur_cmd->command)
	    {
	      pupa_free (cur_cmd);
	      goto fail;
	    }

	  cur_cmd->next = 0;
	  
	  *next_cmd = cur_cmd;
	  next_cmd = &(cur_cmd->next);
	  
	  cur_entry->num++;
	}
    }

 fail:

  pupa_file_close (file);

  /* If no entry was found or any error occurred, return NULL.  */
  if (menu->size == 0 || pupa_errno != PUPA_ERR_NONE)
    {
      free_menu (menu);
      return 0;
    }

  /* Check values of the default entry and the fallback one.  */
  if (menu->fallback_entry >= menu->size)
    menu->fallback_entry = -1;

  if (menu->default_entry < 0 || menu->default_entry >= menu->size)
    {
      if (menu->fallback_entry < 0)
	menu->default_entry = 0;
      else
	{
	  menu->default_entry = menu->fallback_entry;
	  menu->fallback_entry = -1;
	}
    }
  
  return menu;
}

/* This starts the normal mode.  */
void
pupa_enter_normal_mode (const char *config)
{
  if (pupa_setjmp (pupa_exit_env) == 0)
    pupa_normal_execute (config, 0);
}

/* Initialize the screen.  */
void
pupa_normal_init_page (void)
{
  pupa_cls ();
  pupa_printf ("\n\
                             PUPA  version %s\n\n",
	       PACKAGE_VERSION);
}

/* Read the config file CONFIG and execute the menu interface or
   the command-line interface.  */
void
pupa_normal_execute (const char *config, int nested)
{
  pupa_menu_t menu = 0;

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      pupa_errno = PUPA_ERR_NONE;
    }

  if (menu)
    pupa_menu_run (menu, nested);
  else
    pupa_cmdline_run (nested);
}

/* Enter normal mode from rescue mode.  */
static void
pupa_rescue_cmd_normal (int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename.  */
      char *config;
      const char *prefix;
      
      prefix = pupa_dl_get_prefix ();
      if (prefix)
	{
	  config = pupa_malloc (pupa_strlen (prefix) + sizeof ("/pupa.cfg"));
	  if (! config)
	    return;

	  pupa_sprintf (config, "%s/pupa.cfg", prefix);
	  pupa_enter_normal_mode (config);
	  pupa_free (config);
	}
      else
	pupa_enter_normal_mode (0);
    }
  else
    pupa_enter_normal_mode (argv[0]);
}

#ifdef PUPA_UTIL
void
pupa_normal_init (void)
{
  pupa_set_history (PUPA_DEFAULT_HISTORY_SIZE);

  /* Register a command "normal" for the rescue mode.  */
  pupa_rescue_register_command ("normal", pupa_rescue_cmd_normal,
				"enter normal mode");

  /* This registers some built-in commands.  */
  pupa_command_init ();
  
}

void
pupa_normal_fini (void)
{
  pupa_set_history (0);
  pupa_rescue_unregister_command ("normal");

}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  /* Normal mode shouldn't be unloaded.  */
  pupa_dl_ref (mod);

  pupa_set_history (PUPA_DEFAULT_HISTORY_SIZE);

  /* Register a command "normal" for the rescue mode.  */
  pupa_rescue_register_command ("normal", pupa_rescue_cmd_normal,
				"enter normal mode");

  /* This registers some built-in commands.  */
  pupa_command_init ();
}

PUPA_MOD_FINI
{
  pupa_set_history (0);
  pupa_rescue_unregister_command ("normal");
}
#endif /* ! PUPA_UTIL */
