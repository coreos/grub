/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000, 2001, 2010  Free Software Foundation, Inc.
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
#include <grub/normal.h>
#include <grub/script_sh.h>
#include <grub/i18n.h>
#include <grub/term.h>
#include <grub/legacy_parse.h>
#include <grub/crypto.h>
#include <grub/auth.h>

static grub_err_t
legacy_file (const char *filename)
{
  grub_file_t file;
  char *entryname = NULL, *entrysrc = NULL;
  grub_menu_t menu;
  char *suffix = grub_strdup ("");

  auto grub_err_t getline (char **line, int cont);
  grub_err_t getline (char **line, 
		      int cont __attribute__ ((unused)))
  {
    *line = 0;
    return GRUB_ERR_NONE;
  }

  if (!suffix)
    return grub_errno;

  file = grub_file_open (filename);
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
	int is_suffix;

	oldname = entryname;
	parsed = grub_legacy_parse (buf, &entryname, &is_suffix);
	grub_free (buf);
	if (is_suffix)
	  {
	    char *t;
	    
	    t = suffix;
	    suffix = grub_realloc (suffix, grub_strlen (suffix)
				       + grub_strlen (parsed) + 1);
	    if (!suffix)
	      {
		grub_free (t);
		grub_free (entrysrc);
		grub_free (parsed);
		grub_free (suffix);
		return grub_errno;
	      }
	    grub_memcpy (entrysrc + grub_strlen (entrysrc), parsed,
			 grub_strlen (parsed) + 1);
	    grub_free (parsed);
	    parsed = NULL;
	    continue;
	  }
	if (oldname != entryname && oldname)
	  {
	    const char **args = grub_malloc (sizeof (args[0]));
	    if (!args)
	      {
		grub_file_close (file);
		return grub_errno;
	      }
	    args[0] = oldname;
	    grub_normal_add_menu_entry (1, args, NULL, NULL, NULL, NULL,
					entrysrc);
	  }
      }

      if (parsed && !entryname)
	{
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
		  grub_free (suffix);
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
      grub_normal_add_menu_entry (1, args, NULL, NULL, NULL, NULL, entrysrc);
    }

  grub_normal_parse_line (suffix, getline);
  grub_print_error ();
  grub_free (suffix);

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

  cutargs = grub_malloc (sizeof (cutargs[0]) * (argc - 1));
  cutargc = argc - 1;
  grub_memcpy (cutargs + 1, args + 2, sizeof (cutargs[0]) * (argc - 2));
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
      cmd = grub_command_find ("module");
      if (!cmd)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "command module not found");

      return cmd->func (cmd, argc, args);
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     "no kernel with module support is loaded in legacy way");
}

static grub_err_t
grub_cmd_legacy_initrdnounzip (struct grub_command *mycmd __attribute__ ((unused)),
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
      char **newargs;
      grub_err_t err;
      newargs = grub_malloc ((argc + 1) * sizeof (newargs[0]));
      if (!newargs)
	return grub_errno;
      grub_memcpy (newargs + 1, args, argc * sizeof (newargs[0]));
      newargs[0] = "--nounzip";
      cmd = grub_command_find ("module");
      if (!cmd)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "command module not found");

      err = cmd->func (cmd, argc + 1, newargs);
      grub_free (newargs);
      return err;
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

static grub_err_t
check_password_deny (const char *user __attribute__ ((unused)),
		     const char *entered  __attribute__ ((unused)),
		     void *password __attribute__ ((unused)))
{
  return GRUB_ACCESS_DENIED;
}

#define MD5_HASHLEN 16

struct legacy_md5_password
{
  grub_uint8_t *salt;
  int saltlen;
  grub_uint8_t hash[MD5_HASHLEN];
};

static int
check_password_md5_real (const char *entered,
			 struct legacy_md5_password *pw)
{
  int enteredlen = grub_strlen (entered);
  unsigned char alt_result[MD5_HASHLEN];
  unsigned char *digest;
  grub_uint8_t ctx[GRUB_MD_MD5->contextsize];
  int i;

  GRUB_MD_MD5->init (ctx);
  GRUB_MD_MD5->write (ctx, entered, enteredlen);
  GRUB_MD_MD5->write (ctx, pw->salt + 3, pw->saltlen - 3);
  GRUB_MD_MD5->write (ctx, entered, enteredlen);
  digest = GRUB_MD_MD5->read (ctx);
  GRUB_MD_MD5->final (ctx);
  memcpy (alt_result, digest, MD5_HASHLEN);
  
  GRUB_MD_MD5->init (ctx);
  GRUB_MD_MD5->write (ctx, entered, enteredlen);
  GRUB_MD_MD5->write (ctx, pw->salt, pw->saltlen); /* include the $1$ header */
  for (i = enteredlen; i > 16; i -= 16)
    GRUB_MD_MD5->write (ctx, alt_result, 16);
  GRUB_MD_MD5->write (ctx, alt_result, i);

  for (i = enteredlen; i > 0; i >>= 1)
    GRUB_MD_MD5->write (ctx, entered + ((i & 1) ? enteredlen : 0), 1);
  digest = GRUB_MD_MD5->read (ctx);
  GRUB_MD_MD5->final (ctx);

