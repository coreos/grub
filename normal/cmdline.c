/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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
#include <pupa/machine/partition.h>
#include <pupa/disk.h>
#include <pupa/file.h>
#include <pupa/env.h>

static char *kill_buf;

static int hist_size;
static char **hist_lines = 0;
static int hist_pos = 0;
static int hist_end = 0;
static int hist_used = 0;

pupa_err_t
pupa_set_history (int newsize)
{
  char **old_hist_lines = hist_lines;
  hist_lines = pupa_malloc (sizeof (char *) * newsize);

  /* Copy the old lines into the new buffer.  */
  if (old_hist_lines)
    {
      /* Remove the lines that don't fit in the new buffer.  */
      if (newsize < hist_used)
	{
	  int i;
	  int delsize = hist_used - newsize;
	  hist_used = newsize;

	  for (i = 0; i < delsize; i++)
	    {
	      int pos = hist_end - i;
	      if (pos > hist_size)
		pos -= hist_size;
	      pupa_free (old_hist_lines[pos]);
	    }

	  hist_end -= delsize;
	  if (hist_end < 0)
	    hist_end = hist_size - hist_end;
	}

      if (hist_pos < hist_end)
	pupa_memmove (hist_lines, old_hist_lines + hist_pos,
		      (hist_end - hist_pos) * sizeof (char *));
      else
	{
	  /* Copy the first part.  */
	  pupa_memmove (hist_lines, old_hist_lines,
			hist_pos * sizeof (char *));


	  /* Copy the last part.  */
	  pupa_memmove (hist_lines + hist_pos, old_hist_lines + hist_pos,
			(hist_size - hist_pos) * sizeof (char *));

	}
    }

  pupa_free (old_hist_lines);

  hist_size = newsize;
  hist_pos = 0;
  hist_end = hist_used;
  return 0;
}

/* Get the entry POS from the history where `0' is the newest
   entry.  */
static char *
pupa_history_get (int pos)
{
  pos = (hist_pos + pos) % hist_size;
  return hist_lines[pos];
}


/* Insert a new history line S on the top of the history.  */
static void
pupa_history_add (char *s)
{
  /* Remove the oldest entry in the history to make room for a new
     entry.  */
  if (hist_used + 1 > hist_size)
    {
      hist_end--;
      if (hist_end < 0)
	hist_end = hist_size + hist_end;

      pupa_free (hist_lines[hist_end]);
    }
  else
    hist_used++;

  /* Move to the next position.  */
  hist_pos--;
  if (hist_pos < 0)
    hist_pos = hist_size + hist_pos;

  /* Insert into history.  */
  hist_lines[hist_pos] = pupa_strdup (s);
}

/* Replace the history entry on position POS with the string S.  */
static void
pupa_history_replace (int pos, char *s)
{
  pos = (hist_pos + pos) % hist_size;
  pupa_free (hist_lines[pos]);
  hist_lines[pos] = pupa_strdup (s);
}

/* Try to complete the string in BUF, return the characters that
   should be added to the string.  This command outputs the possible
   completions, in that case set RESTORE to 1 so the caller can
   restore the prompt.  */
