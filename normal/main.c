/* main.c - the normal mode main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/reader.h>
#include <grub/menu_viewer.h>
#include <grub/auth.h>
#include <grub/i18n.h>
#include <grub/charset.h>

#define GRUB_DEFAULT_HISTORY_SIZE	50

static int nested_level = 0;
int grub_normal_exit_level = 0;

/* Read a line from the file FILE.  */
char *
grub_file_getline (grub_file_t file)
{
  char c;
  int pos = 0;
  int literal = 0;
  char *cmdline;
  int max_len = 64;

  /* Initially locate some space.  */
  cmdline = grub_malloc (max_len);
  if (! cmdline)
    return 0;

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

      if (pos == 0)
	{
	  if (! grub_isspace (c))
	    cmdline[pos++] = c;
	}
      else
	{
	  if (pos >= max_len)
	    {
	      char *old_cmdline = cmdline;
	      max_len = max_len * 2;
	      cmdline = grub_realloc (cmdline, max_len);
	      if (! cmdline)
		{
		  grub_free (old_cmdline);
		  return 0;
		}
	    }

	  if (c == '\n')
	    break;

	  cmdline[pos++] = c;
	}
    }

  cmdline[pos] = '\0';

  /* If the buffer is empty, don't return anything at all.  */
  if (pos == 0)
    {
      grub_free (cmdline);
      cmdline = 0;
    }

  return cmdline;
}

static void
free_menu (grub_menu_t menu)
{
  grub_menu_entry_t entry = menu->entry_list;

  while (entry)
    {
      grub_menu_entry_t next_entry = entry->next;

      grub_free ((void *) entry->title);
      grub_free ((void *) entry->sourcecode);
      entry = next_entry;
    }

  grub_free (menu);
  grub_env_unset_menu ();
}

static void
free_menu_entry_classes (struct grub_menu_entry_class *head)
{
  /* Free all the classes.  */
  while (head)
    {
      struct grub_menu_entry_class *next;

      grub_free (head->name);
      next = head->next;
      grub_free (head);
      head = next;
    }
}

/* Add a menu entry to the current menu context (as given by the environment
   variable data slot `menu').  As the configuration file is read, the script
   parser calls this when a menu entry is to be created.  */
grub_err_t
grub_normal_add_menu_entry (int argc, const char **args,
			    const char *sourcecode)
{
  const char *menutitle = 0;
  const char *menusourcecode;
  grub_menu_t menu;
  grub_menu_entry_t *last;
  int failed = 0;
  int i;
  struct grub_menu_entry_class *classes_head;  /* Dummy head node for list.  */
  struct grub_menu_entry_class *classes_tail;
  char *users = NULL;

  /* Allocate dummy head node for class list.  */
  classes_head = grub_zalloc (sizeof (struct grub_menu_entry_class));
  if (! classes_head)
    return grub_errno;
  classes_tail = classes_head;

  menu = grub_env_get_menu ();
  if (! menu)
    return grub_error (GRUB_ERR_MENU, "no menu context");

  last = &menu->entry_list;

  menusourcecode = grub_strdup (sourcecode);
  if (! menusourcecode)
    return grub_errno;

  /* Parse menu arguments.  */
  for (i = 0; i < argc; i++)
    {
      /* Capture arguments.  */
      if (grub_strncmp ("--", args[i], 2) == 0)
	{
	  const char *arg = &args[i][2];

	  /* Handle menu class.  */
	  if (grub_strcmp(arg, "class") == 0)
	    {
	      char *class_name;
	      struct grub_menu_entry_class *new_class;

	      i++;
	      class_name = grub_strdup (args[i]);
	      if (! class_name)
		{
		  failed = 1;
		  break;
		}

	      /* Create a new class and add it at the tail of the list.  */
	      new_class = grub_zalloc (sizeof (struct grub_menu_entry_class));
	      if (! new_class)
		{
		  grub_free (class_name);
		  failed = 1;
		  break;
		}
	      /* Fill in the new class node.  */
	      new_class->name = class_name;
	      /* Link the tail to it, and make it the new tail.  */
	      classes_tail->next = new_class;
	      classes_tail = new_class;
	      continue;
	    }
	  else if (grub_strcmp(arg, "users") == 0)
	    {
	      i++;
	      users = grub_strdup (args[i]);
	      if (! users)
		{
		  failed = 1;
		  break;
		}

	      continue;
	    }
	  else
	    {
	      /* Handle invalid argument.  */
	      failed = 1;
	      grub_error (GRUB_ERR_MENU,
			  "invalid argument for menuentry: %s", args[i]);
	      break;
	    }
	}

      /* Capture title.  */
      if (! menutitle)
	{
	  menutitle = grub_strdup (args[i]);
	}
      else
	{
	  failed = 1;
	  grub_error (GRUB_ERR_MENU,
		      "too many titles for menuentry: %s", args[i]);
	  break;
	}
    }

  /* Validate arguments.  */
  if ((! failed) && (! menutitle))
    {
      grub_error (GRUB_ERR_MENU, "menuentry is missing title");
      failed = 1;
    }

  /* If argument parsing failed, free any allocated resources.  */
  if (failed)
    {
      free_menu_entry_classes (classes_head);
      grub_free ((void *) menutitle);
      grub_free ((void *) menusourcecode);

      /* Here we assume that grub_error has been used to specify failure details.  */
      return grub_errno;
    }

  /* Add the menu entry at the end of the list.  */
  while (*last)
    last = &(*last)->next;

  *last = grub_zalloc (sizeof (**last));
  if (! *last)
    {
      free_menu_entry_classes (classes_head);
      grub_free ((void *) menutitle);
      grub_free ((void *) menusourcecode);
      return grub_errno;
    }

  (*last)->title = menutitle;
  (*last)->classes = classes_head;
  if (users)
    (*last)->restricted = 1;
  (*last)->users = users;
  (*last)->sourcecode = menusourcecode;

  menu->size++;

  return GRUB_ERR_NONE;
}

