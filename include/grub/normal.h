/* normal.h - prototypes for the normal mode */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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

#ifndef PUPA_NORMAL_HEADER
#define PUPA_NORMAL_HEADER	1

#include <pupa/setjmp.h>
#include <pupa/symbol.h>
#include <pupa/err.h>

/* The maximum size of a command-line.  */
#define PUPA_MAX_CMDLINE	1600

/* Can be run in the command-line.  */
#define PUPA_COMMAND_FLAG_CMDLINE	0x1
/* Can be run in the menu.  */
#define PUPA_COMMAND_FLAG_MENU		0x2
/* Can be run in both interfaces.  */
#define PUPA_COMMAND_FLAG_BOTH		0x3
/* Only for the command title.  */
#define PUPA_COMMAND_FLAG_TITLE		0x4
/* Don't print the command on booting.  */
#define PUPA_COMMAND_FLAG_NO_ECHO	0x8

/* The command description.  */
struct pupa_command
{
  /* The name.  */
  const char *name;

  /* The callback function.  */
  int (*func) (int argc, char *argv[]);

  /* The flags.  */
  unsigned flags;

  /* The summary of the command usage.  */
  const char *summary;

  /* The description of the command.  */
  const char *description;

  /* The next element.  */
  struct pupa_command *next;
};
typedef struct pupa_command *pupa_command_t;

/* The command list.  */
struct pupa_command_list
{
  /* The string of a command.  */
  const char *command;

  /* The next element.  */
  struct pupa_command_list *next;
};
typedef struct pupa_command_list *pupa_command_list_t;

/* The menu entry.  */
struct pupa_menu_entry
{
  /* The title name.  */
  const char *title;

  /* The number of commands.  */
  int num;

  /* The list of commands.  */
  pupa_command_list_t command_list;

  /* The next element.  */
  struct pupa_menu_entry *next;
};
typedef struct pupa_menu_entry *pupa_menu_entry_t;

/* The menu.  */
struct pupa_menu
{
  /* The default entry number.  */
  int default_entry;

  /* The fallback entry number.  */
  int fallback_entry;

  /* The timeout to boot the default entry automatically.  */
  int timeout;
  
  /* The size of a menu.  */
  int size;

  /* The list of menu entries.  */
  pupa_menu_entry_t entry_list;
};
typedef struct pupa_menu *pupa_menu_t;

/* To exit from the normal mode.  */
extern pupa_jmp_buf pupa_exit_env;

void pupa_enter_normal_mode (const char *config);
void pupa_normal_execute (const char *config, int nested);
void pupa_menu_run (pupa_menu_t menu, int nested);
void pupa_cmdline_run (int nested);
int pupa_cmdline_get (const char *prompt, char cmdline[], unsigned max_len,
		      int echo_char, int readline);
void EXPORT_FUNC(pupa_register_command) (const char *name,
			    int (*func) (int argc, char *argv[]),
			    unsigned flags,
			    const char *summary,
			    const char *description);
void EXPORT_FUNC(pupa_unregister_command) (const char *name);
pupa_command_t pupa_command_find (char *cmdline);
pupa_err_t pupa_set_history (int newsize);
int pupa_iterate_commands (int (*iterate) (pupa_command_t));
int pupa_command_execute (char *cmdline);
void pupa_command_init (void);
void pupa_normal_init_page (void);

#ifdef PUPA_UTIL
void pupa_normal_init (void);
void pupa_normal_fini (void);
#endif

#endif /* ! PUPA_NORMAL_HEADER */
