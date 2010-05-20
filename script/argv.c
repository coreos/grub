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
#include <grub/fs.h>
#include <grub/env.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/script_sh.h>

#include <regex.h>

#define ARG_ALLOCATION_UNIT  (32 * sizeof (char))
#define ARGV_ALLOCATION_UNIT (8 * sizeof (void*))

static unsigned
round_up_exp (unsigned v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  if (sizeof (v) > 4)
    v |= v >> 32;

  v++;
  v += (v == 0);

  return v;
}

static inline int regexop (char ch);
static char ** merge (char **lhs, char **rhs);
static char *make_dir (const char *prefix, const char *start, const char *end);
static int make_regex (const char *regex_start, const char *regex_end,
		       regex_t *regexp);
static void split_path (char *path, char **suffix_end, char **regex_end);
static char ** match_devices (const regex_t *regexp);
static char ** match_files (const char *prefix, const char *suffix_start,
			    const char *suffix_end, const regex_t *regexp);
static char ** match_paths_with_escaped_suffix (char **paths,
						const char *suffix_start,
						const char *suffix_end,
						const regex_t *regexp);
static int expand (char *arg, struct grub_script_argv *argv);

void
grub_script_argv_free (struct grub_script_argv *argv)
{
  unsigned i;

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
      p = grub_malloc (2 * sizeof (char *));
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

  p = grub_realloc (p, round_up_exp ((argv->argc + 1) * sizeof (char *)));
  if (! p)
    return 1;

  argv->argc++;
  argv->args = p;
  argv->args[argv->argc] = 0;
  return 0;
}

enum append_type {
  APPEND_RAW,
  APPEND_ESCAPED,
  APPEND_UNESCAPED
};

static int
append (struct grub_script_argv *argv, const char *s, enum append_type type)
{
  int a;
  int b;
  char ch;
  char *p = argv->args[argv->argc - 1];

  if (! s)
    return 0;

  a = p ? grub_strlen (p) : 0;
  b = grub_strlen (s) * (type == APPEND_ESCAPED ? 2 : 1);

  p = grub_realloc (p, round_up_exp ((a + b + 1) * sizeof (char)));
  if (! p)
    return 1;

  switch (type)
    {
    case APPEND_RAW:
      grub_strcpy (p + a, s);
      break;

    case APPEND_ESCAPED:
      while ((ch = *s++))
	{
	  if (regexop (ch))
	    p[a++] = '\\';
	  p[a++] = ch;
	}
      p[a] = '\0';
      break;

    case APPEND_UNESCAPED:
      while ((ch = *s++))
	{
	  if (ch == '\\' && regexop (*s))
	    p[a++] = *s++;
	  else
	    p[a++] = ch;
	}
      p[a] = '\0';
      break;
    }

  argv->args[argv->argc - 1] = p;
  return 0;
}


/* Append `s' to the last argument.  */
int
grub_script_argv_append (struct grub_script_argv *argv, const char *s)
{
  return append (argv, s, APPEND_RAW);
}

/* Append `s' to the last argument, but escape any shell regex ops.  */
int
grub_script_argv_append_escaped (struct grub_script_argv *argv, const char *s)
{
  return append (argv, s, APPEND_ESCAPED);
}

