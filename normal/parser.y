/* parser.y - The scripting parser.  */
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

%{
#include <grub/script.h>
#include <grub/mm.h>

#define YYFREE		grub_free
#define YYMALLOC	grub_malloc

/* Keep track of the memory allocated for this specific function.  */
static struct grub_script_mem *func_mem = 0;

static char *menu_entry = 0;

%}

%union {
  struct grub_script_cmd *cmd;
  struct grub_script_arglist *arglist;
  struct grub_script_arg *arg;
  char *string;
}

%token GRUB_PARSER_TOKEN_IF		"if"
%token GRUB_PARSER_TOKEN_WHILE		"while"
%token GRUB_PARSER_TOKEN_FUNCTION	"function"
%token GRUB_PARSER_TOKEN_MENUENTRY	"menuentry"
%token GRUB_PARSER_TOKEN_ELSE		"else"
%token GRUB_PARSER_TOKEN_THEN		"then"
%token GRUB_PARSER_TOKEN_FI		"fi"
%token GRUB_PARSER_TOKEN_NAME
%token GRUB_PARSER_TOKEN_VAR
%type <cmd> script grubcmd command commands menuentry if
%type <arglist> arguments;
%type <arg> argument;
%type <string> "if" "while" "function" "else" "then" "fi"
%type <string> text GRUB_PARSER_TOKEN_NAME GRUB_PARSER_TOKEN_VAR

%%
/* It should be possible to do this in a clean way...  */
script:		commands returns
		  {
		    grub_script_parsed = $1;
		  }
;

/* Some tokens are both used as token or as plain text.  XXX: Add all
   tokens without causing conflicts.  */
text:		GRUB_PARSER_TOKEN_NAME
		  {
		    $$ = $1;
		  }
		| "if"
		  {
		    $$ = $1;
		  }
		| "while"
		  {
		    $$ = $1;
		  }
;

ws:		/* Empty */
		| ' '
;

returns:	/* Empty */
		| '\n'
;

/* An argument can consist of some static text mixed with variables,
   for example: `foo${bar}baz'.  */
argument:	GRUB_PARSER_TOKEN_VAR
		  {
		    $$ = grub_script_arg_add (0, GRUB_SCRIPT_ARG_TYPE_VAR, $1);
		  }
		| text
		  {
		    $$ = grub_script_arg_add (0, GRUB_SCRIPT_ARG_TYPE_STR, $1);
		  }
		| argument GRUB_PARSER_TOKEN_VAR
		  {
		    $$ = grub_script_arg_add ($1, GRUB_SCRIPT_ARG_TYPE_VAR, $2);
		  }
		| argument text
		  {
		    $$ = grub_script_arg_add ($1, GRUB_SCRIPT_ARG_TYPE_STR, $2);
		  }
;

arguments:	argument
		  {
		    $$ = grub_script_add_arglist (0, $1);
		  }
		| arguments ' ' argument
		  {
		    $$ = grub_script_add_arglist ($1, $3);
		  }
;

grubcmd:	ws GRUB_PARSER_TOKEN_NAME ' ' arguments ws
		  {
		    $$ = grub_script_create_cmdline ($2, $4);
		  }
		| ws GRUB_PARSER_TOKEN_NAME ws
		  {
		    $$ = grub_script_create_cmdline ($2, 0);
		  }
;

/* A single command.  */
command:	grubcmd 	{ $$ = $1; }
		| if 		{ $$ = $1; }
		| function	{ $$ = 0;  }
		| menuentry	{ $$ = $1;  }
;

/* A block of commands.  */
commands:	command
		  { 
		    $$ = grub_script_add_cmd (0, $1);
		  }
		| commands ';' command
		  { 
		    struct grub_script_cmdblock *cmd;
		    cmd = (struct grub_script_cmdblock *) $1;
		    $$ = grub_script_add_cmd (cmd, $3);
		  }
		| commands '\n' command
		  { 
		    struct grub_script_cmdblock *cmd;
		    cmd = (struct grub_script_cmdblock *) $1;
		    $$ = grub_script_add_cmd (cmd, $3);
		  }
;

/* A function.  Carefully save the memory that is allocated.  */
function:	"function" ' ' GRUB_PARSER_TOKEN_NAME
		  { 
		    grub_script_lexer_ref ();
		  } ws '{' returns
		  { 
		    /* The first part of the function was recognised.
		       Now start recording the memory usage to store
		       this function.  */
		    func_mem = grub_script_mem_record ();
		  } commands returns '}'
		  {
		    struct grub_script *script;

		    /* All the memory usage for parsing this function
		       was recorded.  */
		    func_mem = grub_script_mem_record_stop (func_mem);
		    script = grub_script_create ($9, func_mem);
		    if (script)
		      grub_script_function_create ($3, script);
		    grub_script_lexer_deref ();
		  }
;

/* A menu entry.  Carefully save the memory that is allocated.  */
menuentry:	"menuentry" ' ' argument
		  { 
		    grub_script_lexer_ref ();
		  } ws '{' returns
		  { 
		    /* Record sourcecode of the menu entry.  It can be
		       parsed multiple times if it is part of a
		       loop.  */
		    grub_script_lexer_record_start ();
		  } commands returns '}'
		  {
		    menu_entry = grub_script_lexer_record_stop ();
		    $$ = grub_script_create_cmdmenu ($3, menu_entry, 0);
		    grub_script_lexer_deref ();
		  }
;

/* The first part of the if statement.  It's used to switch the lexer
   to a state in which it demands more tokens.  */
if_statement:	"if" { grub_script_lexer_ref (); }
;

/* The if statement.  */
if:		 if_statement grubcmd ';' ws "then" returns commands returns "fi"
		  {
		    $$ = grub_script_create_cmdif ($2, $7, 0);
		    grub_script_lexer_deref ();
		  }
		 | if_statement grubcmd ';' ws "then" returns commands returns "else" returns commands returns  "fi"
		  {
		    $$ = grub_script_create_cmdif ($2, $7, $11);
		    grub_script_lexer_deref ();
		  }
;
