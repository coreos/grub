/* lexer.c - The scripting lexer.  */
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

#include <grub/parser.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/script_sh.h>

#include "grub_script.tab.h"
#include "grub_script.yy.h"

void
grub_script_lexer_ref (struct grub_lexer_param *state)
{
  state->refs++;
}

void
grub_script_lexer_deref (struct grub_lexer_param *state)
{
  state->refs--;
}

/* Start recording all characters passing through the lexer.  */
void
grub_script_lexer_record_start (struct grub_parser_param *parser)
{
  struct grub_lexer_param *lexer = parser->lexerstate;

  lexer->record = 1;
  lexer->recordpos = 0;
  if (lexer->recording) /* reuse last record */
    return;

  lexer->recordlen = GRUB_LEXER_INITIAL_RECORD_SIZE;
  lexer->recording = grub_malloc (lexer->recordlen);
  if (!lexer->recording)
    {
      grub_script_yyerror (parser, 0);
      lexer->record = 0;
      lexer->recordlen = 0;
    }
}

char *
grub_script_lexer_record_stop (struct grub_parser_param *parser)
{
  char *ptr;
  char *result;
  struct grub_lexer_param *lexer = parser->lexerstate;

  auto char *compact (char *start, char *end);
  char *compact (char *start, char *end)
  {
    /* Delete '{' and '}' characters and whitespaces.  */
    while (*start && grub_isspace (*start)) start++;
    if (*start == '{') start++;
    while (*start && grub_isspace (*start)) start++;

    while (*end && grub_isspace (*end)) end--;
    if (*end == '}') end--;
    while (*end && grub_isspace (*end)) end--;
    end[1] = '\0';

    return start;
  }

  if (!lexer->record || !lexer->recording)
    return 0;

  /* XXX This is not necessary in BASH.  */

  ptr = compact (lexer->recording, lexer->recording + lexer->recordpos - 1);
  lexer->record = 0;
  lexer->recordpos = 0;

  /* This memory would be freed by, grub_script_free.  */
  result = grub_script_malloc (parser, grub_strlen (ptr) + 1);
  if (result)
    grub_strcpy (result, ptr);

  return result;
}

#define MAX(a,b) ((a) < (b) ? (b) : (a))

/* Record STR if input recording is enabled.  */
void
grub_script_lexer_record (struct grub_parser_param *parser, char *str)
{
  int len;
  char *old;
  struct grub_lexer_param *lexer = parser->lexerstate;

  if (!lexer->record)
    return;

  len = grub_strlen (str);
  if (lexer->recordpos + len + 1 > lexer->recordlen)
    {
      old = lexer->recording;
      lexer->recordlen = MAX (len, lexer->recordlen) * 2;
      lexer->recording = grub_realloc (lexer->recording, lexer->recordlen);
      if (!lexer->recording)
	{
	  grub_free (old);
	  lexer->record = 0;
	  lexer->recordpos = 0;
	  lexer->recordlen /= 2;
	  grub_script_yyerror (parser, 0);
	  return;
	}
    }
  grub_strcpy (lexer->recording + lexer->recordpos, str);
  lexer->recordpos += len;
}

/* Append '\n' to SRC, before '\0'  */
static char *
append_newline (const char *src)
{
  char *line;
  grub_size_t len;

  len = grub_strlen (src);
  line = grub_malloc (len + 2);
  if (!line)
    return 0;

  grub_strcpy (line, src);

  line[len] = '\n';
  line[len + 1] = '\0';
  return line;
}

/* Read next line of input if necessary, and set yyscanner buffers.  */
int
grub_script_lexer_yywrap (struct grub_parser_param *parserstate)
{
  int len;
  char *line;
  char *line2;
  YY_BUFFER_STATE buffer;
  struct grub_lexer_param *lexerstate = parserstate->lexerstate;

  if (!lexerstate->refs)
    return 0;

  if (!lexerstate->getline)
    {
      grub_script_yyerror (parserstate, "unexpected end of file");
      return 0;
    }

  line = 0;
  buffer = 0;
  lexerstate->getline (&line, 1);
  if (!line)
    {
      grub_script_yyerror (parserstate, 0); /* XXX this could be for ^C case? */
      return 0;
    }

  len = grub_strlen (line);
  if (line[len - 1] == '\n')
    {
      buffer = yy_scan_string (line, lexerstate->yyscanner);
    }
  else
    {
      line2 = append_newline (line);
      if (line2)
	{
	  buffer = yy_scan_string (line2, lexerstate->yyscanner);
	  grub_free (line2);
	}
    }

  grub_free (line);
  if (!buffer)
    {
      grub_script_yyerror (parserstate, 0);
      return 0;
    }

  return 1;
}

