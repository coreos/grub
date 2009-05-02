/* normal.h - prototypes for the normal mode */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_NORMAL_HEADER
#define GRUB_NORMAL_HEADER	1

#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/menu.h>
#include <grub/command.h>
#include <grub/file.h>

/* The maximum size of a command-line.  */
#define GRUB_MAX_CMDLINE	1600

/* The type of a completion item.  */
enum grub_completion_type
  {
    GRUB_COMPLETION_TYPE_COMMAND,
    GRUB_COMPLETION_TYPE_DEVICE,
    GRUB_COMPLETION_TYPE_PARTITION,
    GRUB_COMPLETION_TYPE_FILE,
    GRUB_COMPLETION_TYPE_ARGUMENT
  };
typedef enum grub_completion_type grub_completion_type_t;

extern struct grub_menu_viewer grub_normal_text_menu_viewer;

/* Callback structure menu viewers can use to provide user feedback when
   default entries are executed, possibly including fallback entries.  */
typedef struct grub_menu_execute_callback
{
  /* Called immediately before ENTRY is booted.  */
  void (*notify_booting) (grub_menu_entry_t entry, void *userdata);

  /* Called when executing one entry has failed, and another entry, ENTRY, will
     be executed as a fallback.  The implementation of this function should
     delay for a period of at least 2 seconds before returning in order to
     allow the user time to read the information before it can be lost by
     executing ENTRY.  */
  void (*notify_fallback) (grub_menu_entry_t entry, void *userdata);

  /* Called when an entry has failed to execute and there is no remaining
     fallback entry to attempt.  */
  void (*notify_failure) (void *userdata);
}
*grub_menu_execute_callback_t;

void grub_enter_normal_mode (const char *config);
void grub_normal_execute (const char *config, int nested, int batch);
void grub_menu_execute_with_fallback (grub_menu_t menu,
				      grub_menu_entry_t entry,
				      grub_menu_execute_callback_t callback,
				      void *callback_data);
void grub_menu_entry_run (grub_menu_entry_t entry);
void grub_menu_execute_entry(grub_menu_entry_t entry);
grub_menu_entry_t grub_menu_get_entry (grub_menu_t menu, int no);
int grub_menu_get_timeout (void);
void grub_menu_set_timeout (int timeout);
int grub_cmdline_get (const char *prompt, char cmdline[], unsigned max_len,
		      int echo_char, int readline);
grub_err_t grub_set_history (int newsize);
void grub_normal_init_page (void);
void grub_menu_init_page (int nested, int edit);
char *grub_normal_do_completion (char *buf, int *restore,
				 void (*hook) (const char *item, grub_completion_type_t type, int count));
grub_err_t grub_normal_print_device_info (const char *name);
char *grub_env_write_color_normal (struct grub_env_var *var, const char *val);
char *grub_env_write_color_highlight (struct grub_env_var *var, const char *val);
void grub_parse_color_name_pair (grub_uint8_t *ret, const char *name);
void grub_wait_after_message (void);

char *grub_file_getline (grub_file_t file);
void read_handler_list (void);
void free_handler_list (void);
void read_command_list (void);
void read_fs_list (void);
void grub_cmdline_run (int nested);
grub_err_t grub_menu_addentry (int argc, const char **args,
			       const char *sourcecode);

#ifdef GRUB_UTIL
void grub_normal_init (void);
void grub_normal_fini (void);
void grub_hello_init (void);
void grub_hello_fini (void);
void grub_ls_init (void);
void grub_ls_fini (void);
void grub_cat_init (void);
void grub_cat_fini (void);
void grub_boot_init (void);
void grub_boot_fini (void);
void grub_cmp_init (void);
void grub_cmp_fini (void);
void grub_terminal_init (void);
void grub_terminal_fini (void);
void grub_loop_init (void);
void grub_loop_fini (void);
void grub_help_init (void);
void grub_help_fini (void);
void grub_halt_init (void);
void grub_halt_fini (void);
void grub_reboot_init (void);
void grub_reboot_fini (void);
void grub_configfile_init (void);
void grub_configfile_fini (void);
void grub_search_init (void);
void grub_search_fini (void);
void grub_test_init (void);
void grub_test_fini (void);
void grub_blocklist_init (void);
void grub_blocklist_fini (void);
#endif

#endif /* ! GRUB_NORMAL_HEADER */
