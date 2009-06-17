/* lexer.c - The scripting lexer.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

static int
check_varstate (grub_parser_state_t state)
{
  return (state == GRUB_PARSER_STATE_VARNAME
	  || state == GRUB_PARSER_STATE_VAR
	  || state == GRUB_PARSER_STATE_QVAR
	  || state == GRUB_PARSER_STATE_VARNAME2
	  || state == GRUB_PARSER_STATE_QVARNAME
	  || state == GRUB_PARSER_STATE_QVARNAME2);
}

static int
check_textstate (grub_parser_state_t state)
{
  return (state == GRUB_PARSER_STATE_TEXT
	  || state == GRUB_PARSER_STATE_ESC
	  || state == GRUB_PARSER_STATE_QUOTE
	  || state == GRUB_PARSER_STATE_DQUOTE);
}

struct grub_lexer_param *
grub_script_lexer_init (char *script, grub_reader_getline_t getline)
{
  struct grub_lexer_param *param;

  param = grub_malloc (sizeof (*param));
  if (! param)
    return 0;

  param->state = GRUB_PARSER_STATE_TEXT;
  param->getline = getline;
  param->refs = 0;
  param->done = 0;
  param->newscript = 0;
  param->script = script;
  param->record = 0;
  param->recording = 0;
  param->recordpos = 0;
  param->recordlen = 0;
  param->tokenonhold = 0;
  param->was_newline = 0;

  return param;
}

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
grub_script_lexer_record_start (struct grub_lexer_param *state)
{
  state->record = 1;
  state->recordlen = 100;
  state->recording = grub_malloc (state->recordlen);
  state->recordpos = 0;
}

char *
grub_script_lexer_record_stop (struct grub_lexer_param *state)
{
  state->record = 0;

  /* Delete the last character, it is a `}'.  */
  if (state->recordpos > 0)
    {
      if (state->recording[--state->recordpos] != '}')
	{
	  grub_printf ("Internal error while parsing menu entry");
	  for (;;); /* XXX */
	}
      state->recording[state->recordpos] = '\0';
    }

  return state->recording;
}

/* When recording is enabled, record the character C as the next item
   in the character stream.  */
static void
recordchar (struct grub_lexer_param *state, char c)
{
  if (state->recordpos == state->recordlen)
    {
      char *old = state->recording;
      state->recordlen += 100;
      state->recording = grub_realloc (state->recording, state->recordlen);
      if (! state->recording)
	{
	  grub_free (old);
	  state->record = 0;
	}
    }
  state->recording[state->recordpos++] = c;
}

/* Fetch the next character for the lexer.  */
static void
nextchar (struct grub_lexer_param *state)
{
  if (state->record)
    recordchar (state, *state->script);
  state->script++;
}

