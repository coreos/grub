/* argv.c - methods for constructing argument vector */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/script_sh.h>

#define ARG_ALLOCATION_UNIT  (32 * sizeof (char))
#define ARGV_ALLOCATION_UNIT (8 * sizeof (void*))

void
grub_script_argv_free (struct grub_script_argv *argv)
{
  int i;

  if (argv->args)
    {
      for (i = 0; i < argv->argc; i++)
	grub_free (argv->args[i]);

      grub_free (argv->args);
    }

  argv->argc = 0;
  argv->args = 0;
}

/* Prepare for next argc.  */
int
grub_script_argv_next (struct grub_script_argv *argv)
{
  char **p = argv->args;

  if (argv->argc == 0)
    {
      p = grub_malloc (ALIGN_UP (2 * sizeof (char *), ARG_ALLOCATION_UNIT));
      if (! p)
	return 1;

      argv->argc = 1;
      argv->args = p;
      argv->args[0] = 0;
      argv->args[1] = 0;
      return 0;
    }

  if (! argv->args[argv->argc - 1])
    return 0;

  p = grub_realloc (p, ALIGN_UP ((argv->argc + 1) * sizeof (char *),
				 ARG_ALLOCATION_UNIT));
  if (! p)
    return 1;

  argv->argc++;
  argv->args = p;
  argv->args[argv->argc] = 0;
  return 0;
}

/* Append `s' to the last argument.  */
int
grub_script_argv_append (struct grub_script_argv *argv, const char *s)
{
  int a, b;
  char *p = argv->args[argv->argc - 1];

  if (! s)
    return 0;

  a = p ? grub_strlen (p) : 0;
  b = grub_strlen (s);

  p = grub_realloc (p, ALIGN_UP ((a + b + 1) * sizeof (char),
				 ARG_ALLOCATION_UNIT));
  if (! p)
    return 1;

  grub_strcpy (p + a, s);
  argv->args[argv->argc - 1] = p;
  return 0;
}

/* Split `s' and append words as multiple arguments.  */
int
grub_script_argv_split_append (struct grub_script_argv *argv, char *s)
{
  char ch;
  char *p;
  int errors = 0;

  if (! s)
    return 0;

  while (! errors && *s)
    {
      p = s;
      while (*s && ! grub_isspace (*s))
	s++;

      ch = *s;
      *s = '\0';
      errors += grub_script_argv_append (argv, p);
      *s = ch;

      while (*s && grub_isspace (*s))
	s++;

      if (*s)
	errors += grub_script_argv_next (argv);
    }
  return errors;
}