static grub_menu_t
read_config_file (const char *config)
{
  grub_file_t file;
  grub_parser_t old_parser = 0;

  auto grub_err_t getline (char **line, int cont);
  grub_err_t getline (char **line, int cont __attribute__ ((unused)))
    {
      while (1)
	{
	  char *buf;

	  *line = buf = grub_file_getline (file);
	  if (! buf)
	    return grub_errno;

	  if (buf[0] == '#')
	    {
	      if (buf[1] == '!')
		{
		  grub_parser_t parser;
		  grub_named_list_t list;

		  buf += 2;
		  while (grub_isspace (*buf))
		    buf++;

		  if (! old_parser)
		    old_parser = grub_parser_get_current ();

		  list = GRUB_AS_NAMED_LIST (grub_parser_class.handler_list);
		  parser = grub_named_list_find (list, buf);
		  if (parser)
		    grub_parser_set_current (parser);
		  else
		    {
		      char cmd_name[8 + grub_strlen (buf)];

		      /* Perhaps it's not loaded yet, try the autoload
			 command.  */
		      grub_strcpy (cmd_name, "parser.");
		      grub_strcat (cmd_name, buf);
		      grub_command_execute (cmd_name, 0, 0);
		    }
		}
	      grub_free (*line);
	    }
	  else
	    break;
	}

      return GRUB_ERR_NONE;
    }

  grub_menu_t newmenu;

  newmenu = grub_env_get_menu ();
  if (! newmenu)
    {
      newmenu = grub_zalloc (sizeof (*newmenu));
      if (! newmenu)
	return 0;

      grub_env_set_menu (newmenu);
    }

  /* Try to open the config file.  */
  file = grub_file_open (config);
  if (! file)
    return 0;

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      if ((getline (&line, 0)) || (! line))
	break;

      grub_parser_get_current ()->parse_line (line, getline);
      grub_free (line);
    }

  grub_file_close (file);

  if (old_parser)
    grub_parser_set_current (old_parser);

  return newmenu;
}

/* Initialize the screen.  */
void
grub_normal_init_page (struct grub_term_output *term)
{
  int msg_len;
  int posx;
  const char *msg = _("GNU GRUB  version %s");
  char *msg_formatted;
  grub_uint32_t *unicode_msg;
  grub_uint32_t *last_position;
 
  grub_term_cls (term);

  msg_formatted = grub_xasprintf (msg, PACKAGE_VERSION);
  if (!msg_formatted)
    return;
 
  msg_len = grub_utf8_to_ucs4_alloc (msg_formatted,
  				     &unicode_msg, &last_position);
 
  if (msg_len < 0)
    {
      return;
    }

  posx = grub_getstringwidth (unicode_msg, last_position, term);
  posx = (grub_term_width (term) - posx) / 2;
  grub_term_gotoxy (term, posx, 1);

  grub_print_ucs4 (unicode_msg, last_position, term);
  grub_printf("\n\n");
  grub_free (unicode_msg);
}

static char *
read_lists (struct grub_env_var *var __attribute__ ((unused)),
	    const char *val)
{
  read_command_list ();
  read_fs_list ();
  read_crypto_list ();
  read_terminal_list ();
  return val ? grub_strdup (val) : NULL;
}

/* Read the config file CONFIG and execute the menu interface or
   the command line interface if BATCH is false.  */
