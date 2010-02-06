/* menu.c - General supporting functionality for menus.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/env.h>
#include <grub/menu_viewer.h>
#include <grub/command.h>
#include <grub/parser.h>
#include <grub/auth.h>
#include <grub/i18n.h>
#include <grub/term.h>

/* Time to delay after displaying an error message about a default/fallback
   entry failing to boot.  */
#define DEFAULT_ENTRY_ERROR_DELAY_MS  2500

grub_err_t (*grub_gfxmenu_try_hook) (int entry, grub_menu_t menu,
				     int nested) = NULL;

/* Wait until the user pushes any key so that the user
   can see what happened.  */
void
grub_wait_after_message (void)
{
  grub_putchar ('\n');
  grub_printf_ (N_("Press any key to continue..."));
  (void) grub_getkey ();
  grub_putchar ('\n');
}

/* Get a menu entry by its index in the entry list.  */
grub_menu_entry_t
grub_menu_get_entry (grub_menu_t menu, int no)
{
  grub_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

/* Return the current timeout. If the variable "timeout" is not set or
   invalid, return -1.  */
int
grub_menu_get_timeout (void)
{
  char *val;
  int timeout;

  val = grub_env_get ("timeout");
  if (! val)
    return -1;

  grub_error_push ();

  timeout = (int) grub_strtoul (val, 0, 0);

  /* If the value is invalid, unset the variable.  */
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_env_unset ("timeout");
      grub_errno = GRUB_ERR_NONE;
      timeout = -1;
    }

  grub_error_pop ();

  return timeout;
}

/* Set current timeout in the variable "timeout".  */
void
grub_menu_set_timeout (int timeout)
{
  /* Ignore TIMEOUT if it is zero, because it will be unset really soon.  */
  if (timeout > 0)
    {
      char buf[16];

      grub_snprintf (buf, sizeof (buf), "%d", timeout);
      grub_env_set ("timeout", buf);
    }
}

/* Get the first entry number from the value of the environment variable NAME,
   which is a space-separated list of non-negative integers.  The entry number
   which is returned is stripped from the value of NAME.  If no entry number
   can be found, -1 is returned.  */
static int
get_and_remove_first_entry_number (const char *name)
{
  char *val;
  char *tail;
  int entry;

  val = grub_env_get (name);
  if (! val)
    return -1;

  grub_error_push ();

  entry = (int) grub_strtoul (val, &tail, 0);

  if (grub_errno == GRUB_ERR_NONE)
    {
      /* Skip whitespace to find the next digit.  */
      while (*tail && grub_isspace (*tail))
	tail++;
      grub_env_set (name, tail);
    }
  else
    {
      grub_env_unset (name);
      grub_errno = GRUB_ERR_NONE;
      entry = -1;
    }

  grub_error_pop ();

  return entry;
}

/* Run a menu entry.  */
void
grub_menu_execute_entry(grub_menu_entry_t entry)
{
  grub_err_t err = GRUB_ERR_NONE;

  if (entry->restricted)
    err = grub_auth_check_authentication (entry->users);

  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_env_set ("chosen", entry->title);

  grub_parser_execute ((char *) entry->sourcecode);

  if (grub_errno == GRUB_ERR_NONE && grub_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    grub_command_execute ("boot", 0, 0);
}

/* Execute ENTRY from the menu MENU, falling back to entries specified
   in the environment variable "fallback" if it fails.  CALLBACK is a
   pointer to a struct of function pointers which are used to allow the
   caller provide feedback to the user.  */
void
grub_menu_execute_with_fallback (grub_menu_t menu,
				 grub_menu_entry_t entry,
				 grub_menu_execute_callback_t callback,
				 void *callback_data)
{
  int fallback_entry;

  callback->notify_booting (entry, callback_data);

  grub_menu_execute_entry (entry);

  /* Deal with fallback entries.  */
  while ((fallback_entry = get_and_remove_first_entry_number ("fallback"))
	 >= 0)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      entry = grub_menu_get_entry (menu, fallback_entry);
      callback->notify_fallback (entry, callback_data);
      grub_menu_execute_entry (entry);
      /* If the function call to execute the entry returns at all, then this is
	 taken to indicate a boot failure.  For menu entries that do something
	 other than actually boot an operating system, this could assume
	 incorrectly that something failed.  */
    }

  callback->notify_failure (callback_data);
}

