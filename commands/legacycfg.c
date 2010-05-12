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

struct legacy_command
{
  const char *name;
  const char *map;
  unsigned argc;
  enum arg_type {
    TYPE_VERBATIM,
    TYPE_FORCE_OPTION,
    TYPE_NOAPM_OPTION,
    TYPE_TYPE_OPTION,
    TYPE_FILE,
    TYPE_PARTITION,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_REST_VERBATIM
  } argt[3];
  enum {
    FLAG_IGNORE_REST = 1
  } flags;
};

struct legacy_command legacy_commands[] =
  {
    {"blocklist", "blocklist '%s'\n", 1, {TYPE_FILE}, 0},
    {"boot", "boot\n", 0, {}, 0},
    /* bootp unsupported.  */
    {"cat", "cat '%s'\n", 1, {TYPE_FILE}, 0},
    {"chainloader", "chainloader %s '%s'\n", 2, {TYPE_FORCE_OPTION, TYPE_FILE},
     0},
    {"cmp", "cmp '%s' '%s'\n", 2, {TYPE_FILE, TYPE_FILE}, FLAG_IGNORE_REST},
    /* FIXME: Implement command.  */
    {"color", "legacy_color '%s' '%s'\n", 2, {TYPE_VERBATIM, TYPE_VERBATIM},
     FLAG_IGNORE_REST},
    {"configfile", "legacy_configfile '%s'\n", 1, {TYPE_FILE}, 0},
    {"debug",
     "if [ -z \"$debug\" ]; then set debug=all; else set debug=; fi\n",
     0, {}, 0},
    {"default", "set default='%s'; if [ x\"$default\" = xsaved ]; then load_env; set default=\"$saved_entry\"; fi\n", 1, {TYPE_VERBATIM}, 0},
    /* dhcp unsupported.  */
    /* displayapm unsupported.  */
    {"displaymem", "lsmmap\n", 0, {}, 0},
    /* embed unsupported.  */
    {"fallback", "set fallback='%s'\n", 1, {TYPE_VERBATIM}, 0},
    {"find", "search -f '%s'\n", 1, {TYPE_FILE}, 0},
    /* fstest unsupported.  */
    /* geometry unsupported.  */
    {"halt", "halt %s\n", 1, {TYPE_NOAPM_OPTION}, 0},
    /* help unsupported.  */    /* NUL_TERMINATE */
    /* hiddenmenu unsupported.  */
    {"hide", "parttool '%s' hidden+\n", 1, {TYPE_PARTITION}, 0},
    /* ifconfig unsupported.  */
    /* impsprobe unsupported.  */
    /* FIXME: Implement command.  */
    {"initrd", "legacy_initrd '%s'\n", 1, {TYPE_FILE}, 0},
    /* install unsupported.  */
    /* ioprobe unsupported.  */
    /* FIXME: implement command. */
    {"kernel", "legacy_kernel %s '%s' %s\n", 3, {TYPE_TYPE_OPTION, TYPE_FILE,
						 TYPE_REST_VERBATIM}, 0},
    /* lock is handled separately. */
    {"makeactive", "parttool '%s' boot+\n", 1, {TYPE_PARTITION}, 0},
    {"map", "drivemap '%s' '%s'\n", 2, {TYPE_PARTITION, TYPE_PARTITION},
     FLAG_IGNORE_REST},
    /* md5crypt unsupported.  */
    {"module", "legacy_initrd '%s'\n", 1, {TYPE_FILE}, 0},
    /* modulenounzip unsupported.  */
    {"pager", "set pager=%d\n", 1, {TYPE_BOOL}, 0},
    /* partnew unsupported.  */
    {"parttype", "parttool '%s' type=%s\n", 2, {TYPE_PARTITION, TYPE_INT}, 0},
    /* password unsupported.  */    /* NUL_TERMINATE */
    /* pause unsupported.  */
    /* rarp unsupported.  */
    {"read", "read_dword %s\n", 1, {TYPE_INT}, 0},
    {"reboot", "reboot\n", 0, {}, 0},
    {"root", "set root='%s'\n", 1, {TYPE_PARTITION}, 0},
    {"rootnoverify", "set root='%s'\n", 1, {TYPE_PARTITION}, 0},
    {"savedefault", "saved_entry=${chosen}; save_env saved_entry\n", 0, {}, 0},
    {"serial", "serial %s\n", 1, {TYPE_REST_VERBATIM}, 0},
    /* setkey unsupported.  */    /* NUL_TERMINATE */
    /* setup unsupported.  */
    /* terminal unsupported.  */    /* NUL_TERMINATE */
    /* terminfo unsupported.  */    /* NUL_TERMINATE */
    /* testload unsupported.  */
    /* testvbe unsupported.  */
    /* tftpserver unsupported.  */
    {"timeout", "set timeout=%s\n", 1, {TYPE_INT}, 0},
    /* title is handled separately. */
    {"unhide", "parttool '%s' hidden-\n", 1, {TYPE_PARTITION}, 0},
    /* uppermem unsupported.  */
    {"uuid", "search -u '%s'\n", 1, {TYPE_VERBATIM}, 0},
    /* vbeprobe unsupported.  */
  };