struct grub_lexer_param *
grub_script_lexer_init (struct grub_parser_param *parser, char *script,
			grub_reader_getline_t getline)
{
  int len;
  char *script2;
  YY_BUFFER_STATE buffer;
  struct grub_lexer_param *lexerstate;

  lexerstate = grub_zalloc (sizeof (*lexerstate));
  if (!lexerstate)
    return 0;

  lexerstate->size = GRUB_LEXER_INITIAL_TEXT_SIZE;
  lexerstate->text = grub_malloc (lexerstate->size);
  if (!lexerstate->text)
    {
      grub_free (lexerstate);
      return 0;
    }

  lexerstate->getline = getline;	/* rest are all zeros already */
  if (yylex_init (&lexerstate->yyscanner))
    {
      grub_free (lexerstate->text);
      grub_free (lexerstate);
      return 0;
    }

  buffer = 0;
  script = script ? : "\n";
  len = grub_strlen (script);

  if (script[len - 1] == '\n')
    {
      buffer = yy_scan_string (script, lexerstate->yyscanner);
    }
  else
    {
      script2 = append_newline (script);
      if (script2)
	{
	  buffer = yy_scan_string (script2, lexerstate->yyscanner);
	  grub_free (script2);
	}
    }

  if (!buffer)
    {
      yylex_destroy (lexerstate->yyscanner);
      grub_free (lexerstate->yyscanner);

      grub_free (lexerstate->text);
      grub_free (lexerstate);
      return 0;
    }
  yyset_extra (parser, lexerstate->yyscanner);

  return lexerstate;
}

void
grub_script_lexer_fini (struct grub_lexer_param *lexerstate)
{
  if (!lexerstate)
    return;

  yylex_destroy (lexerstate->yyscanner);

  grub_free (lexerstate->recording);
  grub_free (lexerstate->text);
  grub_free (lexerstate);
}

int
grub_script_yylex (union YYSTYPE *value,
		   struct grub_parser_param *parserstate)
{
  char *str;
  int token;
  grub_script_arg_type_t type;
  struct grub_lexer_param *lexerstate = parserstate->lexerstate;

  value->arg = 0;
  if (parserstate->err)
    return GRUB_PARSER_TOKEN_BAD;

  if (lexerstate->eof)
    return GRUB_PARSER_TOKEN_EOF;

  /* 
   * Words with environment variables, like foo${bar}baz needs
   * multiple tokens to be merged into a single grub_script_arg.  We
   * use two variables to achieve this: lexerstate->merge_start and
   * lexerstate->merge_end
   */

  lexerstate->merge_start = 0;
  lexerstate->merge_end = 0;
  do
    {
      /* Empty lexerstate->text.  */
      lexerstate->used = 1;
      lexerstate->text[0] = '\0';

      token = yylex (value, lexerstate->yyscanner);
      if (token == GRUB_PARSER_TOKEN_BAD)
	break;

      /* Merging feature uses lexerstate->text instead of yytext.  */
      if (lexerstate->merge_start)
	{
	  str = lexerstate->text;
	  type = lexerstate->type;
	}
      else
	{
	  str = yyget_text (lexerstate->yyscanner);
	  type = GRUB_SCRIPT_ARG_TYPE_TEXT;
	}
      grub_dprintf("lexer", "token %u text [%s]\n", token, str);

      value->arg = grub_script_arg_add (parserstate, value->arg, type, str);
    }
  while (lexerstate->merge_start && !lexerstate->merge_end);

  if (!value->arg || parserstate->err)
    return GRUB_PARSER_TOKEN_BAD;

  return token;
}

void
grub_script_yyerror (struct grub_parser_param *state, char const *err)
{
  if (err)
    grub_error (GRUB_ERR_INVALID_COMMAND, err);

  grub_print_error ();
  state->err++;
}