  for (i = 0; i < 1000; i++)
    {
      memcpy (alt_result, digest, 16);

      GRUB_MD_MD5->init (ctx);
      if ((i & 1) != 0)
	GRUB_MD_MD5->write (ctx, entered, enteredlen);
      else
	GRUB_MD_MD5->write (ctx, alt_result, 16);
      
      if (i % 3 != 0)
	GRUB_MD_MD5->write (ctx, pw->salt + 3, pw->saltlen - 3);

      if (i % 7 != 0)
	GRUB_MD_MD5->write (ctx, entered, enteredlen);

      if ((i & 1) != 0)
	GRUB_MD_MD5->write (ctx, alt_result, 16);
      else
	GRUB_MD_MD5->write (ctx, entered, enteredlen);
      digest = GRUB_MD_MD5->read (ctx);
      GRUB_MD_MD5->final (ctx);
    }

  return (grub_crypto_memcmp (digest, pw->hash, MD5_HASHLEN) == 0);
}

static grub_err_t
check_password_md5 (const char *user,
		    const char *entered,
		    void *password)
{
  if (!check_password_md5_real (entered, password))
    return GRUB_ACCESS_DENIED;

  grub_auth_authenticate (user);

  return GRUB_ERR_NONE;
}

static inline int
ib64t (char c)
{
  if (c == '.')
    return 0;
  if (c == '/')
    return 1;
  if (c >= '0' && c <= '9')
    return c - '0' + 2;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 12;
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 38;
  return -1;
}

static grub_err_t
grub_cmd_legacy_password (struct grub_command *mycmd __attribute__ ((unused)),
			  int argc, char **args)
{
  const char *salt, *saltend;
  const char *p;
  struct legacy_md5_password *pw = NULL;
  int i;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "arguments expected");
  if (args[0][0] != '-' || args[0][1] != '-')
    return grub_normal_set_password ("legacy", args[0]);
  if (grub_memcmp (args[0], "--md5", sizeof ("--md5")) != 0)
    goto fail;
  if (argc == 1)
    goto fail;
  if (grub_strlen(args[1]) <= 3)
    goto fail;
  salt = args[1];
  saltend = grub_strchr (salt + 3, '$');
  if (!saltend)
    goto fail;
  pw = grub_malloc (sizeof (*pw));
  if (!pw)
    goto fail;

  p = saltend + 1;
  for (i = 0; i < 5; i++)
    {
      int n;
      grub_uint32_t w = 0;

      for (n = 0; n < 4; n++)
	{
	  int ww = ib64t(*p++);
	  if (ww == -1)
	    goto fail;
	  w |= ww << (n * 6);
	}
      pw->hash[i == 4 ? 5 : 12+i] = w & 0xff;
      pw->hash[6+i] = (w >> 8) & 0xff;
      pw->hash[i] = (w >> 16) & 0xff;
    }
  {
    int n;
    grub_uint32_t w = 0;
    for (n = 0; n < 2; n++)
      {
	int ww = ib64t(*p++);
	if (ww == -1)
	  goto fail;
	w |= ww << (6 * n);
      }
    if (w >= 0x100)
      goto fail;
    pw->hash[11] = w;
  }

  pw->saltlen = saltend - salt;
  pw->salt = (grub_uint8_t *) grub_strndup (salt, pw->saltlen);
  if (!pw->salt)
    goto fail;

  return grub_auth_register_authentication ("legacy", check_password_md5, pw);

 fail:
  grub_free (pw);
  /* This is to imitate minor difference between grub-legacy in GRUB2.
     If 2 password commands are executed in a row and second one fails
     on GRUB2 the password of first one is used, whereas in grub-legacy
     authenthication is denied. In case of no password command was executed
     early both versions deny any access.  */
  return grub_auth_register_authentication ("legacy", check_password_deny,
					    NULL);
}

static grub_command_t cmd_source, cmd_configfile, cmd_kernel, cmd_initrd;
static grub_command_t cmd_color, cmd_password, cmd_initrdnounzip;

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
  cmd_initrdnounzip = grub_register_command ("legacy_initrd_nounzip",
					     grub_cmd_legacy_initrdnounzip,
					     N_("FILE [ARG ...]"),
					     N_("Simulate grub-legacy modulenounzip command"));

  cmd_configfile = grub_register_command ("legacy_configfile",
					  grub_cmd_legacy_configfile,
					  N_("FILE"),
					  N_("Parse legacy config"));
  cmd_color = grub_register_command ("legacy_color",
				     grub_cmd_legacy_color,
				     N_("NORMAL [HIGHLIGHT]"),
				     N_("Simulate grub-legacy color command"));
  cmd_password = grub_register_command ("legacy_password",
					grub_cmd_legacy_password,
					N_("[--md5] PASSWD [FILE]"),
					N_("Simulate grub-legacy password command"));
}

GRUB_MOD_FINI(legacycfg)
{
  grub_unregister_command (cmd_source);
  grub_unregister_command (cmd_configfile);
  grub_unregister_command (cmd_kernel);
  grub_unregister_command (cmd_initrd);
  grub_unregister_command (cmd_initrdnounzip);
  grub_unregister_command (cmd_color);
  grub_unregister_command (cmd_password);
}