static char *
pupa_tab_complete (char *buf, int *restore)
{
  char *pos = buf;
  char *path;
  
  char *found = 0;
  int begin;
  int end;
  int len;
  int numfound = 0;

  /* The disk that is used for pupa_partition_iterate.  */
  pupa_device_t partdev;
		
  /* String that is added when matched.  */
  char *matchstr;

  auto void print_simple_completion (char *comp);
  auto void print_partition_completion (char *comp);
  auto int NESTED_FUNC_ATTR add_completion (const char *comp, const char *match,
					    const char *what, 
					    void (*print_completion) (char *));
  auto int iterate_commands (pupa_command_t cmd);
  auto int iterate_dev (const char *devname);
  auto int iterate_part (const pupa_partition_t p);
  auto int iterate_dir (const char *filename, int dir);
  


  void print_simple_completion (char *comp)
    {
      pupa_printf (" %s", comp);
    }

  void print_partition_completion (char *comp)
    {
      pupa_print_partinfo (partdev, comp);
      pupa_errno = 0;
    }

  /* Add a string to the list of possible completions.  COMP is the
     string that should be added.  If this string completely matches
     add the string MATCH to the input after adding COMP.  The string
     WHAT contains a discription of the kind of data that is added.
     Use PRINT_COMPLETION to show the completions if there are
     multiple matches.  XXX: Because of a bug in gcc it is required to
     use __regparm__ in some cases.  */

  int NESTED_FUNC_ATTR add_completion (const char *comp, const char *match,
				       const char *what, 
				       void (*print_completion) (char *))
    {
      /* Bug in strncmp then len ==0.  */
      if (!len || pupa_strncmp (pos, comp, len) == 0)
	{
	  numfound++;
	
	  if (numfound == 1)
	    {
	      begin = len;
	      found = pupa_strdup (comp);
	      end = pupa_strlen (found);
	      matchstr = (char *) match;
	    }
	  /* Multiple matches found, print the first instead of completing.  */
	  else if (numfound == 2)
	    {
	      pupa_printf ("\nPossible %s are: ", what);
	      print_completion (found);
	    }
	    
	  if (numfound > 1)
	    {
	      char *s1 = found;
	      const char *s2 = comp;
	      int cnt = 0;
	    
	      print_completion ((char *) comp);
			    
	      /* Find out how many characters match.  */
	      while ((cnt < end) && *s1 && *s2 && (*s1 == *s2))
		{
		  s1++;
		  s2++;
		  cnt++;
		}	    
	      end = cnt;
	    }
	}

      return 0;
    }

  int iterate_part (const pupa_partition_t p)
    {
      add_completion (pupa_partition_get_name (p), ")", "partitions", 
		      print_partition_completion);
      return 0;
    }

  int iterate_dir (const char *filename, int dir)
    {
      if (!dir)
	add_completion (filename, " ", "files", print_simple_completion);
      else
	{
	  char fname[pupa_strlen (filename) + 2];
	  pupa_strcpy (fname, filename);
	  pupa_sprintf (fname, "%s/", filename);
	  add_completion (fname, "", "files", print_simple_completion);
	}
      return 0;
    }

  int iterate_dev (const char *devname)
    {
      pupa_device_t dev;
      
      /* Complete the partition part.  */
      dev = pupa_device_open (devname);
      
      if (dev)
	{
	  if (dev->disk && dev->disk->has_partitions)
	    add_completion (devname, ",", "disks", print_simple_completion);
	  else 
	    add_completion (devname, ")", "disks", print_simple_completion);
	}

      pupa_errno = PUPA_ERR_NONE;
      return 0;
    }

  int iterate_commands (pupa_command_t cmd)
    {
      if (cmd->flags & PUPA_COMMAND_FLAG_CMDLINE)
	add_completion (cmd->name, " ", "commands", print_simple_completion);
      return 0;
    }
  
  /* Remove blank space on the beginning of the line.  */
  while (*pos == ' ')
    pos++;

  /* Check if the string is a command or path.  */
  path = pupa_strchr (pos, ' ');
      
  if (!path)
    {
      /* Tab complete a command.  */
      len = pupa_strlen (pos);
      
      pupa_iterate_commands (iterate_commands);
    }
  else
    {
      pos = path;

      /* Remove blank space on the beginning of the line.  */
      while (*pos == ' ')
	pos++;
	
      /* Check if this is a completion for a device name.  */
      if (*pos == '(' && !pupa_strchr (pos, ')'))
	{
	  /* Check if this is a device or partition.  */
	  char *partition = pupa_strchr (++pos, ',');
			
	  if (!partition)
	    {
	      /* Complete the disk part.  */
	      len = pupa_strlen (pos);
	      pupa_disk_dev_iterate (iterate_dev);
	      if (pupa_errno)
		goto fail;
	    }
	  else
	    {
	      *partition = '\0';

	      /* Complete the partition part.  */
	      partdev = pupa_device_open (pos);
	      *partition = ',';
	      pupa_errno = PUPA_ERR_NONE;
  
	      if (partdev)
		{
		  if (partdev->disk && partdev->disk->has_partitions)
		    {
		      pos = partition + 1;
		      len = pupa_strlen (pos);
		      
		      pupa_partition_iterate (partdev->disk, iterate_part);
		      if (pupa_errno)
			pupa_errno = 0;
		    }

		  pupa_device_close (partdev);
		}
	      else
		goto fail;
	    }
	}
      else
	{
	  char *device = pupa_file_get_device_name (pos);
	  pupa_device_t dev;
	  pupa_fs_t fs;

	  dev = pupa_device_open (device);
	  if (!dev)
	    goto fail;
			
	  fs = pupa_fs_probe (dev);
	  if (pupa_errno)
	    goto fail;

	  pos = pupa_strrchr (pos, '/');
	  if (pos)
	    {
	      char *dir;
	      char *dirfile;
	      pos++;
	      len = pupa_strlen (pos);
	      
	      dir = pupa_strchr (path, '/');
	      if (!dir)
		{
		  *restore = 0;
		  return 0;
		}

	      dir = pupa_strdup (dir);

	      /* Cut away the filename part.  */
	      dirfile = pupa_strrchr (dir, '/');
	      dirfile[1] = '\0';
	      
	      /* Tab complete a file.  */
	      (fs->dir) (dev, dir, iterate_dir);
	      if (dev)
		pupa_device_close (dev);

	      pupa_free (device);
	      pupa_free (dir);

	      if (pupa_errno)
		goto fail;
	    }
	  else
	    {
	      found = pupa_strdup ("/");
	      matchstr = "";
	      numfound = 1;
	      begin = 0;
	      end = 1;
	    }
	}
		    
    }

  /* If more than one match is found those matches will be printed and
     the prompt should be restored.  */
  if (numfound > 1)
    *restore = 1;
  else
    *restore = 0;

  /* Return the part that matches.  */
  if (end && found)
    {
      char *insert;
      insert = pupa_malloc (end - begin + 1 + sizeof (matchstr));
      pupa_strncpy (insert, found + begin, end - begin);
      insert[end - begin] = '\0';
      if (numfound == 1)
	pupa_strcat (insert, matchstr);
      pupa_free (found);

      return insert;
    }

 fail:
  pupa_free (found);
  pupa_errno = PUPA_ERR_NONE;

  return 0;
}

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

      pupa_print_error ();
      pupa_errno = PUPA_ERR_NONE;
      cmdline[0] = '\0';
      
      if (! pupa_cmdline_get ("pupa> ", cmdline, sizeof (cmdline), 0, 1)
	  && nested)
	return;

      if (! *cmdline)
	continue;

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
  int histpos = 0;
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

  pupa_history_add (buf);

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
	      {
		char *insert;
		int restore;
		
		/* Backup the next character and make it 0 so it will
		   be easy to use string functions.  */
		char backup = buf[lpos];
		buf[lpos] = '\0';
		

		insert = pupa_tab_complete (buf, &restore);
		/* Restore the original string.  */
		buf[lpos] = backup;
		
		if (restore)
		  {
		    /* Restore the prompt.  */
		    pupa_printf ("\n%s%s", prompt, buf);
		    xpos = plen;
		    ystart = ypos = (pupa_getxy () & 0xFF);
		  }

		if (insert)
		  {
		    cl_insert (insert);
		    pupa_free (insert);
		  }
	      }
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
	      {
		char *hist;

		lpos = 0;

		if (histpos > 0)
		  histpos--;

		cl_delete (llen);
		hist = pupa_history_get (histpos);
		cl_insert (hist);

		break;
	      }
	    case 16:	/* Ctrl-p */
	      {
		char *hist;

		lpos = 0;

		if (histpos < hist_used - 1)
		  histpos++;

		cl_delete (llen);
		hist = pupa_history_get (histpos);

		cl_insert (hist);
	      }
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

      pupa_history_replace (histpos, buf);
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
