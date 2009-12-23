/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2009  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/env.h>
#include <grub/i18n.h>

static grub_uint32_t *kill_buf;

static int hist_size;
static grub_uint32_t **hist_lines = 0;
static int hist_pos = 0;
static int hist_end = 0;
static int hist_used = 0;

grub_err_t
grub_set_history (int newsize)
{
  grub_uint32_t **old_hist_lines = hist_lines;
  hist_lines = grub_malloc (sizeof (char *) * newsize);

  /* Copy the old lines into the new buffer.  */
  if (old_hist_lines)
    {
      /* Remove the lines that don't fit in the new buffer.  */
      if (newsize < hist_used)
	{
	  int i;
	  int delsize = hist_used - newsize;
	  hist_used = newsize;

	  for (i = 1; i <= delsize; i++)
	    {
	      int pos = hist_end - i;
	      if (pos < 0)
		pos += hist_size;
	      grub_free (old_hist_lines[pos]);
	    }

	  hist_end -= delsize;
	  if (hist_end < 0)
	    hist_end += hist_size;
	}

      if (hist_pos < hist_end)
	grub_memmove (hist_lines, old_hist_lines + hist_pos,
		      (hist_end - hist_pos) * sizeof (char *));
      else if (hist_used)
	{
	  /* Copy the older part.  */
	  grub_memmove (hist_lines, old_hist_lines + hist_pos,
 			(hist_size - hist_pos) * sizeof (char *));

	  /* Copy the newer part. */
	  grub_memmove (hist_lines + hist_size - hist_pos, old_hist_lines,
			hist_end * sizeof (char *));
	}
    }

  grub_free (old_hist_lines);

  hist_size = newsize;
  hist_pos = 0;
  hist_end = hist_used;
  return 0;
}

/* Get the entry POS from the history where `0' is the newest
   entry.  */
static grub_uint32_t *
grub_history_get (int pos)
{
  pos = (hist_pos + pos) % hist_size;
  return hist_lines[pos];
}

static grub_size_t
strlen_ucs4 (const grub_uint32_t *s)
{
  const grub_uint32_t *p = s;

  while (*p)
    p++;

  return p - s;
}

static grub_uint32_t *
strdup_ucs4 (const grub_uint32_t *s)
{
  grub_size_t len;
  char *p;

  len = strlen_ucs4 (s) + 1;
  p = (char *) grub_malloc (len * sizeof (grub_uint32_t));
  if (! p)
    return 0;

  return grub_memcpy (p, s, len * sizeof (grub_uint32_t));
}


/* Insert a new history line S on the top of the history.  */
static void
grub_history_add (grub_uint32_t *s)
{
  /* Remove the oldest entry in the history to make room for a new
     entry.  */
  if (hist_used + 1 > hist_size)
    {
      hist_end--;
      if (hist_end < 0)
	hist_end = hist_size + hist_end;

      grub_free (hist_lines[hist_end]);
    }
  else
    hist_used++;

  /* Move to the next position.  */
  hist_pos--;
  if (hist_pos < 0)
    hist_pos = hist_size + hist_pos;

  /* Insert into history.  */
  hist_lines[hist_pos] = strdup_ucs4 (s);
}

/* Replace the history entry on position POS with the string S.  */
static void
grub_history_replace (int pos, grub_uint32_t *s)
{
  pos = (hist_pos + pos) % hist_size;
  grub_free (hist_lines[pos]);
  hist_lines[pos] = strdup_ucs4 (s);
}

/* A completion hook to print items.  */
static void
print_completion (const char *item, grub_completion_type_t type, int count)
{
  if (count == 0)
    {
      /* If this is the first time, print a label.  */
      const char *what;

      switch (type)
	{
	case GRUB_COMPLETION_TYPE_COMMAND:
	  what = "commands";
	  break;
	case GRUB_COMPLETION_TYPE_DEVICE:
	  what = "devices";
	  break;
	case GRUB_COMPLETION_TYPE_FILE:
	  what = "files";
	  break;
	case GRUB_COMPLETION_TYPE_PARTITION:
	  what = "partitions";
	  break;
	case GRUB_COMPLETION_TYPE_ARGUMENT:
	  what = "arguments";
	  break;
	default:
	  what = "things";
	  break;
	}

      grub_printf ("\nPossible %s are:\n", what);
    }

  if (type == GRUB_COMPLETION_TYPE_PARTITION)
    {
      grub_normal_print_device_info (item);
      grub_errno = GRUB_ERR_NONE;
    }
  else
    grub_printf (" %s", item);
}

