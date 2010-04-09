/* parser.y - The scripting parser.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

%{
#include <grub/script_sh.h>
#include <grub/mm.h>

#define YYFREE          grub_free
#define YYMALLOC        grub_malloc
#define YYLTYPE_IS_TRIVIAL      0
#define YYENABLE_NLS    0

%}

%union {
  struct grub_script_cmd *cmd;
  struct grub_script_arglist *arglist;
  struct grub_script_arg *arg;
  char *string;
}

%token GRUB_PARSER_TOKEN_BAD
%token GRUB_PARSER_TOKEN_EOF 0 "end-of-input"

%token GRUB_PARSER_TOKEN_NEWLINE "\n"
%token GRUB_PARSER_TOKEN_AND     "&&"
%token GRUB_PARSER_TOKEN_OR      "||"
%token GRUB_PARSER_TOKEN_SEMI2   ";;"
%token GRUB_PARSER_TOKEN_PIPE    "|"
%token GRUB_PARSER_TOKEN_AMP     "&"
%token GRUB_PARSER_TOKEN_SEMI    ";"
%token GRUB_PARSER_TOKEN_LBR     "{"
%token GRUB_PARSER_TOKEN_RBR     "}"
%token GRUB_PARSER_TOKEN_NOT     "!"
%token GRUB_PARSER_TOKEN_LSQBR2  "["
%token GRUB_PARSER_TOKEN_RSQBR2  "]"
%token GRUB_PARSER_TOKEN_LT      "<"
%token GRUB_PARSER_TOKEN_GT      ">"

%token <arg> GRUB_PARSER_TOKEN_CASE      "case"
%token <arg> GRUB_PARSER_TOKEN_DO        "do"
%token <arg> GRUB_PARSER_TOKEN_DONE      "done"
%token <arg> GRUB_PARSER_TOKEN_ELIF      "elif"
%token <arg> GRUB_PARSER_TOKEN_ELSE      "else"
%token <arg> GRUB_PARSER_TOKEN_ESAC      "esac"
%token <arg> GRUB_PARSER_TOKEN_FI        "fi"
%token <arg> GRUB_PARSER_TOKEN_FOR       "for"
%token <arg> GRUB_PARSER_TOKEN_IF        "if"
%token <arg> GRUB_PARSER_TOKEN_IN        "in"
%token <arg> GRUB_PARSER_TOKEN_SELECT    "select"
%token <arg> GRUB_PARSER_TOKEN_THEN      "then"
%token <arg> GRUB_PARSER_TOKEN_UNTIL     "until"
%token <arg> GRUB_PARSER_TOKEN_WHILE     "while"
%token <arg> GRUB_PARSER_TOKEN_TIME      "time"
%token <arg> GRUB_PARSER_TOKEN_FUNCTION  "function"
%token <arg> GRUB_PARSER_TOKEN_MENUENTRY "menuentry"
%token <arg> GRUB_PARSER_TOKEN_NAME      "name"
%token <arg> GRUB_PARSER_TOKEN_WORD      "word"

%type <arglist> word argument arguments0 arguments1
%type <cmd> script_init script grubcmd ifclause ifcmd forcmd command
%type <cmd> commands1 menuentry statement

%pure-parser
%lex-param   { struct grub_parser_param *state };
%parse-param { struct grub_parser_param *state };

%start script_init

%%
/* It should be possible to do this in a clean way...  */
script_init: { state->err = 0; } script { state->parsed = $2; state->err = 0; }
;

script: newlines0
        {
          $$ = 0;
        }
      | script statement delimiter newlines0
        {
          struct grub_script_cmdblock *cmdblock;
          cmdblock = (struct grub_script_cmdblock *) $1;
          $$ = grub_script_add_cmd (state, cmdblock, $2);
        }
      | error
        {
          $$ = 0;
          yyerror (state, "Incorrect command");
          yyerrok;
        }
;

newlines0: /* Empty */ | newlines1 ;
newlines1: newlines0 "\n" ;

delimiter: ";"
         | "\n"
;
delimiters0: /* Empty */ | delimiters1 ;
delimiters1: delimiter
          | delimiters1 "\n"
;

word: GRUB_PARSER_TOKEN_NAME { $$ = grub_script_add_arglist (state, 0, $1); }
    | GRUB_PARSER_TOKEN_WORD { $$ = grub_script_add_arglist (state, 0, $1); }
