/* char_io.c - basic console input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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

#include "shared.h"

void
print_error (void)
{
  if (errnum > ERR_NONE && errnum < MAX_ERR_NUM)
#ifndef STAGE1_5
    /* printf("\7\n %s\n", err_list[errnum]); */
    printf ("\nError: %s\n", err_list[errnum]);
#else /* STAGE1_5 */
    printf ("Error: %u\n", errnum);
#endif /* STAGE1_5 */
}

char *
convert_to_ascii (char *buf, int c,...)
{
  unsigned long num = *((&c) + 1), mult = 10;
  char *ptr = buf;

#ifndef STAGE1_5
  if (c == 'x')
    mult = 16;

  if ((num & 0x80000000uL) && c == 'd')
    {
      num = (~num) + 1;
      *(ptr++) = '-';
      buf++;
    }
#endif

  do
    {
      int dig = num % mult;
      *(ptr++) = ((dig > 9) ? dig + 'a' - 10 : '0' + dig);
    }
  while (num /= mult);

  /* reorder to correct direction!! */
  {
    char *ptr1 = ptr - 1;
    char *ptr2 = buf;
    while (ptr1 > ptr2)
      {
	int c = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = c;
	ptr1--;
	ptr2++;
      }
  }

  return ptr;
}

void
grub_printf (const char *format,...)
{
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];

  dataptr++;

  while ((c = *(format++)) != 0)
    {
      if (c != '%')
	putchar (c);
      else
	switch (c = *(format++))
	  {
#ifndef STAGE1_5
	  case 'd':
	  case 'x':
#endif
	  case 'u':
	    *convert_to_ascii (str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	      putchar (*(ptr++));
	    break;

#ifndef STAGE1_5
	  case 'c':
	    putchar ((*(dataptr++)) & 0xff);
	    break;

	  case 's':
	    ptr = (char *) (*(dataptr++));

	    while ((c = *(ptr++)) != 0)
	      putchar (c);
	    break;
#endif
	  }
    }
}

#ifndef STAGE1_5
int
grub_sprintf (char *buffer, const char *format, ...)
{
  /* XXX hohmuth
     ugly hack -- should unify with printf() */
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];
  char *bp = buffer;

  dataptr++;

  while ((c = *format++) != 0)
    {
      if (c != '%')
	*bp++ = c; /* putchar(c); */
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii (str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	      *bp++ = *(ptr++); /* putchar(*(ptr++)); */
	    break;

	  case 'c': *bp++ = (*(dataptr++))&0xff;
	    /* putchar((*(dataptr++))&0xff); */
	    break;

	  case 's':
	    ptr = (char *) (*(dataptr++));

	    while ((c = *ptr++) != 0)
	      *bp++ = c; /* putchar(c); */
	    break;
	  }
    }

  *bp = 0;
  return bp - buffer;
}

void
init_page (void)
{
  cls ();

  printf ("\n    GRUB  version %s  (%dK lower / %dK upper memory)\n\n",
	  version_string, mbi.mem_lower, mbi.mem_upper);
}

/* The number of the history entries.  */
static int num_history = 0;

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
static char *
get_history (int no)
{
  if (no < 0 || no >= num_history)
    return 0;

  return (char *) HISTORY_BUF + MAX_CMDLINE * no;
}

/* Add CMDLINE to the history buffer.  */
static void
add_history (const char *cmdline, int no)
{
  grub_memmove ((char *) HISTORY_BUF + MAX_CMDLINE * (no + 1),
		(char *) HISTORY_BUF + MAX_CMDLINE * no,
		MAX_CMDLINE * (num_history - no));
  grub_strcpy ((char *) HISTORY_BUF + MAX_CMDLINE * no, cmdline);
  if (num_history < HISTORY_SIZE)
    num_history++;
}

/* Don't use this with a MAXLEN greater than 1600 or so!  The problem
   is that GET_CMDLINE depends on the everything fitting on the screen
   at once.  So, the whole screen is about 2000 characters, minus the
   PROMPT, and space for error and status lines, etc.  MAXLEN must be
   at least 1, and PROMPT and CMDLINE must be valid strings (not NULL
   or zero-length).

   If ECHO_CHAR is nonzero, echo it instead of the typed character. */
