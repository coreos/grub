/* main.c - the normal mode main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/rescue.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/env.h>

grub_jmp_buf grub_exit_env;

#define GRUB_DEFAULT_HISTORY_SIZE	50

/* Read a line from the file FILE.  */
static int
get_line (grub_file_t file, char cmdline[], int max_len)
{
  char c;
  int pos = 0;
  int literal = 0;
  int comment = 0;

  while (1)
    {
      if (grub_file_read (file, &c, 1) != 1)
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
	  else if (! grub_isspace (c))
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
free_menu (grub_menu_t menu)
{
  grub_menu_entry_t entry = menu->entry_list;
  
  while (entry)
    {
      grub_menu_entry_t next_entry = entry->next;
      grub_command_list_t cmd = entry->command_list;
      
      while (cmd)
	{
	  grub_command_list_t next_cmd = cmd->next;

	  grub_free ((void *) cmd->command);
	  cmd = next_cmd;
	}

      grub_free ((void *) entry->title);
      entry = next_entry;
    }

  grub_free (menu);
}

/* Read the config file CONFIG and return a menu. If no entry is present,
   return NULL.  */
static grub_menu_t
read_config_file (const char *config)
{
  grub_file_t file;
  static char cmdline[GRUB_MAX_CMDLINE];
  grub_menu_t menu;
  grub_menu_entry_t *next_entry, cur_entry = 0;
  grub_command_list_t *next_cmd, cur_cmd;
  
  /* Try to open the config file.  */
  file = grub_file_open (config);
  if (! file)
    return 0;

  /* Initialize the menu.  */
  menu = (grub_menu_t) grub_malloc (sizeof (*menu));
  if (! menu)
    {
      grub_file_close (file);
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
      grub_command_t cmd;
      
      cmd = grub_command_find (cmdline);
      grub_errno = GRUB_ERR_NONE;
      if (! cmd)
	{
	  grub_printf ("Unknown command `%s' is ignored.\n", cmdline);
	  continue;
	}

      if (cmd->flags & GRUB_COMMAND_FLAG_TITLE)
	{
	  char *p;
	  
	  cur_entry = (grub_menu_entry_t) grub_malloc (sizeof (*cur_entry));
	  if (! cur_entry)
	    goto fail;

	  p = grub_strchr (cmdline, ' ');
	  if (p)
	    cur_entry->title = grub_strdup (p);
	  else
	    cur_entry->title = grub_strdup ("");
	  
	  if (! cur_entry->title)
	    {
	      grub_free (cur_entry);
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
	  if (cmd->flags & GRUB_COMMAND_FLAG_MENU)
	    {
	      grub_command_execute (cmdline);
	      grub_print_error ();
	      grub_errno = GRUB_ERR_NONE;
	    }
	  else
	    {
	      grub_printf ("Invalid command `%s' is ignored.\n", cmdline);
	      continue;
	    }
	}
      else
	{
	  cur_cmd = (grub_command_list_t) grub_malloc (sizeof (*cur_cmd));
	  if (! cur_cmd)
	    goto fail;

	  cur_cmd->command = grub_strdup (cmdline);
	  if (! cur_cmd->command)
	    {
	      grub_free (cur_cmd);
	      goto fail;
	    }

	  cur_cmd->next = 0;
	  
	  *next_cmd = cur_cmd;
	  next_cmd = &(cur_cmd->next);
	  
	  cur_entry->num++;
	}
    }

 fail:

  grub_file_close (file);

  /* If no entry was found or any error occurred, return NULL.  */
  if (menu->size == 0 || grub_errno != GRUB_ERR_NONE)
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
grub_enter_normal_mode (const char *config)
{
  if (grub_setjmp (grub_exit_env) == 0)
    grub_normal_execute (config, 0);
}

/* Initialize the screen.  */
void
grub_normal_init_page (void)
{
  grub_cls ();
  grub_printf ("\n\
                         GNU GRUB  version %s\n\n",
	       PACKAGE_VERSION);
}

/* Read the config file CONFIG and execute the menu interface or
   the command-line interface.  */
void
grub_normal_execute (const char *config, int nested)
{
  grub_menu_t menu = 0;

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      grub_errno = GRUB_ERR_NONE;
    }

  if (menu)
    grub_menu_run (menu, nested);
  else
    grub_cmdline_run (nested);
}

/* Enter normal mode from rescue mode.  */
static void
grub_rescue_cmd_normal (int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename.  */
      char *config;
      const char *prefix;
      
      prefix = grub_env_get ("prefix");
      if (prefix)
	{
	  config = grub_malloc (grub_strlen (prefix) + sizeof ("/grub.cfg"));
	  if (! config)
	    return;

	  grub_sprintf (config, "%s/grub.cfg", prefix);
	  grub_enter_normal_mode (config);
	  grub_free (config);
	}
      else
	grub_enter_normal_mode (0);
    }
  else
    grub_enter_normal_mode (argv[0]);
}

#ifdef GRUB_UTIL
void
grub_normal_init (void)
{
  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  /* Register a command "normal" for the rescue mode.  */
  grub_rescue_register_command ("normal", grub_rescue_cmd_normal,
				"enter normal mode");

  /* This registers some built-in commands.  */
  grub_command_init ();
  
}

void
grub_normal_fini (void)
{
  grub_set_history (0);
  grub_rescue_unregister_command ("normal");

}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  /* Normal mode shouldn't be unloaded.  */
  grub_dl_ref (mod);

  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  /* Register a command "normal" for the rescue mode.  */
  grub_rescue_register_command ("normal", grub_rescue_cmd_normal,
				"enter normal mode");

  /* This registers some built-in commands.  */
  grub_command_init ();
}

GRUB_MOD_FINI
{
  grub_set_history (0);
  grub_rescue_unregister_command ("normal");
}
#endif /* ! GRUB_UTIL */
