%{
/* yylex.l  The scripting lexer.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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

#include <grub/parser.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/script_sh.h>
#include "grub_script.tab.h"

#define yyfree    grub_lexer_yyfree
#define yyalloc   grub_lexer_yyalloc
#define yyrealloc grub_lexer_yyrealloc

/* 
 * As we don't have access to yyscanner, we cannot do much except to
 * print the fatal error.
 */
#define YY_FATAL_ERROR(msg)                     \
  do {                                          \
    grub_printf ("fatal error: %s\n", msg);     \
  } while (0)

#define COPY(str, hint)                         \
  do {                                          \
    copy_string (yyextra, str, hint);           \
  } while (0)


#define RECORD                                  \
  do {                                          \
    grub_script_lexer_record (yyextra, yytext); \
  } while (0)

#define ARG(t)                        \
  do {                                \
    yyextra->lexerstate->type = t;    \
    return GRUB_PARSER_TOKEN_WORD;    \
  } while (0)

/* We don't need YY_INPUT, as we rely on yy_scan_strings */
#define YY_INPUT(buf,res,max) do { res = 0; } while (0)

/* forward declarations */
static void  grub_lexer_yyfree (void *, yyscan_t yyscanner);
static void* grub_lexer_yyalloc (yy_size_t, yyscan_t yyscanner);
static void* grub_lexer_yyrealloc (void*, yy_size_t, yyscan_t yyscanner);
static void  copy_string (struct grub_parser_param *, const char *,
                          unsigned hint);

%}

%top{

#include <sys/types.h>

typedef size_t yy_size_t;
#define YY_TYPEDEF_YY_SIZE_T 1

/* 
 * Some flex hacks for -nostdinc; XXX We need to fix these when libc
 * support becomes availble in GRUB.
 */

#ifndef GRUB_UTIL
#define stdin  0
#define stdout 0

#define fprintf(...) 0
#define exit(...)
#endif

#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic warning "-Wunused-parameter"
#pragma GCC diagnostic warning "-Wstrict-prototypes"
#pragma GCC diagnostic warning "-Wmissing-prototypes"

}

%option ecs
%option meta-ecs

%option warn
%option array
%option stack
%option reentrant
%option bison-bridge
%option never-interactive

%option noyyfree noyyalloc noyyrealloc
%option nounistd nostdinit nodefault noyylineno noyywrap

/* Reduce lexer size, by not defining these.  */
%option noyy_top_state
%option noinput nounput
%option noyyget_in noyyset_in
%option noyyget_out noyyset_out
%option noyyget_debug noyyset_debug
%option noyyget_lineno noyyset_lineno

%option extra-type="struct grub_parser_param*"

BLANK           [ \t]
COMMENT         ^[ \t]*#.*$

CHAR            [^|&$;<> \t\n\'\"\\]
DIGITS          [[:digit:]]+
NAME            [[:alpha:]_][[:alnum:][:digit:]_]*

ESC             \\.
VARIABLE        ${NAME}|$\{{NAME}\}|${DIGITS}|$\{{DIGITS}\}|$\?|$\{\?\}
DQSTR           \"([^\\\"]|{ESC})*\"
SQSTR           \'[^\']*\'
WORD            ({CHAR}|{DQSTR}|{SQSTR}|{ESC}|{VARIABLE})+

%x              SPLIT
%x              DQUOTE
%x              SQUOTE
%x              VAR

%%

 /* White spaces */
{BLANK}+        { RECORD; }
{COMMENT}       { RECORD; }

 /* Special symbols */
"\n"            { RECORD; return GRUB_PARSER_TOKEN_NEWLINE; }
"||"            { RECORD; return GRUB_PARSER_TOKEN_OR;      }
"&&"            { RECORD; return GRUB_PARSER_TOKEN_AND;     }
";;"            { RECORD; return GRUB_PARSER_TOKEN_SEMI2;   }
"|"             { RECORD; return GRUB_PARSER_TOKEN_PIPE;    }
"&"             { RECORD; return GRUB_PARSER_TOKEN_AMP;     }
";"             { RECORD; return GRUB_PARSER_TOKEN_SEMI;    }
"<"             { RECORD; return GRUB_PARSER_TOKEN_LT;      }
">"             { RECORD; return GRUB_PARSER_TOKEN_GT;      }

 /* Reserved words */
"!"             { RECORD; return GRUB_PARSER_TOKEN_NOT;       }
"{"             { RECORD; return GRUB_PARSER_TOKEN_LBR;       }
"}"             { RECORD; return GRUB_PARSER_TOKEN_RBR;       }
"[["            { RECORD; return GRUB_PARSER_TOKEN_RSQBR2;    }
"]]"            { RECORD; return GRUB_PARSER_TOKEN_LSQBR2;    }
"time"          { RECORD; return GRUB_PARSER_TOKEN_TIME;      }
"case"          { RECORD; return GRUB_PARSER_TOKEN_CASE;      }
"do"            { RECORD; return GRUB_PARSER_TOKEN_DO;        }
"done"          { RECORD; return GRUB_PARSER_TOKEN_DONE;      }
"elif"          { RECORD; return GRUB_PARSER_TOKEN_ELIF;      }
"else"          { RECORD; return GRUB_PARSER_TOKEN_ELSE;      }
"esac"          { RECORD; return GRUB_PARSER_TOKEN_ESAC;      }
"fi"            { RECORD; return GRUB_PARSER_TOKEN_FI;        }
"for"           { RECORD; return GRUB_PARSER_TOKEN_FOR;       }
"if"            { RECORD; return GRUB_PARSER_TOKEN_IF;        }
"in"            { RECORD; return GRUB_PARSER_TOKEN_IN;        }
"select"        { RECORD; return GRUB_PARSER_TOKEN_SELECT;    }
"then"          { RECORD; return GRUB_PARSER_TOKEN_THEN;      }
"until"         { RECORD; return GRUB_PARSER_TOKEN_UNTIL;     }
"while"         { RECORD; return GRUB_PARSER_TOKEN_WHILE;     }
"function"      { RECORD; return GRUB_PARSER_TOKEN_FUNCTION;  }
"menuentry"     { RECORD; return GRUB_PARSER_TOKEN_MENUENTRY; }

{NAME}          { RECORD; return GRUB_PARSER_TOKEN_NAME; }
{WORD}          {
                  RECORD;
                  /* resplit yytext */
		  grub_dprintf ("lexer", "word: [%s]\n", yytext);
                  yypush_buffer_state (YY_CURRENT_BUFFER, yyscanner);
                  if (yy_scan_string (yytext, yyscanner))
                    {
                      yyextra->lexerstate->merge_start = 1;
                      yy_push_state (SPLIT, yyscanner);
                    }
                  else
                    {
                      grub_script_yyerror (yyextra, 0);
                      yypop_buffer_state (yyscanner);
                      return GRUB_PARSER_TOKEN_WORD;
                    }
                }

.|\n            {
                  grub_script_yyerror (yyextra, "unrecognized token");
                  return GRUB_PARSER_TOKEN_BAD;
                }

 /* Split word into multiple args */

<SPLIT>{
  \\.           { COPY (yytext + 1, yyleng - 1); }
  \"            {
                  yy_push_state (DQUOTE, yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_TEXT);
                }
  \'            {
                  yy_push_state (SQUOTE, yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_TEXT);
                }
  \$            {
                  yy_push_state (VAR, yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_TEXT);
                }
  \\            |
  [^\"\'$\\]+   { COPY (yytext, yyleng); }
  <<EOF>>       {
                  yy_pop_state (yyscanner);
                  yypop_buffer_state (yyscanner);
                  yyextra->lexerstate->merge_end = 1;
                  ARG (GRUB_SCRIPT_ARG_TYPE_TEXT);
                }
}