int
grub_script_yylex (union YYSTYPE *yylval, struct grub_parser_param *parsestate)
{
  grub_parser_state_t newstate;
  char use;
  char *buffer;
  char *bp;
  struct grub_lexer_param *state = parsestate->lexerstate;
  int firstrun = 1;

  yylval->arg = 0;

  if (state->tokenonhold)
    {
      int token = state->tokenonhold;
      state->tokenonhold = 0;
      return token;
    }

  for (;! state->done; firstrun = 0)
    {
      if (! state->script || ! *state->script)
	{
	  /* Check if more tokens are requested by the parser.  */
	  if (((state->refs && ! parsestate->err)
	       || state->state == GRUB_PARSER_STATE_ESC
	       || state->state == GRUB_PARSER_STATE_QUOTE
	       || state->state == GRUB_PARSER_STATE_DQUOTE)
	      && state->getline)
	    {
	      int doexit = 0;
	      if (state->state != GRUB_PARSER_STATE_ESC
		  && state->state != GRUB_PARSER_STATE_QUOTE
		  && state->state != GRUB_PARSER_STATE_DQUOTE
		  && ! state->was_newline)
		{
		  state->was_newline = 1;
		  state->tokenonhold = '\n';
		  break;
		}
	      while (! state->script || ! *state->script)
		{
		  grub_free (state->newscript);
		  state->newscript = 0;
		  state->getline (&state->newscript, 1);
		  state->script = state->newscript;
		  if (! state->script)
		    {
		      doexit = 1;
		      break;
		    }
		}
	      if (doexit)
		break;
	      grub_dprintf ("scripting", "token=`\\n'\n");
	      recordchar (state, '\n');
	      if (state->state == GRUB_PARSER_STATE_VARNAME)
		state->state = GRUB_PARSER_STATE_TEXT;
	      if (state->state == GRUB_PARSER_STATE_QVARNAME)
		state->state = GRUB_PARSER_STATE_DQUOTE;
	      if (state->state == GRUB_PARSER_STATE_DQUOTE
		  || state->state == GRUB_PARSER_STATE_QUOTE)
		yylval->arg = grub_script_arg_add (parsestate, yylval->arg,
						   GRUB_SCRIPT_ARG_TYPE_STR,
						   "\n");
	    }
	  else
	    {
	      grub_free (state->newscript);
	      state->newscript = 0;
	      state->done = 1;
	      grub_dprintf ("scripting", "token=`\\n'\n");
	      state->tokenonhold = '\n';
	      break;
	    }
	}
      state->was_newline = 0;

      newstate = grub_parser_cmdline_state (state->state, *state->script, &use);

      /* Check if it is a text.  */
      if (check_textstate (newstate))
	{
	  /* In case the string is not quoted, this can be a one char
	     length symbol.  */
	  if (newstate == GRUB_PARSER_STATE_TEXT)
	    {
	      int doexit = 0;
	      switch (*state->script)
		{
		case ' ':
		  while (*state->script)
		    {
		      newstate = grub_parser_cmdline_state (state->state,
							    *state->script, &use);
		      if (! (state->state == GRUB_PARSER_STATE_TEXT
			     && *state->script == ' '))
			{
			  grub_dprintf ("scripting", "token=` '\n");
			  if (! firstrun)
			    doexit = 1;
			  break;
			}
		      state->state = newstate;
		      nextchar (state);
		    }
		  grub_dprintf ("scripting", "token=` '\n");
		  if (! firstrun)
		    doexit = 1;
		  break;
		case '{':
		case '}':
		case ';':
		case '\n':
		  {
		    char c;
		    grub_dprintf ("scripting", "token=`%c'\n", *state->script);
		    c = *state->script;;
		    nextchar (state);
		    state->tokenonhold = c;
		    doexit = 1;
		    break;
		  }
		}
	      if (doexit)
		break;
	    }

	  /* XXX: Use a better size.  */
	  buffer = grub_script_malloc (parsestate, 2048);
	  if (! buffer)
	    return 0;

	  bp = buffer;

	  /* Read one token, possible quoted.  */
	  while (*state->script)
	    {
	      newstate = grub_parser_cmdline_state (state->state,
						    *state->script, &use);

	      /* Check if a variable name starts.  */
	      if (check_varstate (newstate))
		break;

	      /* If the string is not quoted or escaped, stop processing
		 when a special token was found.  It will be recognized
		 next time when this function is called.  */
	      if (newstate == GRUB_PARSER_STATE_TEXT
		  && state->state != GRUB_PARSER_STATE_ESC
		  && state->state != GRUB_PARSER_STATE_QUOTE
		  && state->state != GRUB_PARSER_STATE_DQUOTE)
		{
		  int breakout = 0;

		  switch (use)
		    {
		    case ' ':
		    case '{':
		    case '}':
		    case ';':
		    case '\n':
		      breakout = 1;
		    }
		  if (breakout)
		    break;
		  if (use)
		    *(bp++) = use;
		}
	      else if (use)
		*(bp++) = use;

	      state->state = newstate;
	      nextchar (state);
	    }

	  /* A string of text was read in.  */
	  *bp = '\0';
	  grub_dprintf ("scripting", "token=`%s'\n", buffer);
	  yylval->arg = grub_script_arg_add (parsestate, yylval->arg,
					     GRUB_SCRIPT_ARG_TYPE_STR, buffer);

	}
      else if (newstate == GRUB_PARSER_STATE_VAR
	       || newstate == GRUB_PARSER_STATE_QVAR)
	{
	  /* XXX: Use a better size.  */
	  buffer = grub_script_malloc (parsestate, 2096);
	  if (! buffer)
	    return 0;

	  bp = buffer;

	  /* This is a variable, read the variable name.  */
	  while (*state->script)
	    {
	      newstate = grub_parser_cmdline_state (state->state,
						    *state->script, &use);

	      /* Check if this character is not part of the variable name
		 anymore.  */
	      if (! (check_varstate (newstate)))
		{
		  if (state->state == GRUB_PARSER_STATE_VARNAME2
		  || state->state == GRUB_PARSER_STATE_QVARNAME2)
		    nextchar (state);
		  state->state = newstate;
		  break;
		}

	      if (use)
		*(bp++) = use;
	      nextchar (state);
	      state->state = newstate;
	    }

	  *bp = '\0';
	  state->state = newstate;
	  yylval->arg = grub_script_arg_add (parsestate, yylval->arg,
					     GRUB_SCRIPT_ARG_TYPE_VAR, buffer);
	  grub_dprintf ("scripting", "vartoken=`%s'\n", buffer);
	}
      else
	{
	  /* There is either text or a variable name.  In the case you
	 arrive here there is a serious problem with the lexer.  */
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "Internal error\n");
	  return 0;
	}
    }

  if (yylval->arg == 0)
    {
      int token = state->tokenonhold;
      state->tokenonhold = 0;
      return token;
    }

  if (yylval->arg->next == 0 && yylval->arg->type == GRUB_SCRIPT_ARG_TYPE_STR)
    {
      /* Detect some special tokens.  */
      if (! grub_strcmp (yylval->arg->str, "while"))
	return GRUB_PARSER_TOKEN_WHILE;
      else if (! grub_strcmp (yylval->arg->str, "if"))
	return GRUB_PARSER_TOKEN_IF;
      else if (! grub_strcmp (yylval->arg->str, "function"))
	return GRUB_PARSER_TOKEN_FUNCTION;
      else if (! grub_strcmp (yylval->arg->str, "menuentry"))
	return GRUB_PARSER_TOKEN_MENUENTRY;
      else if (! grub_strcmp (yylval->arg->str, "@"))
	return GRUB_PARSER_TOKEN_MENUENTRY;
      else if (! grub_strcmp (yylval->arg->str, "else"))
	return GRUB_PARSER_TOKEN_ELSE;
      else if (! grub_strcmp (yylval->arg->str, "then"))
	return GRUB_PARSER_TOKEN_THEN;
      else if (! grub_strcmp (yylval->arg->str, "fi"))
	return GRUB_PARSER_TOKEN_FI;
    }

  return GRUB_PARSER_TOKEN_ARG;
}

void
grub_script_yyerror (struct grub_parser_param *lex __attribute__ ((unused)),
		     char const *err)
{
  grub_printf ("%s\n", err);
}