/* Append `s' to the last argument, but unescape any escaped shell regex ops.  */
int
grub_script_argv_append_unescaped (struct grub_script_argv *argv, const char *s)
{
  return append (argv, s, APPEND_UNESCAPED);
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

/* Expand `argv' as per shell expansion rules.  */
int
grub_script_argv_expand (struct grub_script_argv *argv)
{
  int i;
  struct grub_script_argv result = { 0, 0 };

  for (i = 0; argv->args[i]; i++)
    if (expand (argv->args[i], &result))
      goto fail;

  grub_script_argv_free (argv);
  *argv = result;
  return 0;

 fail:

  grub_script_argv_free (&result);
  return 1;
}

static char **
merge (char **dest, char **ps)
{
  int i;
  int j;
  char **p;

  if (! dest)
    return ps;

  if (! ps)
    return dest;

  for (i = 0; dest[i]; i++)
    ;
  for (j = 0; ps[j]; j++)
    ;

  p = grub_realloc (dest, sizeof (char*) * (i + j + 1));
  if (! p)
    {
      grub_free (dest);
      grub_free (ps);
      return 0;
    }

  for (j = 0; ps[j]; j++)
    dest[i++] = ps[j];
  dest[i] = 0;

  grub_free (ps);
  return dest;
}

static inline int
regexop (char ch)
{
  return grub_strchr ("*.\\", ch) ? 1 : 0;
}

static char *
make_dir (const char *prefix, const char *start, const char *end)
{
  char ch;
  unsigned i;
  unsigned n;
  char *result;

  i = grub_strlen (prefix);
  n = i + end - start;

  result = grub_malloc (n + 1);
  if (! result)
    return 0;

  grub_strcpy (result, prefix);
  while (start < end && (ch = *start++))
    if (ch == '\\' && regexop (*start))
      result[i++] = *start++;
    else
      result[i++] = ch;

  result[i] = '\0';
  return result;
}

static int
make_regex (const char *start, const char *end, regex_t *regexp)
{
  char ch;
  int i = 0;
  unsigned len = end - start;
  char *buffer = grub_malloc (len * 2 + 1); /* worst case size. */

  while (start < end)
    {
      /* XXX Only * expansion for now.  */
      switch ((ch = *start++))
	{
	case '\\':
	  buffer[i++] = ch;
	  if (*start != '\0')
	    buffer[i++] = *start++;
	  break;

	case '.':
	  buffer[i++] = '\\';
	  buffer[i++] = '.';
	  break;

	case '*':
	  buffer[i++] = '.';
	  buffer[i++] = '*';
	  break;

	default:
	  buffer[i++] = ch;
	}
    }
  buffer[i] = '\0';

  if (regcomp (regexp, buffer, RE_SYNTAX_GNU_AWK))
    {
      grub_free (buffer);
      return 1;
    }

  grub_free (buffer);
  return 0;
}

static void
split_path (char *str, char **suffix_end, char **regex_end)
{
  char ch = 0;
  int regex = 0;

  char *end;
  char *split;

  split = end = str;
  while ((ch = *end))
    {
      if (ch == '\\' && end[1])
	end++;
      else if (regexop (ch))
	regex = 1;
      else if (ch == '/' && ! regex)
	split = end + 1;
      else if (ch == '/' && regex)
	break;

      end++;
    }

  *regex_end = end;
  if (! regex)
    *suffix_end = end;
  else
    *suffix_end = split;
}

static char **
match_devices (const regex_t *regexp)
{
  int i;
  int ndev;
  char **devs;

  auto int match (const char *name);
  int match (const char *name)
  {
    char **t;
    char *buffer = grub_xasprintf ("(%s)", name);
    if (! buffer)
      return 1;

    grub_dprintf ("expand", "matching: %s\n", buffer);
    if (regexec (regexp, buffer, 0, 0, 0))
      {
	grub_free (buffer);
	return 0;
      }

    t = grub_realloc (devs, sizeof (char*) * (ndev + 2));
    if (! t)
      return 1;

    devs = t;
    devs[ndev++] = buffer;
    devs[ndev] = 0;
    return 0;
  }

  ndev = 0;
  devs = 0;

  if (grub_device_iterate (match))
    goto fail;

  return devs;

 fail:

  for (i = 0; devs && devs[i]; i++)
    grub_free (devs[i]);

  if (devs)
    grub_free (devs);

  return 0;
}

static char **
match_files (const char *prefix, const char *suffix, const char *end,
	     const regex_t *regexp)
{
  int i;
  int error;
  char **files;
  unsigned nfile;
  char *dir;
  const char *path;
  char *device_name;
  grub_fs_t fs;
  grub_device_t dev;

  auto int match (const char *name, const struct grub_dirhook_info *info);
  int match (const char *name, const struct grub_dirhook_info *info)
  {
    char **t;
    char *buffer;

    /* skip hidden files, . and .. */
    if (name[0] == '.')
      return 0;

    grub_dprintf ("expand", "matching: %s in %s\n", name, dir);
    if (regexec (regexp, name, 0, 0, 0))
      return 0;

    buffer = grub_xasprintf ("%s%s", dir, name);
    if (! buffer)
      return 1;

    t = grub_realloc (files, sizeof (char*) * (nfile + 2));
    if (! t)
      {
	grub_free (buffer);
	return 1;
      }

    files = t;
    files[nfile++] = buffer;
    files[nfile] = 0;
    return 0;
  }

  nfile = 0;
  files = 0;
  dev = 0;
  device_name = 0;
  grub_error_push ();

  dir = make_dir (prefix, suffix, end);
  if (! dir)
    goto fail;

  device_name = grub_file_get_device_name (dir);
  dev = grub_device_open (device_name);
  if (! dev)
    goto fail;

  fs = grub_fs_probe (dev);
  if (! fs)
    goto fail;

  path = grub_strchr (dir, ')');
  if (! path)
    goto fail;
  path++;

  if (fs->dir (dev, path, match))
    goto fail;

  grub_free (dir);
  grub_device_close (dev);
  grub_free (device_name);
  grub_error_pop ();
  return files;

 fail:

  if (dir)
    grub_free (dir);

  for (i = 0; files && files[i]; i++)
    grub_free (files[i]);

  if (files)
    grub_free (files);

  if (dev)
    grub_device_close (dev);

  if (device_name)
    grub_free (device_name);

  grub_error_pop ();
  return 0;
}

static char **
match_paths_with_escaped_suffix (char **paths,
				 const char *suffix, const char *end,
				 const regex_t *regexp)
{
  if (paths == 0 && suffix == end)
    return match_devices (regexp);

  else if (paths == 0 && suffix[0] == '(')
    return match_files ("", suffix, end, regexp);

  else if (paths == 0 && suffix[0] == '/')
    {
      char **r;
      unsigned n;
      char *root;
      char *prefix;

      root = grub_env_get ("root");
      if (! root)
	return 0;

      prefix = grub_xasprintf ("(%s)", root);
      if (! prefix)
	return 0;

      r = match_files (prefix, suffix, end, regexp);
      grub_free (prefix);
      return r;
    }
  else if (paths)
    {
      int i, j;
      char **r = 0;

      for (i = 0; paths[i]; i++)
	{
	  char **p;

	  p = match_files (paths[i], suffix, end, regexp);
	  if (! p)
	    continue;

	  r = merge (r, p);
	  if (! r)
	    return 0;
	}
      return r;
    }

  return 0;
}

static int
expand (char *arg, struct grub_script_argv *argv)
{
  char *p;
  char *dir;
  char *reg;
  char **paths = 0;

  unsigned i;
  regex_t regex;

  p = arg;
  while (*p)
    {
      /* split `p' into two components: (p..dir), (dir...reg)

	 (p...dir):  path that doesn't need expansion

	 (dir...reg): part of path that needs expansion
       */
      split_path (p, &dir, &reg);
      if (dir < reg)
	{
	  if (make_regex (dir, reg, &regex))
	    goto fail;

	  paths = match_paths_with_escaped_suffix (paths, p, dir, &regex);
	  regfree (&regex);

	  if (! paths)
	    goto done;
	}
      p = reg;
    }

  if (! paths)
    {
      grub_script_argv_next (argv);
      grub_script_argv_append_unescaped (argv, arg);
    }
  else
    for (i = 0; paths[i]; i++)
      {
	grub_script_argv_next (argv);
	grub_script_argv_append (argv, paths[i]);
      }

 done:

  return 0;

 fail:

  regfree (&regex);
  return 1;
}
