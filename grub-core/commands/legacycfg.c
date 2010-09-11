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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/gzio.h>
#include <grub/normal.h>
#include <grub/script_sh.h>
#include <grub/i18n.h>
#include <grub/term.h>
#include <grub/legacy_parse.h>

static grub_err_t
legacy_file (const char *filename)
{
  grub_file_t file;
  char *entryname = NULL, *entrysrc = NULL;
  grub_menu_t menu;

  file = grub_gzfile_open (filename, 1);
  if (! file)
    return grub_errno;

  menu = grub_env_get_menu ();
  if (! menu)
    {
      menu = grub_zalloc (sizeof (*menu));
      if (! menu)
	return grub_errno;

      grub_env_set_menu (menu);
    }

  while (1)
    {
      char *buf = grub_file_getline (file);
      char *parsed;

      if (!buf && grub_errno)
	{
	  grub_file_close (file);
	  return grub_errno;
	}

      if (!buf)
	break;

      {
	char *oldname = NULL;

	oldname = entryname;
	parsed = grub_legacy_parse (buf, &entryname);
	grub_free (buf);
	if (oldname != entryname && oldname)
	  {
	    const char **args = grub_malloc (sizeof (args[0]));
	    if (!args)
	      {
		grub_file_close (file);
		return grub_errno;
	      }
	    args[0] = oldname;
	    grub_normal_add_menu_entry (1, args, entrysrc);
	  }
      }

      if (parsed && !entryname)
	{
	  auto grub_err_t getline (char **line, int cont);
	  grub_err_t getline (char **line __attribute__ ((unused)), 
			      int cont __attribute__ ((unused)))
	  {
	    return GRUB_ERR_NONE;
	  }

	  grub_normal_parse_line (parsed, getline);
	  grub_print_error ();
	  grub_free (parsed);
	}
      else if (parsed)
	{
	  if (!entrysrc)
	    entrysrc = parsed;
	  else
	    {
	      char *t;

	      t = entrysrc;
	      entrysrc = grub_realloc (entrysrc, grub_strlen (entrysrc)
				       + grub_strlen (parsed) + 1);
	      if (!entrysrc)
		{
		  grub_free (t);
		  grub_free (parsed);
		  return grub_errno;
		}
	      grub_memcpy (entrysrc + grub_strlen (entrysrc), parsed,
			   grub_strlen (parsed) + 1);
	      grub_free (parsed);
	      parsed = NULL;
	    }
	}
    }
  grub_file_close (file);

  if (entryname)
    {
      const char **args = grub_malloc (sizeof (args[0]));
      if (!args)
	{
	  grub_file_close (file);
	  return grub_errno;
	}
      args[0] = entryname;
      grub_normal_add_menu_entry (1, args, entrysrc);
    }

  if (menu && menu->size)
    grub_show_menu (menu, 1);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_legacy_source (struct grub_command *cmd __attribute__ ((unused)),
			int argc, char **args)
{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");
  return legacy_file (args[0]);
}

static grub_err_t
grub_cmd_legacy_configfile (struct grub_command *cmd __attribute__ ((unused)),
			    int argc, char **args)
{
  grub_err_t ret;
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_cls ();
  grub_env_context_open (1);

  ret = legacy_file (args[0]);
  grub_env_context_close ();

  return ret;
}

static enum
  { 
    GUESS_IT, LINUX, MULTIBOOT, KFREEBSD, KNETBSD, KOPENBSD 
  } kernel_type;

static grub_err_t
grub_cmd_legacy_kernel (struct grub_command *mycmd __attribute__ ((unused)),
			int argc, char **args)
{
  int i;
  int no_mem_option = 0;
  struct grub_command *cmd;
  char **cutargs;
  int cutargc;
  for (i = 0; i < 2; i++)
    {
      /* FIXME: really support this.  */
      if (argc >= 1 && grub_strcmp (args[0], "--no-mem-option") == 0)
	{
	  no_mem_option = 1;
	  argc--;
	  args++;
	  continue;
	}

      /* linux16 handles both zImages and bzImages.   */
      if (argc >= 1 && (grub_strcmp (args[0], "--type=linux") == 0
			|| grub_strcmp (args[0], "--type=biglinux") == 0))
	{
	  kernel_type = LINUX;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && grub_strcmp (args[0], "--type=multiboot") == 0)
	{
	  kernel_type = MULTIBOOT;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && grub_strcmp (args[0], "--type=freebsd") == 0)
	{
	  kernel_type = KFREEBSD;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && grub_strcmp (args[0], "--type=openbsd") == 0)
	{
	  kernel_type = KOPENBSD;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && grub_strcmp (args[0], "--type=netbsd") == 0)
	{
	  kernel_type = KNETBSD;
	  argc--;
	  args++;
	  continue;
	}
    }

  if (argc < 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "filename required");

  cutargs = grub_malloc (sizeof (cutargsp[0]) * (argc - 1));
  cutargc = argc - 1;
  grub_memcpy (cutargs + 1, args + 2, sizeof (cutargsp[0]) * (argc - 2));
  cutargs[0] = args[0];

  do
    {
      /* First try Linux.  */
      if (kernel_type == GUESS_IT || kernel_type == LINUX)
	{
	  cmd = grub_command_find ("linux16");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, cutargc, cutargs))
		{
		  kernel_type = LINUX;
		  return GRUB_ERR_NONE;
		}
	    }
	  grub_errno = GRUB_ERR_NONE;
	}

      /* Then multiboot.  */
      /* FIXME: dublicate multiboot filename. */
      if (kernel_type == GUESS_IT || kernel_type == MULTIBOOT)
	{
	  cmd = grub_command_find ("multiboot");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, argc, args))
		{
		  kernel_type = MULTIBOOT;
		  return GRUB_ERR_NONE;
		}
	    }
	  grub_errno = GRUB_ERR_NONE;
	}

      /* k*BSD didn't really work well with grub-legacy.  */
      if (kernel_type == GUESS_IT || kernel_type == KFREEBSD)
	{
	  cmd = grub_command_find ("kfreebsd");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, cutargc, cutargs))
		{
		  kernel_type = KFREEBSD;
		  return GRUB_ERR_NONE;
		}
	    }
	  grub_errno = GRUB_ERR_NONE;
	}
      if (kernel_type == GUESS_IT || kernel_type == KNETBSD)
	{
	  cmd = grub_command_find ("knetbsd");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, cutargc, cutargs))
		{
		  kernel_type = KNETBSD;
		  return GRUB_ERR_NONE;
		}
	    }
	  grub_errno = GRUB_ERR_NONE;
	}
      if (kernel_type == GUESS_IT || kernel_type == KOPENBSD)
	{
	  cmd = grub_command_find ("kopenbsd");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, cutargc, cutargs))
		{
		  kernel_type = KOPENBSD;
		  return GRUB_ERR_NONE;
		}
	    }
	  grub_errno = GRUB_ERR_NONE;
	}
    }
  while (0);

  return grub_error (GRUB_ERR_BAD_OS, "couldn't load file %s\n",
		     args[0]);
}