static struct grub_menu_viewer *viewers;

static void
menu_set_chosen_entry (int entry)
{
  struct grub_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->set_chosen_entry (entry, cur->data);
}

static void
menu_print_timeout (int timeout)
{
  struct grub_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->print_timeout (timeout, cur->data);
}

static void
menu_fini (void)
{
  struct grub_menu_viewer *cur, *next;
  for (cur = viewers; cur; cur = next)
    {
      next = cur->next;
      cur->fini (cur->data);
      grub_free (cur);
    }
  viewers = NULL;
}

static void
menu_init (int entry, grub_menu_t menu, int nested)
{
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_err_t err;

    if (grub_gfxmenu_try_hook && grub_strcmp (term->name, "gfxterm") == 0)
      {
	err = grub_gfxmenu_try_hook (entry, menu, nested);
	if(!err)
	  continue;
	grub_print_error ();
	grub_errno = GRUB_ERR_NONE;
      }

    err = grub_menu_try_text (term, entry, menu, nested);
    if(!err)
      continue;
    grub_print_error ();
    grub_errno = GRUB_ERR_NONE;
  }
}

static void
clear_timeout (void)
{
  struct grub_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->clear_timeout (cur->data);
}

void
grub_menu_register_viewer (struct grub_menu_viewer *viewer)
{
  viewer->next = viewers;
  viewers = viewer;
}

/* Get the entry number from the variable NAME.  */
static int
get_entry_number (grub_menu_t menu, const char *name)
{
  char *val;
  int entry;

  val = grub_env_get (name);
  if (! val)
    return -1;

  grub_error_push ();

  entry = (int) grub_strtoul (val, 0, 0);

  if (grub_errno == GRUB_ERR_BAD_NUMBER)
    {
      /* See if the variable matches the title of a menu entry.  */
      grub_menu_entry_t e = menu->entry_list;
      int i;

      grub_errno = GRUB_ERR_NONE;

      for (i = 0; e; i++)
	{
	  if (grub_strcmp (e->title, val) == 0)
	    {
	      entry = i;
	      break;
	    }
	  e = e->next;
	}

      if (! e)
	entry = -1;
    }

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_errno = GRUB_ERR_NONE;
      entry = -1;
    }

  grub_error_pop ();

  return entry;
}

#define GRUB_MENU_PAGE_SIZE 10

/* Show the menu and handle menu entry selection.  Returns the menu entry
   index that should be executed or -1 if no entry should be executed (e.g.,
   Esc pressed to exit a sub-menu or switching menu viewers).
   If the return value is not -1, then *AUTO_BOOT is nonzero iff the menu
   entry to be executed is a result of an automatic default selection because
   of the timeout.  */