static char *
escape (const char *in)
{
  const char *ptr;
  char *ret, *outptr;
  int overhead = 0;
  for (ptr = in; *ptr; ptr++)
    if (*ptr == '\'' || *ptr == '\\')
      overhead++;
  ret = grub_malloc (ptr - in + overhead);
  if (!ret)
    return NULL;
  outptr = ret;
  for (ptr = in; *ptr; ptr++)
    {
      if (*ptr == '\'' || *ptr == '\\')
	*outptr++ = '\\';
      
      *outptr++ = *ptr;
    }
  *outptr++ = 0;
  return ret;
}

static char *
adjust_file (const char *in)
{
  const char *comma, *ptr, *rest;
  char *ret, *outptr;
  int overhead = 0;
  int part;
  if (in[0] != '(')
    return escape (in);
  for (ptr = in + 1; *ptr && *ptr != ')' && *ptr != ','; ptr++)
    if (*ptr == '\'' || *ptr == '\\')
      overhead++;
  comma = ptr;
  if (*comma != ',')
    return escape (in);
  part = grub_strtoull (comma + 1, (char **) &rest, 0);
  for (ptr = rest; *ptr; ptr++)
    if (*ptr == '\'' || *ptr == '\\')
      overhead++;

  /* 30 is enough for any number.  */
  ret = grub_malloc (ptr - in + overhead + 30);
  if (!ret)
    return NULL;

  outptr = ret;
  for (ptr = in; ptr <= comma; ptr++)
    {
      if (*ptr == '\'' || *ptr == '\\')
	*outptr++ = '\\';
      
      *outptr++ = *ptr;
    }
  grub_snprintf (outptr, 30, "%d", part + 1);
  while (*outptr)
    outptr++;
  for (ptr = rest; ptr <= comma; ptr++)
    {
      if (*ptr == '\'' || *ptr == '\\')
	*outptr++ = '\\';
      
      *outptr++ = *ptr;
    }
  return ret;
}

static int
is_option (enum arg_type opt, const char *curarg)
{
  switch (opt)
    {
    case TYPE_NOAPM_OPTION:
      return grub_strcmp (curarg, "--no-apm") == 0;
    case TYPE_FORCE_OPTION:
      return grub_strcmp (curarg, "--force") == 0;
    case TYPE_TYPE_OPTION:
      return grub_strcmp (curarg, "--type=netbsd") == 0
	|| grub_strcmp (curarg, "--type=freebsd") == 0
	|| grub_strcmp (curarg, "--type=openbsd") == 0
	|| grub_strcmp (curarg, "--type=linux") == 0
	|| grub_strcmp (curarg, "--type=biglinux") == 0
	|| grub_strcmp (curarg, "--type=multiboot") == 0;
    default:
      return 0;
    } 
}

