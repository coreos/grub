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
#include <grub/rescue.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/script.h>
#include <grub/menu_viewer.h>

grub_jmp_buf grub_exit_env;

static grub_fs_module_list_t fs_module_list = 0;

#define GRUB_DEFAULT_HISTORY_SIZE	50

/* Read a line from the file FILE.  */
static char *
get_line (grub_file_t file)
{
  char c;
  int pos = 0;
  int literal = 0;
  int comment = 0;
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

      grub_script_free (entry->commands);
      grub_free ((void *) entry->title);
      grub_free ((void *) entry->sourcecode);
      entry = next_entry;
    }

  grub_free (menu);
  grub_env_unset_data_slot ("menu");
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

grub_err_t
grub_normal_menu_addentry (int argc, const char **args,
                           struct grub_script *script, const char *sourcecode)
{
  const char *menutitle = 0;
  const char *menusourcecode;
  grub_menu_t menu;
  grub_menu_entry_t *last;
  int failed = 0;
  int i;
  struct grub_menu_entry_class *classes_head;  /* Dummy head node for list.  */
  struct grub_menu_entry_class *classes_tail;

  /* Allocate dummy head node for class list.  */
  classes_head = grub_malloc (sizeof (struct grub_menu_entry_class));
  if (! classes_head)
    return grub_errno;
  classes_head->name = 0;
  classes_head->next = 0;
  classes_tail = classes_head;

  menu = grub_env_get_data_slot ("menu");
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
	      new_class = grub_malloc (sizeof (struct grub_menu_entry_class));
	      if (! new_class)
		{
		  grub_free (class_name);
		  failed = 1;
		  break;
		}
	      /* Fill in the new class node.  */
	      new_class->name = class_name;
	      new_class->next = 0;
	      /* Link the tail to it, and make it the new tail.  */
	      classes_tail->next = new_class;
	      classes_tail = new_class;
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

  *last = grub_malloc (sizeof (**last));
  if (! *last)
    {
      free_menu_entry_classes (classes_head);
      grub_free ((void *) menutitle);
      grub_free ((void *) menusourcecode);
      return grub_errno;
    }

  (*last)->commands = script;
  (*last)->title = menutitle;
  (*last)->classes = classes_head;
  (*last)->next = 0;
  (*last)->sourcecode = menusourcecode;

  menu->size++;

  return GRUB_ERR_NONE;
}

static grub_menu_t
read_config_file (const char *config, int nested)
{
  grub_file_t file;
  auto grub_err_t getline (char **line);
  int currline = 0;
  int errors = 0;
  
  grub_err_t getline (char **line)
    {
      currline++;

      *line = get_line (file);
      if (! *line)
	return grub_errno;

      return GRUB_ERR_NONE;
    }

  grub_menu_t newmenu;

  newmenu = grub_env_get_data_slot ("menu");

  if (nested || ! newmenu)
    {
      newmenu = grub_malloc (sizeof (*newmenu));
      if (! newmenu)
	return 0;
      newmenu->size = 0;
      newmenu->entry_list = 0;
    }

  /* Try to open the config file.  */
  file = grub_file_open (config);
  if (! file)
    return 0;

  grub_env_set_data_slot ("menu", newmenu);

  while (1)
    {
      struct grub_script *parsed_script;
      int startline;
      char *cmdline;

      cmdline = get_line (file);
      if (!cmdline)
	break;

      startline = ++currline;

      /* Execute the script, line for line.  */
      parsed_script = grub_script_parse (cmdline, getline);

      grub_free (cmdline);

      if (! parsed_script)
	{
	  grub_printf ("(line %d-%d)\n", startline, currline);
	  errors++;
	  continue;
	}

      /* Execute the command(s).  */
      grub_script_execute (parsed_script);

      /* Ignore errors.  */
      grub_errno = GRUB_ERR_NONE;

      /* The parsed script was executed, throw it away.  */
      grub_script_free (parsed_script);
    }

  grub_file_close (file);

  if (errors > 0)
    grub_wait_after_message ();

  return newmenu;
}

/* This starts the normal mode.  */
void
grub_enter_normal_mode (const char *config)
{
  if (grub_setjmp (grub_exit_env) == 0)
    grub_normal_execute (config, 0, 0);
}