;

statement: command   { $$ = $1; }
         | function  { $$ = 0;  }
         | menuentry { $$ = $1; }

argument : "case"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "do"        { $$ = grub_script_add_arglist (state, 0, $1); }
         | "done"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "elif"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "else"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "esac"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "fi"        { $$ = grub_script_add_arglist (state, 0, $1); }
         | "for"       { $$ = grub_script_add_arglist (state, 0, $1); }
         | "if"        { $$ = grub_script_add_arglist (state, 0, $1); }
         | "in"        { $$ = grub_script_add_arglist (state, 0, $1); }
         | "select"    { $$ = grub_script_add_arglist (state, 0, $1); }
         | "then"      { $$ = grub_script_add_arglist (state, 0, $1); }
         | "until"     { $$ = grub_script_add_arglist (state, 0, $1); }
         | "while"     { $$ = grub_script_add_arglist (state, 0, $1); }
         | "function"  { $$ = grub_script_add_arglist (state, 0, $1); }
         | "menuentry" { $$ = grub_script_add_arglist (state, 0, $1); }
         | word { $$ = $1; }
;

arguments0: /* Empty */ { $$ = 0; }
          | arguments1  { $$ = $1; }
;
arguments1: argument arguments0
            {
	      if ($1 && $2)
		{
		  $1->next = $2;
		  $1->argcount += $2->argcount;
		  $2->argcount = 0;
		}
	      $$ = $1;
            }
;

grubcmd: word arguments0
         {
           if ($1 && $2) {
             $1->next = $2;
             $1->argcount += $2->argcount;
             $2->argcount = 0;
           }
           $$ = grub_script_create_cmdline (state, $1);
         }
;

/* A single command.  */
command: grubcmd { $$ = $1; }
       | ifcmd   { $$ = $1; }
       | forcmd  { $$ = $1; }
;

/* A list of commands. */
commands1: newlines0 command
           {
             $$ = grub_script_add_cmd (state, 0, $2);
           }
         | commands1 delimiters1 command
           {
             struct grub_script_cmdblock *cmdblock;
	     cmdblock = (struct grub_script_cmdblock *) $1;
	     $$ = grub_script_add_cmd (state, cmdblock, $3);
           }
;

function: "function" "name" 
          {
            grub_script_lexer_ref (state->lexerstate);
            state->func_mem = grub_script_mem_record (state);
          }
          delimiters0 "{" commands1 delimiters1 "}"
          {
            struct grub_script *script;
            state->func_mem = grub_script_mem_record_stop (state,
                                                           state->func_mem);
            script = grub_script_create ($6, state->func_mem);
            if (script)
              grub_script_function_create ($2, script);

            grub_script_lexer_deref (state->lexerstate);
          }
;

menuentry: "menuentry"
           {
             grub_script_lexer_ref (state->lexerstate);
           }
           arguments1
           {
             grub_script_lexer_record_start (state);
           }
           delimiters0 "{" commands1 delimiters1 "}"
           {
             char *menu_entry;
             menu_entry = grub_script_lexer_record_stop (state);
             grub_script_lexer_deref (state->lexerstate);
             $$ = grub_script_create_cmdmenu (state, $3, menu_entry, 0);
           }
;

ifcmd: "if"
	{
	  grub_script_lexer_ref (state->lexerstate);
	}
	ifclause "fi"
	{
	  $$ = $3;
	  grub_script_lexer_deref (state->lexerstate);
	}
;
ifclause: commands1 delimiters1 "then" commands1 delimiters1
	  {
	    $$ = grub_script_create_cmdif (state, $1, $4, 0);
	  }
	| commands1 delimiters1 "then" commands1 delimiters1 "else" commands1 delimiters1
	  {
	    $$ = grub_script_create_cmdif (state, $1, $4, $7);
	  }
	| commands1 delimiters1 "then" commands1 delimiters1 "elif" ifclause
	  {
	    $$ = grub_script_create_cmdif (state, $1, $4, $7);
	  }
;

forcmd: "for" "name"
        {
	  grub_script_lexer_ref (state->lexerstate);
        }
        "in" arguments0 delimiters1 "do" commands1 delimiters1 "done"
	{
	  $$ = grub_script_create_cmdfor (state, $2, $5, $8);
	  grub_script_lexer_deref (state->lexerstate);
	}
;