<VAR>{
  \?            |
  {DIGITS}      |
  {NAME}        {
                  COPY (yytext, yyleng);
                  yy_pop_state (yyscanner);
                  if (YY_START == SPLIT)
                    ARG (GRUB_SCRIPT_ARG_TYPE_VAR);
                  else
                    ARG (GRUB_SCRIPT_ARG_TYPE_DQVAR);
                }
  \{\?\}        |
  \{{DIGITS}\}  |
  \{{NAME}\}    {
                  yytext[yyleng - 1] = '\0';
                  COPY (yytext + 1, yyleng - 2);
                  yy_pop_state (yyscanner);
                  if (YY_START == SPLIT)
                    ARG (GRUB_SCRIPT_ARG_TYPE_VAR);
                  else
                    ARG (GRUB_SCRIPT_ARG_TYPE_DQVAR);
                }
  .|\n          { return GRUB_PARSER_TOKEN_BAD; }
}

<SQUOTE>{
  \'            {
                  yy_pop_state (yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_SQSTR);
                }
  [^\']+        { COPY (yytext, yyleng); }
}

<DQUOTE>{
  \"            {
                  yy_pop_state (yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_DQSTR);
                }
  \$            {
                  yy_push_state (VAR, yyscanner);
                  ARG (GRUB_SCRIPT_ARG_TYPE_DQSTR);
                }
  \\\\          { COPY ("\\", 1); }
  \\\"          { COPY ("\"", 1); }
  \\\n          { /* ignore */ }
  [^\"$\\\n]+   { COPY (yytext, yyleng); }
  (.|\n)        { COPY (yytext, yyleng); }
}

<<EOF>>         {
                  yypop_buffer_state (yyscanner);
                  if (! grub_script_lexer_yywrap (yyextra))
                    {
                      yyextra->lexerstate->eof = 1;
                      return GRUB_PARSER_TOKEN_EOF;
                    }
                }

%%

static void
grub_lexer_yyfree (void *ptr, yyscan_t yyscanner __attribute__ ((unused)))
{
  grub_free(ptr);
}

static void*
grub_lexer_yyalloc (yy_size_t size, yyscan_t yyscanner __attribute__ ((unused)))
{
  return grub_malloc (size);
}

static void*
grub_lexer_yyrealloc (void *ptr, yy_size_t size,
                      yyscan_t yyscanner __attribute__ ((unused)))
{
  return grub_realloc (ptr, size);
}

#define MAX(a,b) ((a) < (b) ? (b) : (a))

static void copy_string (struct grub_parser_param *parser, const char *str, unsigned hint)
{
  int size;
  char *ptr;
  unsigned len;

  len = hint ? hint : grub_strlen (str);
  if (parser->lexerstate->used + len >= parser->lexerstate->size)
    {
      size = MAX (len, parser->lexerstate->size) * 2;
      ptr = grub_realloc (parser->lexerstate->text, size);
      if (!ptr)
        {
          grub_script_yyerror (parser, 0);
          return;
        }

      parser->lexerstate->text = ptr;
      parser->lexerstate->size = size;
    }
  grub_strcpy (parser->lexerstate->text + parser->lexerstate->used - 1, str);
  parser->lexerstate->used += len;
}
