/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2003  Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/normal.h>
#include <pupa/misc.h>
#include <pupa/term.h>
#include <pupa/err.h>
#include <pupa/types.h>
#include <pupa/mm.h>

static char *kill_buf;

void
pupa_cmdline_run (int nested)
{
  pupa_normal_init_page ();
  
  pupa_printf ("\
 [ Minimal BASH-like line editing is supported. For the first word, TAB\n\
   lists possible command completions. Anywhere else TAB lists possible\n\
   device/file completions.%s ]\n\n",
	       nested ? " ESC at any time exits." : "");
  
  while (1)
    {
      static char cmdline[PUPA_MAX_CMDLINE];
      pupa_command_t cmd;

      pupa_print_error ();
      pupa_errno = PUPA_ERR_NONE;
      cmdline[0] = '\0';
      
      if (! pupa_cmdline_get ("pupa> ", cmdline, sizeof (cmdline), 0, 1)
	  && nested)
	return;

      if (! *cmdline)
	continue;

      cmd = pupa_command_find (cmdline);
      if (! cmd)
	continue;

      if (! (cmd->flags & PUPA_COMMAND_FLAG_CMDLINE))
	{
	  pupa_error (PUPA_ERR_UNKNOWN_COMMAND,
		      "invalid command `%s'", cmd->name);
	  continue;
	}

      pupa_command_execute (cmdline);
    }
}

/* Get a command-line. If ECHO_CHAR is not zero, echo it instead of input
   characters. If READLINE is non-zero, readline-like key bindings are
   available. If ESC is pushed, return non-zero, otherwise return zero.  */
/* FIXME: The dumb interface is not supported yet.  */
int
pupa_cmdline_get (const char *prompt, char cmdline[], unsigned max_len,
		  int echo_char, int readline)
{
  unsigned xpos, ypos, ystart;
  pupa_size_t lpos, llen;
  pupa_size_t plen;
  char buf[max_len];
  int key;
  auto void cl_insert (const char *str);
  auto void cl_delete (unsigned len);
  auto void cl_print (int pos, int c);
  auto void cl_set_pos (void);

  void cl_set_pos (void)
    {
      xpos = (plen + lpos) % 79;
      ypos = ystart + (plen + lpos) / 79;
      pupa_gotoxy (xpos, ypos);
    }
  
  void cl_print (int pos, int c)
    {
      char *p;

      for (p = buf + pos; *p; p++)
	{
	  if (xpos++ > 78)
	    {
	      pupa_putchar ('\n');
	      
	      xpos = 1;
	      if (ypos == (unsigned) (pupa_getxy () & 0xFF))
		ystart--;
	      else
		ypos++;
	    }

	  if (c)
	    pupa_putchar (c);
	  else
	    pupa_putchar (*p);
	}
    }
  
  void cl_insert (const char *str)
    {
      pupa_size_t len = pupa_strlen (str);

      if (len + llen < max_len)
	{
	  pupa_memmove (buf + lpos + len, buf + lpos, llen - lpos + 1);
	  pupa_memmove (buf + lpos, str, len);

	  llen += len;
	  lpos += len;
	  cl_print (lpos - len, echo_char);
	  cl_set_pos ();
	}
    }

  void cl_delete (unsigned len)
    {
      if (lpos + len <= llen)
	{
	  pupa_size_t saved_lpos = lpos;

	  lpos = llen - len;
	  cl_set_pos ();
	  cl_print (lpos, ' ');
	  lpos = saved_lpos;
	  cl_set_pos ();
	  
	  pupa_memmove (buf + lpos, buf + lpos + len, llen - lpos + 1);
	  llen -= len;
	  cl_print (lpos, echo_char);
	  cl_set_pos ();
	}
    }
  
  plen = pupa_strlen (prompt);
  lpos = llen = 0;
  buf[0] = '\0';

  if ((pupa_getxy () >> 8) != 0)
    pupa_putchar ('\n');
  
  pupa_printf (prompt);
  
  xpos = plen;
  ystart = ypos = (pupa_getxy () & 0xFF);
  
  cl_insert (cmdline);

  while ((key = PUPA_TERM_ASCII_CHAR (pupa_getkey ())) != '\n' && key != '\r')
    {
      if (readline)
	{
	  switch (key)
	    {
	    case 1:	/* Ctrl-a */
	      lpos = 0;
	      cl_set_pos ();
	      break;

	    case 2:	/* Ctrl-b */
	      if (lpos > 0)
		{
		  lpos--;
		  cl_set_pos ();
		}
	      break;

	    case 5:	/* Ctrl-e */
	      lpos = llen;
	      cl_set_pos ();
	      break;

	    case 6:	/* Ctrl-f */
	      if (lpos < llen)
		{
		  lpos++;
		  cl_set_pos ();
		}
	      break;

	    case 9:	/* Ctrl-i or TAB */
	      /* FIXME */
	      break;

	    case 11:	/* Ctrl-k */
	      if (lpos < llen)
		{
		  if (kill_buf)
		    pupa_free (kill_buf);

		  kill_buf = pupa_strdup (buf + lpos);
		  pupa_errno = PUPA_ERR_NONE;

		  cl_delete (llen - lpos);
		}
	      break;

	    case 14:	/* Ctrl-n */
	      /* FIXME */
	      break;

	    case 16:	/* Ctrl-p */
	      /* FIXME */
	      break;

	    case 21:	/* Ctrl-u */
	      if (lpos > 0)
		{
		  pupa_size_t n = lpos;
		  
		  if (kill_buf)
		    pupa_free (kill_buf);

		  kill_buf = pupa_malloc (n + 1);
		  pupa_errno = PUPA_ERR_NONE;
		  if (kill_buf)
		    {
		      pupa_memcpy (kill_buf, buf, n);
		      kill_buf[n] = '\0';
		    }

		  lpos = 0;
		  cl_set_pos ();
		  cl_delete (n);
		}
	      break;

	    case 25:	/* Ctrl-y */
	      if (kill_buf)
		cl_insert (kill_buf);
	      break;
	    }
	}

      switch (key)
	{
	case '\e':
	  return 0;

	case '\b':
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos ();
	    }
	  /* fall through */
	  
	case 4:	/* Ctrl-d */
	  if (lpos < llen)
	    cl_delete (1);
	  break;

	default:
	  if (pupa_isprint (key))
	    {
	      char str[2];

	      str[0] = key;
	      str[1] = '\0';
	      cl_insert (str);
	    }
	  break;
	}
    }

  pupa_putchar ('\n');
  pupa_refresh ();

  /* If ECHO_CHAR is NUL, remove leading spaces.  */
  lpos = 0;
  if (! echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  pupa_memcpy (cmdline, buf + lpos, llen - lpos + 1);

  return 1;
}