static char *
legacy_parse (char *buf, char **entryname)
{
  char *ptr;
  char *cmdname;
  unsigned i, cmdnum;

  for (ptr = buf; *ptr && grub_isspace (*ptr); ptr++);
  if ((!*ptr || *ptr == '#') && entryname && *entryname)
    {
      char *ret = grub_xasprintf ("%s\n", buf);
      grub_free (buf);
      return ret;
    }
  if (!*ptr || *ptr == '#')
    {
      grub_free (buf);
      return NULL;
    }

  cmdname = ptr;
  for (ptr = buf; *ptr && !grub_isspace (*ptr) && *ptr != '='; ptr++);

  if (entryname && grub_strncmp ("title", cmdname, ptr - cmdname) == 0
      && ptr - cmdname == sizeof ("title") - 1)
    {
      for (; grub_isspace (*ptr) || *ptr == '='; ptr++);
      *entryname = grub_strdup (ptr);
      grub_free (buf);
      return NULL;
    }

  if (grub_strncmp ("lock", cmdname, ptr - cmdname) == 0
      && ptr - cmdname == sizeof ("lock") - 1)
    {
      /* FIXME */
    }

  for (cmdnum = 0; cmdnum < ARRAY_SIZE (legacy_commands); cmdnum++)
    if (grub_strncmp (legacy_commands[cmdnum].name, cmdname, ptr - cmdname) == 0
	&& legacy_commands[cmdnum].name[ptr - cmdname] == 0)
      break;
  if (cmdnum == ARRAY_SIZE (legacy_commands))
    return grub_xasprintf ("# Unsupported legacy command: %s\n", buf);

  for (; grub_isspace (*ptr) || *ptr == '='; ptr++);

  char *args[ARRAY_SIZE (legacy_commands[0].argt)];
  memset (args, 0, sizeof (args));

  {
    unsigned j = 0;
    for (i = 0; i < legacy_commands[cmdnum].argc; i++)
      {
	char *curarg, *cptr = NULL, c = 0;
	for (; grub_isspace (*ptr); ptr++);
	curarg = ptr;
	for (; !grub_isspace (*ptr); ptr++);
	if (i != legacy_commands[cmdnum].argc - 1
	    || (legacy_commands[cmdnum].flags & FLAG_IGNORE_REST))
	  {
	    cptr = ptr;
	    c = *cptr;
	    *ptr = 0;
	  }
	if (*ptr)
	  ptr++;
	switch (legacy_commands[cmdnum].argt[i])
	  {
	  case TYPE_PARTITION:
	  case TYPE_FILE:
	    args[j++] = adjust_file (curarg);
	    break;

	  case TYPE_REST_VERBATIM:
	    {
	      char *outptr, *outptr0;
	      int overhead = 3;
	      ptr = curarg;
	      while (*ptr)
		{
		  for (; grub_isspace (*ptr); ptr++);
		  for (; *ptr && !grub_isspace (*ptr); ptr++)
		    if (*ptr == '\\' || *ptr == '\'')
		      overhead++;
		  if (*ptr)
		    ptr++;
		  overhead += 3;
		}
	      outptr0 = args[j++] = grub_malloc (overhead + (ptr - curarg));
	      if (!outptr0)
		{
		  grub_free (buf);
		  return NULL;
		}
	      ptr = curarg;
	      outptr = outptr0;
	      while (*ptr)
		{
		  for (; grub_isspace (*ptr); ptr++);
		  if (outptr != outptr0)
		    *outptr++ = ' ';
		  *outptr++ = '\'';
		  for (; *ptr && !grub_isspace (*ptr); ptr++)
		    {
		      if (*ptr == '\\' || *ptr == '\'')
			*outptr++ = '\\';
		      *outptr++ = *ptr;
		    }
		  *outptr++ = '\'';
		  if (*ptr)
		    ptr++;
		  overhead += 3;
		}
	      *outptr++ = 0;
	    }
	    break;

	  case TYPE_VERBATIM:
	    args[j++] = escape (curarg);
	    break;
	  case TYPE_FORCE_OPTION:
	  case TYPE_NOAPM_OPTION:
	  case TYPE_TYPE_OPTION:
	    if (is_option (legacy_commands[cmdnum].argt[i], curarg))
	      {
		args[j++] = grub_strdup (curarg);
		break;
	      }
	    if (cptr)
	      *cptr = c;
	    ptr = curarg;
	    args[j++] = "";
	    break;
	  case TYPE_INT:
	    {
	      char *brk;
	      int base = 10;
	      brk = curarg;
	      if (brk[0] == '0' && brk[1] == 'x')
		base = 16;
	      else if (brk[0] == '0')
		base = 8;
	      for (; *brk; brk++)
		{
		  if (base == 8 &&  (*brk == '8' || *brk == '9'))
		    break;
		  if (grub_isdigit (*brk))
		    continue;
		  if (base != 16)
		    break;
		  if (!(*brk >= 'a' && *brk <= 'f')
		      && !(*brk >= 'A' && *brk <= 'F'))
		    break;
		}
	      if (brk == curarg)
		args[j++] = grub_strdup ("0");
	      else
		args[j++] = grub_strndup (curarg, brk - curarg);
	    }
	    break;
	  case TYPE_BOOL:
	    if (curarg[0] == 'o' && curarg[1] == 'n'
		&& (curarg[2] == 0 || grub_isspace (curarg[2])))
	      args[j++] = grub_strdup ("1");
	    else
	      args[j++] = grub_strdup ("0");
	    break;
	  }
      }
  }
  grub_free (buf);
  return grub_xasprintf (legacy_commands[cmdnum].map, args[0], args[1], args[2]);
}

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
	parsed = legacy_parse (buf, &entryname);
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
