/* lexer.c - The scripting lexer.  */
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

#include <grub/parser.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/script.h>

#include "grub_script.tab.h"

static grub_parser_state_t grub_script_lexer_state;
static int grub_script_lexer_done = 0;
static grub_err_t (*grub_script_lexer_getline) (char **);

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
	  || state == GRUB_PARSER_STATE_QUOTE
	  || state == GRUB_PARSER_STATE_DQUOTE);
}

/* The amount of references to the lexer by the parser.  If the parser
   expects tokens the lexer is referenced.  */
static int grub_script_lexer_refs = 0;
static char *script;
static char *newscript;

static int record = 0;
static char *recording = 0;
static int recordpos = 0;
static int recordlen = 0;

/* XXX: The lexer is not reentrant.  */
void
grub_script_lexer_init (char *s, grub_err_t (*getline) (char **))
{
  grub_script_lexer_state = GRUB_PARSER_STATE_TEXT;
  grub_script_lexer_getline = getline;
  grub_script_lexer_refs = 0;
  grub_script_lexer_done = 0;
  newscript = 0;
  script = s;
}

void
grub_script_lexer_ref (void)
{
  grub_script_lexer_refs++;
}

void
grub_script_lexer_deref (void)
{
  grub_script_lexer_refs--;
}

/* Start recording all characters passing through the lexer.  */
void
grub_script_lexer_record_start (void)
{
  record = 1;
  recordlen = 100;
  recording = grub_malloc (recordlen);
  recordpos = 0;
}

char *
grub_script_lexer_record_stop (void)
{
  record = 0;

  /* Delete the last character, it is a `}'.  */
  if (recordpos > 0)
    {
      if (recording[--recordpos] != '}')
	{
	  grub_printf ("Internal error while parsing menu entry");
	  for (;;); /* XXX */
	}
      recording[recordpos] = '\0';
    }

  return recording;
}

/* When recording is enabled, record the character C as the next item
   in the character stream.  */
static void
recordchar (char c)
{
  if (recordpos == recordlen)
    {
      char *old = recording;
      recordlen += 100;
      recording = grub_realloc (recording, recordlen);
      if (! recording)
	{
	  grub_free (old);
	  record = 0;
	}
    }
  recording[recordpos++] = c;
}

/* Fetch the next character for the lexer.  */
static void
nextchar (void)
{
  if (record)
    recordchar (*script);
  script++;
}