void
grub_normal_execute (const char *config, int nested, int batch)
{
  grub_menu_t menu = 0;

  read_lists (NULL, NULL);
  read_handler_list ();
  grub_register_variable_hook ("prefix", NULL, read_lists);
  grub_command_execute ("parser.grub", 0, 0);

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      grub_errno = GRUB_ERR_NONE;
    }

  if (! batch)
    {
      if (menu && menu->size)
	{
	  grub_show_menu (menu, nested);
	  if (nested)
	    free_menu (menu);
	}
    }
}

/* This starts the normal mode.  */
void
grub_enter_normal_mode (const char *config)
{
  nested_level++;
  grub_normal_execute (config, 0, 0);
  grub_cmdline_run (0);
  nested_level--;
  if (grub_normal_exit_level)
    grub_normal_exit_level--;
}

/* Enter normal mode from rescue mode.  */
static grub_err_t
grub_cmd_normal (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename. It is necessary to make CONFIG static,
	 so that it won't get broken by longjmp.  */
      char *config;
      const char *prefix;

      prefix = grub_env_get ("prefix");
      if (prefix)
	{
	  config = grub_xasprintf ("%s/grub.cfg", prefix);
	  if (! config)
	    goto quit;

	  grub_enter_normal_mode (config);
	  grub_free (config);
	}
      else
	grub_enter_normal_mode (0);
    }
  else
    grub_enter_normal_mode (argv[0]);

quit:
  return 0;
}

/* Exit from normal mode to rescue mode.  */
static grub_err_t
grub_cmd_normal_exit (struct grub_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  if (nested_level <= grub_normal_exit_level)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not in normal environment");
  grub_normal_exit_level++;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_normal_reader_init (int nested)
{
  struct grub_term_output *term;
  const char *msg = _("Minimal BASH-like line editing is supported. For "
		      "the first word, TAB lists possible command completions. Anywhere "
		      "else TAB lists possible device or file completions. %s");
  const char *msg_esc = _("ESC at any time exits.");
  char *msg_formatted;

  msg_formatted = grub_xasprintf (msg, nested ? msg_esc : "");
  if (!msg_formatted)
    return grub_errno;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_normal_init_page (term);
    grub_term_setcursor (term, 1);
    
    grub_print_message_indented (msg_formatted, 3, STANDARD_MARGIN, term);
    grub_puts ("\n");
  }
  grub_free (msg_formatted);
 
  return 0;
}


static grub_err_t
grub_normal_read_line_real (char **line, int cont, int nested)
{
  grub_parser_t parser = grub_parser_get_current ();
  char *prompt;

  if (cont)
    prompt = grub_xasprintf (">");
  else
    prompt = grub_xasprintf ("%s>", parser->name);

  if (!prompt)
    return grub_errno;

  while (1)
    {
      *line = grub_cmdline_get (prompt);
      if (*line)
	break;

      if (cont || nested)
	{
	  grub_free (*line);
	  *line = 0;
	  return grub_errno;
	}
    }

  return 0;
}

static grub_err_t
grub_normal_read_line (char **line, int cont)
{
  return grub_normal_read_line_real (line, cont, 0);
}

void
grub_cmdline_run (int nested)
{
  grub_err_t err = GRUB_ERR_NONE;

  err = grub_auth_check_authentication (NULL);

  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_normal_reader_init (nested);

  while (1)
    {
      char *line;

      if (grub_normal_exit_level)
	break;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      grub_normal_read_line_real (&line, 0, nested);
      if (! line)
	break;

      grub_parser_get_current ()->parse_line (line, grub_normal_read_line);
      grub_free (line);
    }
}

static char *
grub_env_write_pager (struct grub_env_var *var __attribute__ ((unused)),
		      const char *val)
{
  grub_set_more ((*val == '1'));
  return grub_strdup (val);
}

GRUB_MOD_INIT(normal)
{
  grub_context_init ();

  /* Normal mode shouldn't be unloaded.  */
  if (mod)
    grub_dl_ref (mod);

  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  grub_register_variable_hook ("pager", 0, grub_env_write_pager);

  /* Register a command "normal" for the rescue mode.  */
  grub_register_command ("normal", grub_cmd_normal,
			 0, "Enter normal mode.");
  grub_register_command ("normal_exit", grub_cmd_normal_exit,
			 0, "Exit from normal mode.");

  /* Reload terminal colors when these variables are written to.  */
  grub_register_variable_hook ("color_normal", NULL, grub_env_write_color_normal);
  grub_register_variable_hook ("color_highlight", NULL, grub_env_write_color_highlight);

  /* Preserve hooks after context changes.  */
  grub_env_export ("color_normal");
  grub_env_export ("color_highlight");
}

GRUB_MOD_FINI(normal)
{
  grub_context_fini ();

  grub_set_history (0);
  grub_register_variable_hook ("pager", 0, 0);
  grub_fs_autoload_hook = 0;
  free_handler_list ();
}
