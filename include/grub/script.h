/* script.h  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/err.h>

struct grub_script_mem;

/* The generic header for each scripting command or structure.  */
struct grub_script_cmd
{
  /* This function is called to execute the command.  */
  grub_err_t (*exec) (struct grub_script_cmd *cmd);

  /* The next command.  This can be used by the parent to form a chain
     of commands.  */
  struct grub_script_cmd *next;
};

struct grub_script
{
  struct grub_script_mem *mem;
  struct grub_script_cmd *cmd;
};

typedef enum
{
  GRUB_SCRIPT_ARG_TYPE_STR,
  GRUB_SCRIPT_ARG_TYPE_VAR
} grub_script_arg_type_t;

/* A part of an argument.  */
struct grub_script_arg
{
  grub_script_arg_type_t type;

  char *str;

  /* Next argument part.  */
  struct grub_script_arg *next;
};

/* A complete argument.  It consists of a list of one or more `struct
   grub_script_arg's.  */
struct grub_script_arglist
{
  struct grub_script_arglist *next;
  struct grub_script_arg *arg;
  /* Only stored in the first link.  */
  int argcount;
};

/* A single command line.  */
struct grub_script_cmdline
{
  struct grub_script_cmd cmd;

  /* The arguments for this command.  */
  struct grub_script_arglist *arglist;

  /* The command name of this command.  XXX: Perhaps an argument
     should be used for this so we can use variables as command
     name.  */
  char *cmdname;
};

/* A block of commands, this can be used to group commands.  */
struct grub_script_cmdblock
{
  struct grub_script_cmd cmd;

  /* A chain of commands.  */
  struct grub_script_cmd *cmdlist;
};

/* An if statement.  */
struct grub_script_cmdif
{
  struct grub_script_cmd cmd;

  /* The command used to check if the if is true or false.  */
  struct grub_script_cmd *bool;

  /* The code executed in case the result if bool was true.  */
  struct grub_script_cmd *true;

  /* The code executed in case the result if bool was false.  */
  struct grub_script_cmd *false;
};

struct grub_script_arglist *
grub_script_create_arglist (void);

struct grub_script_arglist *
grub_script_add_arglist (struct grub_script_arglist *list,
			 struct grub_script_arg *arg);
struct grub_script_cmd *
grub_script_create_cmdline (char *cmdname,
			    struct grub_script_arglist *arglist);
struct grub_script_cmd *
grub_script_create_cmdblock (void);

struct grub_script_cmd *
grub_script_create_cmdif (struct grub_script_cmd *bool,
			  struct grub_script_cmd *true,
			  struct grub_script_cmd *false);
struct grub_script_cmd *
grub_script_add_cmd (struct grub_script_cmdblock *cmdblock,
		     struct grub_script_cmd *cmd);
struct grub_script_arg *
grub_script_arg_add (struct grub_script_arg *arg,
		     grub_script_arg_type_t type, char *str);

struct grub_script *grub_script_parse (char *script,
				       grub_err_t (*getline) (char **));
void grub_script_free (struct grub_script *script);
struct grub_script *grub_script_create (struct grub_script_cmd *cmd,
					struct grub_script_mem *mem);

void grub_script_lexer_init (char *s, grub_err_t (*getline) (char **));
void grub_script_lexer_ref (void);
void grub_script_lexer_deref (void);

/* Functions to track allocated memory.  */
void *grub_script_malloc (grub_size_t size);
struct grub_script_mem *grub_script_mem_record (void);
struct grub_script_mem *grub_script_mem_record_stop (struct grub_script_mem *restore);

/* Functions used by bison.  */
int grub_script_yylex (void);
int grub_script_yyparse (void);
void grub_script_yyerror (char const *err);

/* Commands to execute, don't use these directly.  */
grub_err_t grub_script_execute_cmdline (struct grub_script_cmd *cmd);
grub_err_t grub_script_execute_cmdblock (struct grub_script_cmd *cmd);
grub_err_t grub_script_execute_cmdif (struct grub_script_cmd *cmd);

/* Execute any GRUB pre-parsed command or script.  */
grub_err_t grub_script_execute (struct grub_script *script);

/* This variable points to the parsed command.  This is used to
   communicate with the bison code.  */
extern struct grub_script_cmd *grub_script_parsed;



/* The function description.  */
struct grub_script_function
{
  /* The name.  */
  char *name;

  /* The script function.  */
  struct grub_script *func;

  /* The flags.  */
  unsigned flags;

  /* The next element.  */
  struct grub_script_function *next;

  int references;
};
typedef struct grub_script_function *grub_script_function_t;

grub_script_function_t grub_script_function_create (char *functionname,
						    struct grub_script *cmd);
void grub_script_function_remove (const char *name);
grub_script_function_t grub_script_function_find (char *functionname);
int grub_script_function_iterate (int (*iterate) (grub_script_function_t));
int grub_script_function_call (grub_script_function_t func,
			       int argc, char **args);