int
grub_script_yylex (void)
{
  grub_parser_state_t newstate;
  char use;
  char *buffer;
  char *bp;

  if (grub_script_lexer_done)
    return 0;

  if (! *script)
    {
      /* Check if more tokens are requested by the parser.  */
      if ((grub_script_lexer_refs
	   || grub_script_lexer_state == GRUB_PARSER_STATE_ESC)
	  && grub_script_lexer_getline)
	{
	  while (!script || ! grub_strlen (script))
	    {
	      grub_free (newscript);
	      newscript = 0;
	      grub_script_lexer_getline (&newscript);
	      script = newscript;
	      if (! script)
		return 0;
	    }
	  grub_dprintf ("scripting", "token=`\\n'\n");
	  recordchar ('\n');
	  if (grub_script_lexer_state != GRUB_PARSER_STATE_ESC)
	    return '\n';
	}
      else
	{
	  grub_free (newscript);
	  newscript = 0;
	  grub_script_lexer_done = 1;
	  grub_dprintf ("scripting", "token=`\\n'\n");
	  return '\n';
	}
    }

  newstate = grub_parser_cmdline_state (grub_script_lexer_state, *script, &use);

  /* Check if it is a text.  */
  if (check_textstate (newstate))
    {
      /* In case the string is not quoted, this can be a one char
	 length symbol.  */
      if (newstate == GRUB_PARSER_STATE_TEXT)
	{
	  switch (*script)
	    {
	    case ' ':
	      while (*script)
		{
		  newstate = grub_parser_cmdline_state (grub_script_lexer_state,
							*script, &use);
		  if (! (grub_script_lexer_state == GRUB_PARSER_STATE_TEXT
			 && *script == ' '))
		    {
		      grub_dprintf ("scripting", "token=` '\n");
		      return ' ';
		    }
		  grub_script_lexer_state = newstate;
		  nextchar ();
		}
	      grub_dprintf ("scripting", "token=` '\n");
	      return ' ';
	    case '{':
	    case '}':
	    case ';':
	    case '\n':
	      {
		char c;
		grub_dprintf ("scripting", "token=`%c'\n", *script);
		c = *script;;
		nextchar ();
		return c;
	      }
	    }
	}

      /* XXX: Use a better size.  */
      buffer = grub_script_malloc (2048);
      if (! buffer)
	return 0;

      bp = buffer;

      /* Read one token, possible quoted.  */
      while (*script)
	{
	  newstate = grub_parser_cmdline_state (grub_script_lexer_state,
						*script, &use);

	  /* Check if a variable name starts.  */
	  if (check_varstate (newstate))
	    break;

	  /* If the string is not quoted or escaped, stop processing
	     when a special token was found.  It will be recognised
	     next time when this function is called.  */
	  if (newstate == GRUB_PARSER_STATE_TEXT
	      && grub_script_lexer_state != GRUB_PARSER_STATE_ESC)
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
	      *(bp++) = use;
	    }
	  else if (use)
	    *(bp++) = use;

	  grub_script_lexer_state = newstate;
	  nextchar ();
	}

      /* A string of text was read in.  */
      *bp = '\0';
      grub_dprintf ("scripting", "token=`%s'\n", buffer);
      grub_script_yylval.string = buffer;

      /* Detect some special tokens.  */
      if (! grub_strcmp (buffer, "while"))
	return GRUB_PARSER_TOKEN_WHILE;
      else if (! grub_strcmp (buffer, "if"))
	return GRUB_PARSER_TOKEN_IF;
      else if (! grub_strcmp (buffer, "function"))
	return GRUB_PARSER_TOKEN_FUNCTION;
      else if (! grub_strcmp (buffer, "menuentry"))
	return GRUB_PARSER_TOKEN_MENUENTRY;
      else if (! grub_strcmp (buffer, "@"))
	return GRUB_PARSER_TOKEN_MENUENTRY;
      else if (! grub_strcmp (buffer, "else"))
	return GRUB_PARSER_TOKEN_ELSE;
      else if (! grub_strcmp (buffer, "then"))
	return GRUB_PARSER_TOKEN_THEN;
      else if (! grub_strcmp (buffer, "fi"))
	return GRUB_PARSER_TOKEN_FI;
      else
	return GRUB_PARSER_TOKEN_NAME;
    }
  else if (newstate == GRUB_PARSER_STATE_VAR
	   || newstate == GRUB_PARSER_STATE_QVAR)
    {
      /* XXX: Use a better size.  */
      buffer = grub_script_malloc (2096);
      if (! buffer)
	return 0;

      bp = buffer;

      /* This is a variable, read the variable name.  */
      while (*script)
	{
	  newstate = grub_parser_cmdline_state (grub_script_lexer_state,
						*script, &use);

	  /* Check if this character is not part of the variable name
	     anymore.  */
	  if (! (check_varstate (newstate)))
	    {
	      if (grub_script_lexer_state == GRUB_PARSER_STATE_VARNAME2
		  || grub_script_lexer_state == GRUB_PARSER_STATE_QVARNAME2)
		nextchar ();
	      grub_script_lexer_state = newstate;
	      break;
	    }

	  if (use)
	    *(bp++) = use;
	  nextchar ();
	  grub_script_lexer_state = newstate;
	}

      *bp = '\0';
      grub_script_lexer_state = newstate;
      grub_script_yylval.string = buffer;
      grub_dprintf ("scripting", "vartoken=`%s'\n", buffer);

      return GRUB_PARSER_TOKEN_VAR;
    }
  else
    {
      /* There is either text or a variable name.  In the case you
	 arrive here there is a serious problem with the lexer.  */
      grub_error (GRUB_ERR_BAD_ARGUMENT, "Internal error\n");
      return 0;
    }
}

void
grub_script_yyerror (char const *err)
{
  grub_printf (err);
}