static int
run_menu (grub_menu_t menu, int nested, int *auto_boot)
{
  grub_uint64_t saved_time;
  int default_entry, current_entry;
  int timeout;

  default_entry = get_entry_number (menu, "default");

  /* If DEFAULT_ENTRY is not within the menu entries, fall back to
     the first entry.  */
  if (default_entry < 0 || default_entry >= menu->size)
    default_entry = 0;

  /* If timeout is 0, drawing is pointless (and ugly).  */
  if (grub_menu_get_timeout () == 0)
    {
      *auto_boot = 1;
      return default_entry;
    }

  current_entry = default_entry;

  /* Initialize the time.  */
  saved_time = grub_get_time_ms ();

 refresh:
  menu_init (current_entry, menu, nested);

  timeout = grub_menu_get_timeout ();

  if (timeout > 0)
    menu_print_timeout (timeout);
  else
    clear_timeout ();

  while (1)
    {
      int c;
      timeout = grub_menu_get_timeout ();

      if (grub_normal_exit_level)
	return -1;

      if (timeout > 0)
	{
	  grub_uint64_t current_time;

	  current_time = grub_get_time_ms ();
	  if (current_time - saved_time >= 1000)
	    {
	      timeout--;
	      grub_menu_set_timeout (timeout);
	      saved_time = current_time;
	      menu_print_timeout (timeout);
	    }
	}

      if (timeout == 0)
	{
	  grub_env_unset ("timeout");
          *auto_boot = 1;
	  menu_fini ();
	  return default_entry;
	}

      if (grub_checkkey () >= 0 || timeout < 0)
	{
	  c = GRUB_TERM_ASCII_CHAR (grub_getkey ());

	  if (timeout >= 0)
	    {
	      grub_env_unset ("timeout");
	      grub_env_unset ("fallback");
	      clear_timeout ();
	    }

	  switch (c)
	    {
	    case GRUB_TERM_HOME:
	      current_entry = 0;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case GRUB_TERM_END:
	      current_entry = menu->size - 1;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case GRUB_TERM_UP:
	    case '^':
	      if (current_entry > 0)
		current_entry--;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case GRUB_TERM_DOWN:
	    case 'v':
	      if (current_entry < menu->size - 1)
		current_entry++;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case GRUB_TERM_PPAGE:
	      if (current_entry < GRUB_MENU_PAGE_SIZE)
		current_entry = 0;
	      else
		current_entry -= GRUB_MENU_PAGE_SIZE;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case GRUB_TERM_NPAGE:
	      if (current_entry + GRUB_MENU_PAGE_SIZE < menu->size)
		current_entry += GRUB_MENU_PAGE_SIZE;
	      else
		current_entry = menu->size - 1;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case '\n':
	    case '\r':
	    case 6:
	      menu_fini ();
              *auto_boot = 0;
	      return current_entry;

	    case '\e':
	      if (nested)
		{
		  menu_fini ();
		  return -1;
		}
	      break;

	    case 'c':
	      menu_fini ();
	      grub_cmdline_run (1);
	      goto refresh;

	    case 'e':
	      menu_fini ();
		{
		  grub_menu_entry_t e = grub_menu_get_entry (menu, current_entry);
		  if (e)
		    grub_menu_entry_run (e);
		}
	      goto refresh;

	    default:
	      break;
	    }
	}
    }

  /* Never reach here.  */
  return -1;
}

/* Callback invoked immediately before a menu entry is executed.  */
static void
notify_booting (grub_menu_entry_t entry,
		void *userdata __attribute__((unused)))
{
  grub_printf ("  ");
  grub_printf_ (N_("Booting \'%s\'"), entry->title);
  grub_printf ("\n\n");
}

/* Callback invoked when a default menu entry executed because of a timeout
   has failed and an attempt will be made to execute the next fallback
   entry, ENTRY.  */
static void
notify_fallback (grub_menu_entry_t entry,
		 void *userdata __attribute__((unused)))
{
  grub_printf ("\n   ");
  grub_printf_ (N_("Falling back to \'%s\'"), entry->title);
  grub_printf ("\n\n");
  grub_millisleep (DEFAULT_ENTRY_ERROR_DELAY_MS);
}

/* Callback invoked when a menu entry has failed and there is no remaining
   fallback entry to attempt.  */
static void
notify_execution_failure (void *userdata __attribute__((unused)))
{
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }
  grub_printf ("\n  ");
  grub_printf_ (N_("Failed to boot both default and fallback entries.\n"));
  grub_wait_after_message ();
}

/* Callbacks used by the text menu to provide user feedback when menu entries
   are executed.  */
static struct grub_menu_execute_callback execution_callback =
{
  .notify_booting = notify_booting,
  .notify_fallback = notify_fallback,
  .notify_failure = notify_execution_failure
};

static grub_err_t
show_menu (grub_menu_t menu, int nested)
{
  while (1)
    {
      int boot_entry;
      grub_menu_entry_t e;
      int auto_boot;

      boot_entry = run_menu (menu, nested, &auto_boot);
      if (boot_entry < 0)
	break;

      e = grub_menu_get_entry (menu, boot_entry);
      if (! e)
	continue; /* Menu is empty.  */

      grub_cls ();

      if (auto_boot)
        {
          grub_menu_execute_with_fallback (menu, e, &execution_callback, 0);
        }
      else
        {
          grub_errno = GRUB_ERR_NONE;
          grub_menu_execute_entry (e);
          if (grub_errno != GRUB_ERR_NONE)
            {
              grub_print_error ();
              grub_errno = GRUB_ERR_NONE;
              grub_wait_after_message ();
            }
        }
    }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_show_menu (grub_menu_t menu, int nested)
{
  grub_err_t err1, err2;

  while (1)
    {
      err1 = show_menu (menu, nested);
      grub_print_error ();

      if (grub_normal_exit_level)
	break;

      err2 = grub_auth_check_authentication (NULL);
      if (err2)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}

      break;
    }

  return err1;
}
