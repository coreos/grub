/* arg.c - argument parser */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "pupa/arg.h"
#include "pupa/misc.h"
#include "pupa/mm.h"
#include "pupa/err.h"
#include "pupa/normal.h"

/* Build in parser for default options.  */
static const struct pupa_arg_option help_options[] =
  {
    {"help", 'h', 0, "Display help", 0, ARG_TYPE_NONE},
    {"usage", 'u', 0, "Show how to use this command", 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static struct pupa_arg_option *
find_short (const struct pupa_arg_option *options, char c)
{
  struct pupa_arg_option *found = 0;
  auto struct pupa_arg_option *fnd_short (const struct pupa_arg_option *opt);

  struct pupa_arg_option *fnd_short (const struct pupa_arg_option *opt)
    {
      while (opt->doc)
	{
	  if (opt->shortarg == c)
	    return (struct pupa_arg_option *) opt;
	  opt++;
	}
      return 0;
    }

  if (options)
    found = fnd_short (options);
  if (! found)
    found = fnd_short (help_options);
    
  return found;
}

static char *
find_long_option (char *s)
{
  char *argpos = pupa_strchr (s, '=');

  if (argpos)
    {
      *argpos = '\0';
      return ++argpos;
    }
  return 0;
}

static struct pupa_arg_option *
find_long (const struct pupa_arg_option *options, char *s)
{
  struct pupa_arg_option *found = 0;
  auto struct pupa_arg_option *fnd_long (const struct pupa_arg_option *opt);

  struct pupa_arg_option *fnd_long (const struct pupa_arg_option *opt)
    {
      while (opt->doc)
	{
	  if (opt->longarg && !pupa_strcmp (opt->longarg, s))
	    return (struct pupa_arg_option *) opt;
	  opt++;
	}
      return 0;
    }

  if (options)
    found = fnd_long (options);
  if (!found)
    found = fnd_long (help_options);
    
  return found;
}

static void
show_usage (pupa_command_t cmd)
{
  pupa_printf ("Usage: %s\n", cmd->summary);
}

static void
show_help (pupa_command_t cmd)
{
  static void showargs (const struct pupa_arg_option *opt)
    {
      for (; opt->doc; opt++)
	{
	  if (opt->shortarg && pupa_isgraph (opt->shortarg))
	    pupa_printf ("-%c%c ", opt->shortarg, opt->longarg ? ',':' ');
	  else
	    pupa_printf ("    ");
	  if (opt->longarg)
	    {
	      pupa_printf ("--%s", opt->longarg);
	      if (opt->arg)
		pupa_printf ("=%s", opt->arg);
	    }
	  else
	    pupa_printf ("\t");

	  pupa_printf ("\t\t%s\n", opt->doc);
	}
    }  

  show_usage (cmd);
  pupa_printf ("%s\n\n", cmd->description);
  if (cmd->options)
    showargs (cmd->options);
  showargs (help_options);
  pupa_printf ("\nReport bugs to <%s>.\n", PACKAGE_BUGREPORT);
}


static int
parse_option (pupa_command_t cmd, int key, char *arg, struct pupa_arg_list *usr)
{
  switch (key)
    {
    case 'h':
      show_help (cmd);
      return -1;
      
    case 'u':
      show_usage (cmd);
      return -1;

    default:
      {
	int found = -1;
	int i = 0;
	const struct pupa_arg_option *opt = cmd->options;

	while (opt->doc)
	  {
	    if (opt->shortarg && key == opt->shortarg)
	      {
		found = i;
		break;
	      }
	    opt++;
	    i++;
	  }
	
	if (found == -1)
	  return -1;

	usr[found].set = 1;
	usr[found].arg = arg;
      }
    }
  
  return 0;
}

int
pupa_arg_parse (pupa_command_t cmd, int argc, char **argv,
		struct pupa_arg_list *usr, char ***args, int *argnum)
{
  int curarg;
  char *longarg = 0;
  int complete = 0;
  char **argl = 0;
  int num = 0;
  auto pupa_err_t add_arg (char *s);

  pupa_err_t add_arg (char *s)
    {
      argl = pupa_realloc (argl, (++num) * sizeof (char *));
      if (! args)
	return pupa_errno;
      argl[num - 1] = s;
      return 0;
    }


  for (curarg = 0; curarg < argc; curarg++)
    {
      char *arg = argv[curarg];
      struct pupa_arg_option *opt;
      char *option = 0;

      /* No option is used.  */
      if (arg[0] != '-' || pupa_strlen (arg) == 1)
	{
	  if (add_arg (arg) != 0)
	    goto fail;
  
	  continue;
	}

      /* One or more short options.  */
      if (arg[1] != '-')
	{
	  char *curshort = arg + 1;

	  while (1)
	    {
	      opt = find_short (cmd->options, *curshort);
	      if (!opt)
		{
		  pupa_error (PUPA_ERR_BAD_ARGUMENT,
			      "Unknown argument `-%c'\n", *curshort);
		  goto fail;
		}
	      
	      curshort++;

	      /* Parse all arguments here except the last one because
		 it can have an argument value.  */
	      if (*curshort)
		{
		  if (parse_option (cmd, opt->shortarg, 0, usr) || pupa_errno)
		    goto fail;
		}
	      else
		{
		  if (opt->type != ARG_TYPE_NONE)
		    {
		      if (curarg + 1 < argc)
			{
			  char *nextarg = argv[curarg + 1];
			  if (!(opt->flags & PUPA_ARG_OPTION_OPTIONAL) 
			      || (pupa_strlen (nextarg) < 2 || nextarg[0] != '-'))
			    option = argv[++curarg];
			}
		    }
		  break;
		}
	    }
	  
	}
      else /* The argument starts with "--".  */
	{
	  /* If the argument "--" is used just pass the other
	     arguments.  */
	  if (pupa_strlen (arg) == 2)
	    {
	      for (curarg++; curarg < argc; curarg++)
		if (add_arg (arg) != 0)
		  goto fail;
	      break;
	    }

	  longarg = (char *) pupa_strdup (arg);
	  if (! longarg)
	    goto fail;

	  option = find_long_option (longarg);
	  arg = longarg;

	  opt = find_long (cmd->options, arg + 2);
	  if (!opt)
	    {
	      pupa_error (PUPA_ERR_BAD_ARGUMENT, "Unknown argument `%s'\n", arg);
	      goto fail;
	    }
	}

      if (! (opt->type == ARG_TYPE_NONE 
	     || (!option && (opt->flags & PUPA_ARG_OPTION_OPTIONAL))))
	{
	  if (!option)
	    {
	      pupa_error (PUPA_ERR_BAD_ARGUMENT, 
			  "Missing mandatory option for `%s'\n", opt->longarg);
	      goto fail;
	    }
	  
	  switch (opt->type)
	    {
	    case ARG_TYPE_NONE:
	      /* This will never happen.  */
	      break;
	      
	    case ARG_TYPE_STRING:
		  /* No need to do anything.  */
	      break;
	      
	    case ARG_TYPE_INT:
	      {
		char *tail;
		
		pupa_strtoul (option, &tail, 0);
		if (tail == 0 || tail == option || *tail != '\0' || pupa_errno)
		  {
		    pupa_error (PUPA_ERR_BAD_ARGUMENT, 
				"The argument `%s' requires an integer.", 
				arg);

		    goto fail;
		  }
		break;
	      }
	      
	    case ARG_TYPE_DEVICE:
	    case ARG_TYPE_DIR:
	    case ARG_TYPE_FILE:
	    case ARG_TYPE_PATHNAME:
	      /* XXX: Not implemented.  */
	      break;
	    }
	  if (parse_option (cmd, opt->shortarg, option, usr) || pupa_errno)
	    goto fail;
	}
      else
	{
	  if (option)
	    {
	      pupa_error (PUPA_ERR_BAD_ARGUMENT, 
			  "A value was assigned to the argument `%s' while it "
			  "doesn't require an argument\n", arg);
	      goto fail;
	    }

	  if (parse_option (cmd, opt->shortarg, 0, usr) || pupa_errno)
	    goto fail;
	}
      pupa_free (longarg);
      longarg = 0;
    }      

  complete = 1;

  *args = argl;
  *argnum = num;

 fail:
  pupa_free (longarg);
 
  return complete;
}
