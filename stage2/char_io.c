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
    printf ("\n %s\n", err_list[errnum]);
#else /* STAGE1_5 */
    printf ("Error: %u\n", errnum);
#endif /* STAGE1_5 */

  errnum = ERR_NONE;
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

void
init_page (void)
{
  cls ();

  printf ("\n    GRUB  version %s  (%dK lower / %dK upper memory)\n\n",
	  version_string, mbi.mem_lower, mbi.mem_upper);
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
	     int echo_char, int completion)
{
  int ystart, yend, xend, lpos, c;
  int plen = 0;
  int llen = 0;

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
    cl_print (cmdline, echo_char);
    cl_setcpos ();
  }

  /* nested function definition for erasing to the end of the line */
  static void cl_kill_to_end ()
  {
    int i;
    cmdline[lpos] = 0;
    for (i = lpos; i <= llen; i++)
      {
	if (i && ((i + plen) % 79) == 0)
	  putchar (' ');
	putchar (' ');
      }
    llen = lpos;
    cl_setcpos ();
  }

  while (prompt[plen])
    plen++;
  while (cmdline[llen])
    llen++;
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
	default:
	}

      c = ASCII_CHAR (c);

      switch (c)
	{
	case 27:		/* ESC immediately return 1 */
	  return 1;
	case 9:		/* TAB lists completions */
	  if (completion)
	    {
	      int i, j = 0, llen_old = llen;
	      
	      /* Find the first word.  */
	      while (cmdline[j] == ' ')
		j++;
	      while (cmdline[j] && cmdline[j] != '=' && cmdline[j] != ' ')
		j++;
	      
	      /* Since the command line cannot have a '\n', we're OK to
		 use C.  */
	      c = cmdline[lpos];
	      
	      cl_kill_to_end ();
	      
	      /* goto part after line here */
	      yend = ((llen + plen) / 79) + ystart;
	      putchar ('\n');
	      gotoxy (0, getxy () & 0xff);
	      
	      
	      if (lpos > j)
		{
		  for (i = lpos; i > 0 && cmdline[i - 1] != ' '; i--);
		  if (i <= j)
		    i = j + 1;
		  /* print possible completions */
		  print_completions (cmdline + i);
		  /* if somebody in print_completions has added something, 
		     account for that */
		  while (cmdline[lpos])
		    lpos++, llen_old++;
		}
	      else 
		{
		  /* Print the command list.  */
		  struct builtin **builtin;

		  for (builtin = builtin_table; *builtin != 0; builtin++)
		    {
		      /* Do not print the name if it cannot be run in
			 the command-line interface.  */
		      if (! ((*builtin)->flags & BUILTIN_CMDLINE))
			continue;

		      grub_printf ("%s ", (*builtin)->name);
		    }
		}
	      
	      /* restore command-line */
	      cmdline[lpos] = c;
	      llen = llen_old;
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
	      lpos ++;
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
	case 4:		/* C-d delete character under cursor */
	  if (lpos == llen)
	    break;
	  lpos ++;
	  /* fallthrough is on purpose! */
	case 8:		/* C-h backspace */
#ifdef GRUB_UTIL
	case 127:	/* also backspace */
#endif
	  if (lpos > 0)
	    {
	      int i;
	      for (i = lpos - 1; i < llen; i++)
		cmdline[i] = cmdline[i + 1];
	      i = lpos;
	      lpos = llen - 1;
	      cl_setcpos ();
	      putchar (' ');
	      lpos = i - 1;	/* restore lpos and decrement */
	      llen--;
	      cl_setcpos ();
	      if (lpos != llen)
		{
		  cl_print (cmdline + lpos, echo_char);
		  cl_setcpos ();
		}
	    }
	  break;
	case 21:		/* C-u kill to beginning of line */
	  if (lpos == 0)
	    break;
	  {
	    int i;
	    for (i = 0; i < (llen - lpos); i++)
	      cmdline[i] = cmdline[lpos + i];
	  }
	  lpos = llen - lpos;
	  cl_setcpos ();
	  /* fallthrough on purpose! */
	case 11:		/* C-k kill to end of line */
	  if (lpos < llen)
	    {
	      cl_kill_to_end ();
	      if (c == 21)
		{
		  lpos = 0;
		  cl_setcpos ();
		  cl_print (cmdline, echo_char);
		  cl_setcpos ();
		}
	    }
	  break;
	default:		/* insert printable character into line */
	  if (llen < (maxlen - 1) && c >= ' ' && c <= '~')
	    {
	      if (lpos == llen)
		{
		  cmdline[lpos] = c;
		  cmdline[lpos + 1] = 0;
		  cl_print (cmdline + lpos, echo_char);
		  lpos ++;
		  cl_setcpos ();
		}
	      else
		{
		  int i;
		  for (i = llen; i >= lpos; i--)
		    cmdline[i + 1] = cmdline[i];
		  cmdline[lpos] = c;
		  cl_setcpos ();
		  cl_print (cmdline + lpos, echo_char);
		  lpos++;
		  cl_setcpos ();
		}
	      llen++;
	    }
	}
    }

  /* goto part after line here */
  yend = ((llen + plen) / 79) + ystart;
  putchar ('\n');
  gotoxy (0, getxy () & 0xff);

  /* remove leading spaces */
  /* use c and lpos as indexes now */
  for (lpos = 0; cmdline[lpos] == ' '; lpos++);

  if (lpos != 0)
    {
      c = 0;
      do
	{
	  cmdline[c] = cmdline[lpos];
	  c++;
	  lpos++;
	}
      while (cmdline[lpos]);

      /* Zero-terminate the string. */
      cmdline[c] = 0;
    }

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
memcheck (int start, int len)
{
#ifdef GRUB_UTIL
  /* FIXME: cur_part_desc is the only global variable that we memmove
     to.  We should fix this so that we don't need a special case
     (i.e. so that it lives on the stack, or somewhere inside
     grub_scratch_mem). */
  extern char cur_part_desc[];
  if (start >= (int) cur_part_desc && start + len <= (int) cur_part_desc + 16)
    return ! errnum;
#endif /* GRUB_UTIL */

  if ((start < RAW_ADDR (0x1000)) ||
      (start < RAW_ADDR (0x100000) &&
       RAW_ADDR (mbi.mem_lower * 1024) < (start + len)) ||
      (start >= RAW_ADDR (0x100000) &&
       RAW_ADDR (mbi.mem_upper * 1024) < ((start - 0x100000) + len)))
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
#endif /* ! STAGE1_5 */