struct cmdline_term
{
  unsigned xpos, ypos, ystart, width, height;
  struct grub_term_output *term;
};

/* Get a command-line. If ESC is pushed, return zero,
   otherwise return command line.  */
/* FIXME: The dumb interface is not supported yet.  */
char *
grub_cmdline_get (const char *prompt, unsigned max_len)
{
  grub_size_t lpos, llen;
  grub_size_t plen;
  grub_uint32_t buf[max_len];
  int key;
  int histpos = 0;
  auto void cl_insert (const grub_uint32_t *str);
  auto void cl_delete (unsigned len);
  auto void cl_print (struct cmdline_term *cl_term, int pos, grub_uint32_t c);
  auto void cl_set_pos (struct cmdline_term *cl_term);
  auto void cl_print_all (int pos, grub_uint32_t c);
  auto void cl_set_pos_all (void);
  const char *prompt_translated = _(prompt);
  struct cmdline_term *cl_terms;
  unsigned nterms;

  void cl_set_pos (struct cmdline_term *cl_term)
  {
    cl_term->xpos = (plen + lpos) % (cl_term->width - 1);
    cl_term->ypos = cl_term->ystart + (plen + lpos) / (cl_term->width - 1);
    cl_term->term->gotoxy (cl_term->xpos, cl_term->ypos);
  }

  void cl_set_pos_all ()
  {
    unsigned i;
    for (i = 0; i < nterms; i++)
      cl_set_pos (&cl_terms[i]);
  }

  void cl_print (struct cmdline_term *cl_term, int pos, grub_uint32_t c)
    {
      grub_uint32_t *p;

      for (p = buf + pos; *p; p++)
	{
	  if (cl_term->xpos++ > cl_term->width - 2)
	    {
	      grub_putcode ('\n', cl_term->term);

	      cl_term->xpos = 1;
	      if (cl_term->ypos == (unsigned) (cl_term->height))
		cl_term->ystart--;
	      else
		cl_term->ypos++;
	    }

	  if (c)
	    grub_putcode (c, cl_term->term);
	  else
	    grub_putcode (*p, cl_term->term);
	}
    }

  void cl_print_all (int pos, grub_uint32_t c)
  {
    unsigned i;
    for (i = 0; i < nterms; i++)
      cl_print (&cl_terms[i], pos, c);
  }

  void cl_insert (const grub_uint32_t *str)
    {
      grub_size_t len = strlen_ucs4 (str);

      if (len + llen < max_len)
	{
	  grub_memmove (buf + lpos + len, buf + lpos, llen - lpos + 1);
	  grub_memmove (buf + lpos, str, len);

	  llen += len;
	  lpos += len;
	  cl_print_all (lpos - len, echo_char);
	  cl_set_pos_all ();
	}

      grub_refresh ();
    }

  void cl_delete (unsigned len)
    {
      if (lpos + len <= llen)
	{
	  grub_size_t saved_lpos = lpos;

	  lpos = llen - len;
	  cl_set_pos_all ();
	  cl_print_all (lpos, ' ');
	  lpos = saved_lpos;
	  cl_set_pos_all ();

	  grub_memmove (buf + lpos, buf + lpos + len, llen - lpos + 1);
	  llen -= len;
	  cl_print_all (lpos, echo_char);
	  cl_set_pos_all ();
	}

      grub_refresh ();
    }

  void init_clterm (struct cmdline_term *cl_term_cur)
  {
    cl_term_cur->xpos = plen;
    cl_term_cur->ypos = (cl_term_cur->term->getxy () & 0xFF);
    cl_term_cur->ystart = cl_term_cur->ypos;
    cl_term_cur->width = grub_term_width (cl_term_cur->term);
    cl_term_cur->height = grub_term_height (cl_term_cur->term);
  }

  void init_clterm_all (void)
  {
    unsigned i;
    for (i = 0; i < nterms; i++)
      init_clterm (&cl_terms[i]);
  }

  plen = grub_strlen (prompt_translated);
  lpos = llen = 0;
  buf[0] = '\0';

  grub_putchar ('\n');

  grub_printf ("%s", prompt_translated);

  {
    struct cmdline_term *cl_term_cur;
    struct grub_term_output *cur;
    nterms = 0;
    for (cur = grub_term_outputs; cur; cur = cur->next)
      if (cur->flags & GRUB_TERM_ACTIVE)
	nterms++;

    cl_terms = grub_malloc (sizeof (cl_terms[0]) * nterms);
    if (!cl_terms)
      return grub_errno;
    cl_term_cur = cl_terms;
    for (cur = grub_term_outputs; cur; cur = cur->next)
      if (cur->flags & GRUB_TERM_ACTIVE)
	{
	  cl_term_cur->term = cur;
	  init_clterm (cl_term_cur);
	  cl_term_cur++;
	}
  }

  if (history && hist_used == 0)
    grub_history_add (buf);

  while ((key = GRUB_TERM_ASCII_CHAR (grub_getkey ())) != '\n' && key != '\r')
    {
      switch (key)
	{
	case 1:	/* Ctrl-a */
	  lpos = 0;
	  cl_set_pos_all ();
	  break;

	case 2:	/* Ctrl-b */
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos_all ();
	    }
	  break;

	case 5:	/* Ctrl-e */
	  lpos = llen;
	  cl_set_pos_all ();
	  break;

	case 6:	/* Ctrl-f */
	  if (lpos < llen)
	    {
	      lpos++;
	      cl_set_pos_all ();
	    }
	  break;

	case 9:	/* Ctrl-i or TAB */
	  {
	    grub_uint32_t *insert;
	    int restore;

	    /* Backup the next character and make it 0 so it will
	       be easy to use string functions.  */
	    char backup = buf[lpos];
	    buf[lpos] = '\0';


	    insert = grub_normal_do_completion (buf, &restore,
						print_completion);
	    /* Restore the original string.  */
	    buf[lpos] = backup;

	    if (restore)
	      {
		/* Restore the prompt.  */
		grub_printf ("\n%s", prompt_translated);
		init_clterm_all ();
		cl_print_all (0, 0);
	      }

	    if (insert)
	      {
		cl_insert (insert);
		grub_free (insert);
	      }
	  }
	  break;

	case 11:	/* Ctrl-k */
	  if (lpos < llen)
	    {
	      if (kill_buf)
		grub_free (kill_buf);

	      kill_buf = strdup_ucs4 (buf + lpos);
	      grub_errno = GRUB_ERR_NONE;

	      cl_delete (llen - lpos);
	    }
	  break;

	case 14:	/* Ctrl-n */
	  {
	    grub_uint32_t *hist;

	    lpos = 0;

	    if (histpos > 0)
	      {
		grub_history_replace (histpos, buf);
		histpos--;
	      }

	    cl_delete (llen);
	    hist = grub_history_get (histpos);
	    cl_insert (hist);

	    break;
	  }
	case 16:	/* Ctrl-p */
	  {
	    grub_uint32_t *hist;

	    lpos = 0;

	    if (histpos < hist_used - 1)
	      {
		grub_history_replace (histpos, buf);
		histpos++;
	      }

	    cl_delete (llen);
	    hist = grub_history_get (histpos);

	    cl_insert (hist);
	  }
	  break;

	case 21:	/* Ctrl-u */
	  if (lpos > 0)
	    {
	      grub_size_t n = lpos;

	      if (kill_buf)
		grub_free (kill_buf);

	      kill_buf = grub_malloc (n + 1);
	      grub_errno = GRUB_ERR_NONE;
	      if (kill_buf)
		{
		  grub_memcpy (kill_buf, buf, n);
		  kill_buf[n] = '\0';
		}

	      lpos = 0;
	      cl_set_pos_all ();
	      cl_delete (n);
	    }
	  break;

	case 25:	/* Ctrl-y */
	  if (kill_buf)
	    cl_insert (kill_buf);
	  break;

	case '\e':
	  return 0;

	case '\b':
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos_all ();
	    }
          else
            break;
	  /* fall through */

	case 4:	/* Ctrl-d */
	  if (lpos < llen)
	    cl_delete (1);
	  break;

	default:
	  if (grub_isprint (key))
	    {
	      grub_uint32_t str[2];

	      str[0] = key;
	      str[1] = '\0';
	      cl_insert (str);
	    }
	  break;
	}
    }

  grub_putchar ('\n');
  grub_refresh ();

  /* Remove leading spaces.  */
  lpos = 0;
  while (buf[lpos] == ' ')
    lpos++;

  if (history)
    {
      histpos = 0;
      if (strlen_ucs4 (buf) > 0)
	{
	  grub_uint32_t empty[] = { 0 };
	  grub_history_replace (histpos, buf);
	  grub_history_add (empty);
	}
    }

  return grub_ucs4_to_utf8_alloc (buf + lpos, llen - lpos + 1);
}