/* Initialize the screen.  */
void
grub_normal_init_page (void)
{
  grub_uint8_t width, margin;

#define TITLE ("GNU GRUB  version " PACKAGE_VERSION)

  width = grub_getwh () >> 8;
  margin = (width - (sizeof(TITLE) + 7)) / 2;

  grub_cls ();
  grub_putchar ('\n');

  while (margin--)
    grub_putchar (' ');

  grub_printf ("%s\n\n", TITLE);

#undef TITLE
}

static grub_err_t
grub_dyncmd_dispatcher (struct grub_command *cmd,
			int argc, char **args)
{
  char *modname = cmd->data;
  grub_dl_t mod;
  grub_err_t ret;

  mod = grub_dl_load (modname);
  if (mod)
    {
      char *name;

      grub_free (modname);
      grub_dl_ref (mod);

      name = (char *) cmd->name;
      grub_unregister_command (cmd);

      cmd = grub_command_find (name);
      if (cmd)
	ret = (cmd->func) (cmd, argc, args);
      else
	ret = grub_errno;

      grub_free (name);
    }
  else
    ret = grub_errno;

  return ret;
}

/* Read the file command.lst for auto-loading.  */
static void
read_command_list (void)
{
  const char *prefix;
  static int first_time = 1;

  /* Make sure that this function does not get executed twice.  */
  if (! first_time)
    return;
  first_time = 0;
    
  prefix = grub_env_get ("prefix");
  if (prefix)
    {
      char *filename;

      filename = grub_malloc (grub_strlen (prefix) + sizeof ("/command.lst"));
      if (filename)
	{
	  grub_file_t file;
	  
	  grub_sprintf (filename, "%s/command.lst", prefix);
	  file = grub_file_open (filename);
	  if (file)
	    {
	      char *buf = 0;
	      for (;; grub_free(buf))
		{
		  char *p, *name, *modname;
		  grub_command_t cmd;
		  int prio = 0;

		  buf = get_line (file);

		  if (! buf)
		    break;
		  
		  name = buf;
		  if (*name == '*')
		    {
		      name++;
		      prio++;
		    }

		  if (! grub_isgraph (name[0]))
		    continue;

		  p = grub_strchr (name, ':');
		  if (! p)
		    continue;

		  *p = '\0';
		  while (*++p == ' ')
		    ;

		  if (! grub_isgraph (*p))
		    continue;

		  if (grub_dl_get (p))
		    continue;

		  name = grub_strdup (name);
		  if (! name)
		    continue;

		  modname = grub_strdup (p);
		  if (! modname)
		    {
		      grub_free (name);
		      continue;
		    }

		  cmd = grub_register_command_prio (name,
						    grub_dyncmd_dispatcher,
						    0, "not loaded", prio);
		  if (! cmd)
		    {
		      grub_free (name);
		      grub_free (modname);
		      continue;
		    }
		  cmd->flags |= GRUB_COMMAND_FLAG_DYNCMD;
		  cmd->data = modname;

		  /* Update the active flag.  */
		  grub_command_find (name);
		}

	      grub_file_close (file);
	    }

	  grub_free (filename);
	}
    }

  /* Ignore errors.  */
  grub_errno = GRUB_ERR_NONE;
}

/* The auto-loading hook for filesystems.  */
static int
autoload_fs_module (void)
{
  grub_fs_module_list_t p;

  while ((p = fs_module_list) != 0)
    {
      if (! grub_dl_get (p->name) && grub_dl_load (p->name))
	return 1;

      fs_module_list = p->next;
      grub_free (p->name);
      grub_free (p);
    }

  return 0;
}