static grub_err_t
grub_cmd_legacy_initrd (struct grub_command *mycmd __attribute__ ((unused)),
			int argc, char **args)
{
  struct grub_command *cmd;

  if (kernel_type == LINUX)
    {
      cmd = grub_command_find ("initrd16");
      if (!cmd)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "command initrd16 not found");

      return cmd->func (cmd, argc, args);
    }
  if (kernel_type == MULTIBOOT)
    {
      /* FIXME: dublicate module filename. */
      cmd = grub_command_find ("module");
      if (!cmd)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "command module not found");

      return cmd->func (cmd, argc, args);
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     "no kernel with module support is loaded in legacy way");
}

static grub_err_t
grub_cmd_legacy_color (struct grub_command *mycmd __attribute__ ((unused)),
			int argc, char **args)
{
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "color required");
  grub_env_set ("color_normal", args[0]);
  if (argc >= 2)
    grub_env_set ("color_highlight", args[1]);
  else
    {
      char *slash = grub_strchr (args[0], '/');
      char *invert;
      grub_size_t len;

      len = grub_strlen (args[0]);
      if (!slash)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad color specification %s",
			   args[0]);
      invert = grub_malloc (len + 1);
      if (!invert)
	return grub_errno;
      grub_memcpy (invert, slash + 1, len - (slash - args[0]) - 1);
      invert[len - (slash - args[0]) - 1] = '/'; 
      grub_memcpy (invert + len - (slash - args[0]), args[0], slash - args[0]);
      invert[len] = 0;
      grub_env_set ("color_highlight", invert);
      grub_free (invert);
    }

  return grub_errno;
}

static grub_command_t cmd_source, cmd_configfile, cmd_kernel, cmd_initrd;
static grub_command_t cmd_color;

GRUB_MOD_INIT(legacycfg)
{
  cmd_source = grub_register_command ("legacy_source",
				      grub_cmd_legacy_source,
				      N_("FILE"), N_("Parse legacy config"));
  cmd_kernel = grub_register_command ("legacy_kernel",
				      grub_cmd_legacy_kernel,
				      N_("[--no-mem-option] [--type=TYPE] FILE [ARG ...]"),
				      N_("Simulate grub-legacy kernel command"));

  cmd_initrd = grub_register_command ("legacy_initrd",
				      grub_cmd_legacy_initrd,
				      N_("FILE [ARG ...]"),
				      N_("Simulate grub-legacy initrd command"));
  cmd_configfile = grub_register_command ("legacy_configfile",
					  grub_cmd_legacy_configfile,
					  N_("FILE"),
					  N_("Parse legacy config"));
  cmd_color = grub_register_command ("legacy_color",
				     grub_cmd_legacy_color,
				     N_("NORMAL [HIGHLIGHT]"),
				     N_("Simulate grub-legacy color command"));
}

GRUB_MOD_FINI(legacycfg)
{
  grub_unregister_command (cmd_source);
  grub_unregister_command (cmd_configfile);
  grub_unregister_command (cmd_kernel);
  grub_unregister_command (cmd_initrd);
  grub_unregister_command (cmd_color);
}
