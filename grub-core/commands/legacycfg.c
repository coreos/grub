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

static grub_command_t cmd_source, cmd_configfile;

GRUB_MOD_INIT(legacycfg)
{
  cmd_source = grub_register_command ("legacy_source",
				      grub_cmd_legacy_source,
				      N_("FILE"), N_("Parse legacy config"));
  cmd_configfile = grub_register_command ("legacy_configfile",
					  grub_cmd_legacy_configfile,
					  N_("FILE"),
					  N_("Parse legacy config"));
}

GRUB_MOD_FINI(legacycfg)
{
  grub_unregister_command (cmd_source);
  grub_unregister_command (cmd_configfile);
}