int
get_cmdline (char *prompt, char *cmdline, int maxlen,
	     int echo_char, int readline)
{
  int ystart, yend, xend, lpos, c;
  /* The length of PROMPT.  */
  int plen = 0;
  /* The length of the command-line.  */
  int llen = 0;
  /* The index for the history.  */
  int history = -1;
  /* The working buffer for the command-line.  */
  char *buf = (char *) CMDLINE_BUF;
  /* The kill buffer.  */
  char *kill = (char *) KILL_BUF;

  /* nested function definition for code simplicity */
  static void cl_print (char *str, int echo_char)
    {
      while (*str != 0)
	{
	  putchar (echo_char ? echo_char : *str);
	  str++;
	  if (++xend > 78)
	    {
	      xend = 0;
	      putchar (' ');
	      if (yend == (getxy () & 0xff))
		ystart--;
	      else
		yend++;
	    }
	}
    }

  /* nested function definition for code simplicity */
  static void cl_setcpos (void)
    {
      yend = ((lpos + plen) / 79) + ystart;
      xend = ((lpos + plen) % 79);
      gotoxy (xend, yend);
    }

  /* nested function definition for initial command-line printing */
  static void cl_init ()
    {
      /* distinguish us from other lines and error messages! */
      putchar ('\n');

      /* print full line and set position here */
      ystart = (getxy () & 0xff);
      yend = ystart;
      xend = 0;
      cl_print (prompt, 0);
      cl_print (buf, echo_char);
      cl_setcpos ();
    }

  /* nested function definition for erasing to the end of the line */
  static void cl_kill_to_end ()
    {
      int i;
      buf[lpos] = 0;
      for (i = lpos; i <= llen; i++)
	{
	  if (i && ((i + plen) % 79) == 0)
	    putchar (' ');
	  putchar (' ');
	}
      llen = lpos;
      cl_setcpos ();
    }

  static void cl_insert (const char *str)
    {
      int l = grub_strlen (str);

      if (llen + l < maxlen)
	{
	  if (lpos == llen)
	    {
	      grub_memmove (buf + lpos, str, l + 1);
	      cl_setcpos ();
	      cl_print (buf + lpos, echo_char);
	      lpos += l;
	      cl_setcpos ();
	    }
	  else
	    {
	      grub_memmove (buf + lpos + l, buf + lpos, llen - lpos + 1);
	      grub_memmove (buf + lpos, str, l);
	      cl_setcpos ();
	      cl_print (buf + lpos, echo_char);
	      lpos += l;
	      cl_setcpos ();
	    }
	  llen += l;
	}
    }

  plen = grub_strlen (prompt);
  llen = grub_strlen (cmdline);

  if (maxlen > MAX_CMDLINE)
    {
      maxlen = MAX_CMDLINE;
      if (llen >= MAX_CMDLINE)
	{
	  llen = MAX_CMDLINE - 1;
	  cmdline[MAX_CMDLINE] = 0;
	}
    }
  lpos = llen;
  grub_strcpy (buf, cmdline);

  cl_init ();

  while (ASCII_CHAR (c = getkey ()) != '\n' && ASCII_CHAR (c) != '\r')
    {
      switch (c)
	{
	case KEY_LEFT:
	  c = 2;
	  break;
	case KEY_RIGHT:
	  c = 6;
	  break;
	case KEY_UP:
	  c = 16;
	  break;
	case KEY_DOWN:
	  c = 14;
	  break;
	case KEY_HOME:
	  c = 1;
	  break;
	case KEY_END:
	  c = 5;
	  break;
	case KEY_DC:
	  c = 4;
	  break;
	case KEY_BACKSPACE:
	  c = 8;
	  break;
	}

      c = ASCII_CHAR (c);

      /* If READLINE is non-zero, handle readline-like key bindings.  */
      if (readline)
	{
	  switch (c)
	    {
	    case 9:		/* TAB lists completions */
	      {
		int i;
		/* POS points to the first space after a command.  */
		int pos = 0;
		int ret;
		char *completion_buffer = (char *) COMPLETION_BUF;
		int equal_pos = -1;
		int is_filename;

		/* Find the first word.  */
		while (buf[pos] == ' ')
		  pos++;
		while (buf[pos] && buf[pos] != '=' && buf[pos] != ' ')
		  pos++;

		is_filename = (lpos > pos);

		/* Find the position of the equal character after a
		   command, and replace it with a space.  */
		for (i = pos; buf[i] && buf[i] != ' '; i++)
		  if (buf[i] == '=')
		    {
		      equal_pos = i;
		      buf[i] = ' ';
		      break;
		    }

		/* Find the position of the first character in this
		   word.  */
		for (i = lpos; i > 0 && buf[i - 1] != ' '; i--)
		  ;

		/* Invalidate the cache, because the user may exchange
		   removable disks.  */
		buf_drive = -1;

		/* Copy this word to COMPLETION_BUFFER and do the
		   completion.  */
		grub_memmove (completion_buffer, buf + i, lpos - i);
		completion_buffer[lpos - i] = 0;
		ret = print_completions (is_filename, 1);
		errnum = ERR_NONE;

		if (ret >= 0)
		  {
		    /* Found, so insert COMPLETION_BUFFER.  */
		    cl_insert (completion_buffer + lpos - i);

		    if (ret > 0)
		      {
			/* There is more than one candidates, so print
			   the list.  */

			/* Go to the part after the line here.  */
			yend = ((llen + plen) / 79) + ystart;
			grub_putchar ('\n');
			gotoxy (0, getxy () & 0xff);

			print_completions (is_filename, 0);
			errnum = ERR_NONE;
		      }
		  }

		/* Restore the command-line.  */
		if (equal_pos >= 0)
		  buf[equal_pos] = '=';

		if (ret)
		  cl_init ();
	      }
	      break;
	    case 1:		/* C-a go to beginning of line */
	      lpos = 0;
	      cl_setcpos ();
	      break;
	    case 5:		/* C-e go to end of line */
	      lpos = llen;
	      cl_setcpos ();
	      break;
	    case 6:		/* C-f forward one character */
	      if (lpos < llen)
		{
		  lpos++;
		  cl_setcpos ();
		}
	      break;
	    case 2:		/* C-b backward one character */
	      if (lpos > 0)
		{
		  lpos--;
		  cl_setcpos ();
		}
	      break;
	    case 21:		/* C-u kill to beginning of line */
	      if (lpos == 0)
		break;
	      /* Copy the string being deleted to KILL.  */
	      grub_memmove (kill, buf, lpos);
	      kill[lpos] = 0;
	      grub_memmove (buf, buf + lpos, llen - lpos + 1);
	      lpos = llen - lpos;
	      cl_setcpos ();
	      cl_kill_to_end ();
	      lpos = 0;
	      cl_setcpos ();
	      cl_print (buf, echo_char);
	      cl_setcpos ();
	      break;
	    case 11:		/* C-k kill to end of line */
	      if (lpos == llen)
		break;
	      /* Copy the string being deleted to KILL.  */
	      grub_memmove (kill, buf + lpos, llen - lpos + 1);
	      cl_kill_to_end ();
	      break;
	    case 25:		/* C-y yank the kill buffer */
	      cl_insert (kill);
	      break;
	    case 16:		/* C-p fetch the previous command */
	      {
		char *p;

		if (history < 0)
		  /* Save the working buffer.  */
		  grub_strcpy (cmdline, buf);
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history++;
		p = get_history (history);
		if (! p)
		  {
		    history--;
		    break;
		  }

		lpos = 0;
		cl_setcpos ();
		cl_kill_to_end ();
		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_print (buf, 0);
		cl_setcpos ();
	      }
	      break;
	    case 14:		/* C-n fetch the next command */
	      {
		char *p;

		if (history < 0)
		  {
		    break;
		  }
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history--;
		p = get_history (history);
		if (! p)
		  p = cmdline;

		lpos = 0;
		cl_setcpos ();
		cl_kill_to_end ();
		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_print (buf, 0);
		cl_setcpos ();
	      }
	      break;
	    }
	}

      /* ESC, C-d and C-h are always handled. Actually C-d is not
	 functional if READLINE is zero, as the cursor cannot go
	 backward, but that's ok.  */
      switch (c)
	{
	case 27:	/* ESC immediately return 1 */
	  return 1;
	case 4:		/* C-d delete character under cursor */
	  if (lpos == llen)
	    break;
	  lpos++;
	  /* fallthrough is on purpose! */
	case 8:		/* C-h backspace */
#ifdef GRUB_UTIL
	case 127:	/* also backspace */
#endif
	  if (lpos > 0)
	    {
	      int i;
	      grub_memmove (buf + lpos - 1, buf + lpos, llen - lpos + 1);
	      i = lpos;
	      lpos = llen - 1;
	      cl_setcpos ();
	      putchar (' ');
	      lpos = i - 1;	/* restore lpos and decrement */
	      llen--;
	      cl_setcpos ();
	      if (lpos != llen)
		{
		  cl_print (buf + lpos, echo_char);
		  cl_setcpos ();
		}
	    }
	  break;
	default:		/* insert printable character into line */
	  if (c >= ' ' && c <= '~')
	    {
	      char str[2];

	      str[0] = c;
	      str[1] = 0;
	      cl_insert (str);
	    }
	}
    }

  /* Move the cursor to the end of the command.  */
  lpos = llen;
  cl_setcpos ();
  grub_putchar ('\n');
  gotoxy (0, getxy () & 0xff);

  /* If ECHO_CHAR is NUL, remove the leading spaces.  */
  lpos = 0;
  if (! echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  /* Copy the working buffer to CMDLINE.  */
  grub_memmove (cmdline, buf + lpos, llen - lpos + 1);

  /* If the readline-like feature is turned on and CMDLINE is not
     empty, add it into the history list.  */
  if (readline && lpos < llen)
    add_history (cmdline, 0);

  return 0;
}
#endif /* STAGE1_5 */

int
safe_parse_maxint (char **str_ptr, int *myint_ptr)
{
  char *ptr = *str_ptr;
  int myint = 0;
  int mult = 10, found = 0;

  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      /* A bit tricky. This below makes use of the equivalence:
	 (A >= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      if (myint > ((MAXINT - digit) / mult))
	{
	  errnum = ERR_NUMBER_PARSING;
	  return 0;
	}
      myint = (myint * mult) + digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = myint;

  return 1;
}

int
grub_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return (c + ('a' - 'A'));

  return c;
}

int
grub_isspace (int c)
{
  if (c == ' ' || c == '\t' || c == '\n')
    return 1;

  return 0;
}

#ifndef STAGE1_5
int
grub_memcmp (const char *s1, const char *s2, int n)
{
  while (n)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1++;
      s2++;
      n--;
    }

  return 0;
}

int
grub_strncat (char *s1, const char *s2, int n)
{
  int i = -1;

  while (++i < n && s1[i] != 0);

  while (i < n && (s1[i++] = *(s2++)) != 0);

  s1[n - 1] = 0;

  if (i >= n)
    return 0;

  s1[i] = 0;

  return 1;
}

int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 || *s2)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1 ++;
      s2 ++;
    }

  return 0;
}
#endif /* ! STAGE1_5 */

int
substring (char *s1, char *s2)
{
  while (*s1 == *s2)
    {
      /* The strings match exactly. */
      if (! *(s1++))
	return 0;
      s2 ++;
    }

  /* S1 is a substring of S2. */
  if (*s1 == 0)
    return -1;

  /* S1 isn't a substring. */
  return 1;
}

#ifndef STAGE1_5
/* Terminate the string STR with NUL.  */
int
nul_terminate (char *str)
{
  int ch;
  
  while (*str && ! grub_isspace (*str))
    str++;

  ch = *str;
  *str = 0;
  return ch;
}

char *
grub_strstr (const char *s1, const char *s2)
{
  const char *ptr, *tmp;

  while (*s1)
    {
      ptr = s1;
      tmp = s2;

      while (*s1 && *s1++ == *tmp++);

      if (tmp > s2 && !*(tmp - 1))
	return (char *) ptr;
    }

  return 0;
}

int
grub_strlen (const char *str)
{
  int len = 0;

  while (*str++)
    len++;

  return len;
}
#endif /* ! STAGE1_5 */

int
memcheck (int addr, int len)
{
#ifdef GRUB_UTIL
  static int start_addr (void)
    {
      int ret;
# if defined(HAVE_START_SYMBOL)
      asm volatile ("movl	$start, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_START_SYMBOL)
      asm volatile ("movl	$_start, %0" : "=a" (ret));
# endif
      return ret;
    }

  static int end_addr (void)
    {
      int ret;
# if defined(HAVE_END_SYMBOL)
      asm volatile ("movl	$end, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_END_SYMBOL)
      asm volatile ("movl	$_end, %0" : "=a" (ret));
# endif
      return ret;
    }

  if (start_addr () <= addr && end_addr () > addr + len)
    return ! errnum;
#endif /* GRUB_UTIL */

  if ((addr < RAW_ADDR (0x1000))
      || (addr < RAW_ADDR (0x100000)
	  && RAW_ADDR (mbi.mem_lower * 1024) < (addr + len))
      || (addr >= RAW_ADDR (0x100000)
	  && RAW_ADDR (mbi.mem_upper * 1024) < ((addr - 0x100000) + len)))
    errnum = ERR_WONT_FIT;

  return ! errnum;
}

char *
grub_memmove (char *to, const char *from, int len)
{
   if (memcheck ((int) to, len))
     {
       /* This assembly code is stolen from
	  linux-2.2.2/include/asm-i386/string.h. This is not very fast
	  but compact.  */
       int d0, d1, d2;

       if (to < from)
	 {
	   asm volatile ("cld\n\t"
			 "rep\n\t"
			 "movsb"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),"1" (from),"2" (to)
			 : "memory");
	 }
       else
	 {
	   asm volatile ("std\n\t"
			 "rep\n\t"
			 "movsb\n\t"
			 "cld"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),
			 "1" (len - 1 + from),
			 "2" (len - 1 + to)
			 : "memory");
	 }
     }

   return errnum ? NULL : to;
}

#ifndef STAGE1_5
void *
grub_memset (void *start, int c, int len)
{
  char *p = start;

  if (memcheck ((int) start, len))
    {
      while (len -- > 0)
	*p ++ = c;
    }

  return errnum ? NULL : start;
}

char *
grub_strcpy (char *dest, const char *src)
{
  grub_memmove (dest, src, grub_strlen (src) + 1);
  return dest;
}
#endif /* ! STAGE1_5 */