/* Read the file fs.lst for auto-loading.  */
static void
read_fs_list (void)
{
  const char *prefix;
  static int first_time = 1;

  /* Make sure that this function does not get executed twice.  */
  if (! first_time)
    return;
  first_time = 0;
  
  prefix = grub_env_get ("prefix");
  if (prefix)
    {
      char *filename;

      filename = grub_malloc (grub_strlen (prefix) + sizeof ("/fs.lst"));
      if (filename)
	{
	  grub_file_t file;
	  
	  grub_sprintf (filename, "%s/fs.lst", prefix);
	  file = grub_file_open (filename);
	  if (file)
	    {
	      while (1)
		{
		  char *buf;
		  char *p;
		  char *q;
		  grub_fs_module_list_t fs_mod;
		  
		  buf = get_line (file);
		  if (! buf)
		    break;

		  p = buf;
		  q = buf + grub_strlen (buf) - 1;

		  /* Ignore space.  */
		  while (grub_isspace (*p))
		    p++;

		  while (p < q && grub_isspace (*q))
		    *q-- = '\0';

		  /* If the line is empty, skip it.  */
		  if (p >= q)
		    continue;

		  fs_mod = grub_malloc (sizeof (*fs_mod));
		  if (! fs_mod)
		    continue;

		  fs_mod->name = grub_strdup (p);
		  if (! fs_mod->name)
		    {
		      grub_free (fs_mod);
		      continue;
		    }

		  fs_mod->next = fs_module_list;
		  fs_module_list = fs_mod;
		}

	      grub_file_close (file);
	    }

	  grub_free (filename);
	}
    }

  /* Ignore errors.  */
  grub_errno = GRUB_ERR_NONE;

  /* Set the hook.  */
  grub_fs_autoload_hook = autoload_fs_module;
}

/* Read the config file COFIG, and execute the menu interface or
   the command-line interface if BATCH is false.  */
void
grub_normal_execute (const char *config, int nested, int batch)
{
  grub_menu_t menu = 0;

  read_command_list ();
  read_fs_list ();
  
  if (config)
    {
      menu = read_config_file (config, nested);

      /* Ignore any error.  */
      grub_errno = GRUB_ERR_NONE;
    }

  if (! batch)
    {
      if (menu && menu->size)
        {
          grub_menu_viewer_show_menu (menu, nested);
          if (nested)
            free_menu (menu);
        }
      else
        grub_cmdline_run (nested);
    }
}

static grub_err_t
grub_cmd_rescue (struct grub_command *cmd __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  grub_longjmp (grub_exit_env, 0);

  /* Never reach here.  */
  return 0;
}

static grub_command_t cmd_normal;

/* Enter normal mode from rescue mode.  */
static grub_err_t
grub_cmd_normal (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_command_t cmd_rescue;

  grub_unregister_command (cmd_normal);
  cmd_rescue = grub_register_command ("rescue", grub_cmd_rescue,
				      0, "enter rescue mode");

  if (argc == 0)
    {
      /* Guess the config filename. It is necessary to make CONFIG static,
	 so that it won't get broken by longjmp.  */
      static char *config;
      const char *prefix;
      
      prefix = grub_env_get ("prefix");
      if (prefix)
	{
	  config = grub_malloc (grub_strlen (prefix) + sizeof ("/grub.cfg"));
	  if (! config)
	    goto quit;

	  grub_sprintf (config, "%s/grub.cfg", prefix);
	  grub_enter_normal_mode (config);
	  grub_free (config);
	}
      else
	grub_enter_normal_mode (0);
    }
  else
    grub_enter_normal_mode (argv[0]);

quit:
  grub_unregister_command (cmd_rescue);
  cmd_normal = grub_register_command_prio ("normal", grub_cmd_normal,
					   0, "enter normal mode", 0);
  return 0;
}

GRUB_MOD_INIT(normal)
{
  /* Normal mode shouldn't be unloaded.  */
  if (mod)
    grub_dl_ref (mod);

  grub_menu_viewer_register (&grub_normal_text_menu_viewer);

  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  /* Register a command "normal" for the rescue mode.  */
  cmd_normal = grub_register_command ("normal", grub_cmd_normal,
				      0, "enter normal mode");

  /* Reload terminal colors when these variables are written to.  */
  grub_register_variable_hook ("color_normal", NULL, grub_env_write_color_normal);
  grub_register_variable_hook ("color_highlight", NULL, grub_env_write_color_highlight);

  /* Preserve hooks after context changes.  */
  grub_env_export ("color_normal");
  grub_env_export ("color_highlight");
}

GRUB_MOD_FINI(normal)
{
  grub_set_history (0);
  grub_unregister_command (cmd_normal);
}
