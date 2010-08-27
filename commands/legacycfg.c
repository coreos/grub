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
  const char *shortdesc;
  const char *longdesc;
};

struct legacy_command legacy_commands[] =
  {
    {"blocklist", "blocklist '%s'\n", 1, {TYPE_FILE}, 0, "FILE",
     "Print the blocklist notation of the file FILE."
    },
    {"boot", "boot\n", 0, {}, 0, 0,
     "Boot the OS/chain-loader which has been loaded."},
    /* bootp unsupported.  */
    {"cat", "cat '%s'\n", 1, {TYPE_FILE}, 0, "FILE",
     "Print the contents of the file FILE."},
    {"chainloader", "chainloader %s '%s'\n", 2, {TYPE_FORCE_OPTION, TYPE_FILE},
     0, "[--force] FILE",
     "Load the chain-loader FILE. If --force is specified, then load it"
     " forcibly, whether the boot loader signature is present or not."},
    {"cmp", "cmp '%s' '%s'\n", 2, {TYPE_FILE, TYPE_FILE}, FLAG_IGNORE_REST,
     "FILE1 FILE2",
     "Compare the file FILE1 with the FILE2 and inform the different values"
     " if any."},
    /* FIXME: Implement command.  */
    {"color", "legacy_color '%s' '%s'\n", 2, {TYPE_VERBATIM, TYPE_VERBATIM},
     FLAG_IGNORE_REST, "NORMAL [HIGHLIGHT]",
     "Change the menu colors. The color NORMAL is used for most"
     " lines in the menu, and the color HIGHLIGHT is used to highlight the"
     " line where the cursor points. If you omit HIGHLIGHT, then the"
     " inverted color of NORMAL is used for the highlighted line."
     " The format of a color is \"FG/BG\". FG and BG are symbolic color names."
     " A symbolic color name must be one of these: black, blue, green,"
     " cyan, red, magenta, brown, light-gray, dark-gray, light-blue,"
     " light-green, light-cyan, light-red, light-magenta, yellow and white."
     " But only the first eight names can be used for BG. You can prefix"
     " \"blink-\" to FG if you want a blinking foreground color."},
    {"configfile", "legacy_configfile '%s'\n", 1, {TYPE_FILE}, 0, "FILE",
     "Load FILE as the configuration file."},
    {"debug",
     "if [ -z \"$debug\" ]; then set debug=all; else set debug=; fi\n",
     0, {}, 0, 0, "Turn on/off the debug mode."},
    {"default",
     "set default='%s'; if [ x\"$default\" = xsaved ]; then load_env; "
     "set default=\"$saved_entry\"; fi\n", 1, {TYPE_VERBATIM}, 0, 
     "[NUM | `saved']",
     "Set the default entry to entry number NUM (if not specified, it is"
     " 0, the first entry) or the entry number saved by savedefault."},
    /* dhcp unsupported.  */
    /* displayapm unsupported.  */
    {"displaymem", "lsmmap\n", 0, {}, 0, 0, 
     "Display what GRUB thinks the system address space map of the"
     " machine is, including all regions of physical RAM installed."},
    /* embed unsupported.  */
    {"fallback", "set fallback='%s'\n", 1, {TYPE_VERBATIM}, 0, "NUM...",
     "Go into unattended boot mode: if the default boot entry has any"
     " errors, instead of waiting for the user to do anything, it"
     " immediately starts over using the NUM entry (same numbering as the"
     " `default' command). This obviously won't help if the machine"
     " was rebooted by a kernel that GRUB loaded."},
    {"find", "search -f '%s'\n", 1, {TYPE_FILE}, 0, "FILENAME",
     "Search for the filename FILENAME in all of partitions and print the list of"
     " the devices which contain the file."},
    /* fstest unsupported.  */
    /* geometry unsupported.  */
    {"halt", "halt %s\n", 1, {TYPE_NOAPM_OPTION}, 0, "[--no-apm]",
     "Halt your system. If APM is available on it, turn off the power using"
     " the APM BIOS, unless you specify the option `--no-apm'."},
    /* help unsupported.  */    /* NUL_TERMINATE */
    /* hiddenmenu unsupported.  */
    {"hide", "parttool '%s' hidden+\n", 1, {TYPE_PARTITION}, 0, "PARTITION",
     "Hide PARTITION by setting the \"hidden\" bit in"
     " its partition type code."},
    /* ifconfig unsupported.  */
    /* impsprobe unsupported.  */
    /* FIXME: Implement command.  */
    {"initrd", "legacy_initrd '%s' %s\n", 2, {TYPE_FILE, TYPE_REST_VERBATIM}, 0,
     "FILE [ARG ...]",
     "Load an initial ramdisk FILE for a Linux format boot image and set the"
     " appropriate parameters in the Linux setup area in memory."},
    /* install unsupported.  */
    /* ioprobe unsupported.  */
    /* FIXME: implement command. */
    {"kernel", "legacy_kernel %s '%s' %s\n", 4, {TYPE_TYPE_OR_NOMEM_OPTION,
						 TYPE_TYPE_OR_NOMEM_OPTION,
						 TYPE_FILE,
						 TYPE_REST_VERBATIM}, 0,
     "[--no-mem-option] [--type=TYPE] FILE [ARG ...]",
     "Attempt to load the primary boot image from FILE. The rest of the"
     " line is passed verbatim as the \"kernel command line\".  Any modules"
     " must be reloaded after using this command. The option --type is used"
     " to suggest what type of kernel to be loaded. TYPE must be either of"
     " \"netbsd\", \"freebsd\", \"openbsd\", \"linux\", \"biglinux\" and"
     " \"multiboot\". The option --no-mem-option tells GRUB not to pass a"
     " Linux's mem option automatically."},
    /* lock is handled separately. */
    {"makeactive", "parttool \"$root\" boot+\n", 0, {}, 0, 0
     "Set the active partition on the root disk to GRUB's root device."
     " This command is limited to _primary_ PC partitions on a hard disk."},
    {"map", "drivemap '%s' '%s'\n", 2, {TYPE_PARTITION, TYPE_PARTITION},
     FLAG_IGNORE_REST, "TO_DRIVE FROM_DRIVE",
     "Map the drive FROM_DRIVE to the drive TO_DRIVE. This is necessary"
     " when you chain-load some operating systems, such as DOS, if such an"
     " OS resides at a non-first drive."},
    /* md5crypt unsupported.  */
    {"module", "legacy_initrd '%s' %s\n", 1, {TYPE_FILE, TYPE_REST_VERBATIM}, 0,
     "FILE [ARG ...]",
     "Load a boot module FILE for a Multiboot format boot image (no"
     " interpretation of the file contents is made, so users of this"
     " command must know what the kernel in question expects). The"
     " rest of the line is passed as the \"module command line\", like"
     " the `kernel' command."},
    /* modulenounzip unsupported.  */
    /* FIXME: allow toggle.  */
    {"pager", "set pager=%d\n", 1, {TYPE_BOOL}, 0, "[FLAG]",
     "Toggle pager mode with no argument. If FLAG is given and its value"
     " is `on', turn on the mode. If FLAG is `off', turn off the mode."},
    /* partnew unsupported.  */
    {"parttype", "parttool '%s' type=%s\n", 2, {TYPE_PARTITION, TYPE_INT}, 0,
     "PART TYPE", "Change the type of the partition PART to TYPE."},
    /* password unsupported.  */    /* NUL_TERMINATE */
    /* pause unsupported.  */
    /* rarp unsupported.  */
    {"read", "read_dword %s\n", 1, {TYPE_INT}, 0, "ADDR",
     "Read a 32-bit value from memory at address ADDR and"
     " display it in hex format."},
    {"reboot", "reboot\n", 0, {}, 0, 0, "Reboot your system."},
    /* FIXME: Support HDBIAS.  */
    {"root", "set root='%s'\n", 1, {TYPE_PARTITION}, 0, "[DEVICE [HDBIAS]]",
     "Set the current \"root device\" to the device DEVICE, then"
     " attempt to mount it to get the partition size (for passing the"
     " partition descriptor in `ES:ESI', used by some chain-loaded"
     " bootloaders), the BSD drive-type (for booting BSD kernels using"
     " their native boot format), and correctly determine "
     " the PC partition where a BSD sub-partition is located. The"
     " optional HDBIAS parameter is a number to tell a BSD kernel"
     " how many BIOS drive numbers are on controllers before the current"
     " one. For example, if there is an IDE disk and a SCSI disk, and your"
     " FreeBSD root partition is on the SCSI disk, then use a `1' for HDBIAS."},
    {"rootnoverify", "set root='%s'\n", 1, {TYPE_PARTITION}, 0,
     "[DEVICE [HDBIAS]]",
     "Similar to `root', but don't attempt to mount the partition. This"
     " is useful for when an OS is outside of the area of the disk that"
     " GRUB can read, but setting the correct root device is still"
     " desired. Note that the items mentioned in `root' which"
     " derived from attempting the mount will NOT work correctly."},
    /* FIXME: support arguments.  */
    {"savedefault", "saved_entry=${chosen}; save_env saved_entry\n", 0, {}, 0,
     "[NUM | `fallback']",
     "Save the current entry as the default boot entry if no argument is"
     " specified. If a number is specified, this number is saved. If"
     " `fallback' is used, next fallback entry is saved."},
    {"serial", "serial %s\n", 1, {TYPE_REST_VERBATIM}, 0, 
     "[--unit=UNIT] [--port=PORT] [--speed=SPEED] [--word=WORD] "
     "[--parity=PARITY] [--stop=STOP] [--device=DEV]",
     "Initialize a serial device. UNIT is a digit that specifies which serial"
     " device is used (e.g. 0 == COM1). If you need to specify the port number,"
     " set it by --port. SPEED is the DTE-DTE speed. WORD is the word length,"
     " PARITY is the type of parity, which is one of `no', `odd' and `even'."
     " STOP is the length of stop bit(s). The option --device can be used only"
     " in the grub shell, which specifies the file name of a tty device. The"
     " default values are COM1, 9600, 8N1."},
    /* setkey unsupported.  */    /* NUL_TERMINATE */
    /* setup unsupported.  */
    /* terminal unsupported.  */    /* NUL_TERMINATE */
    /* terminfo unsupported.  */    /* NUL_TERMINATE */
    /* testload unsupported.  */
    /* testvbe unsupported.  */
    /* tftpserver unsupported.  */
    {"timeout", "set timeout=%s\n", 1, {TYPE_INT}, 0, "SEC",
     "Set a timeout, in SEC seconds, before automatically booting the"
     " default entry (normally the first entry defined)."},
    /* title is handled separately. */
    {"unhide", "parttool '%s' hidden-\n", 1, {TYPE_PARTITION}, 0, "PARTITION",
     "Unhide PARTITION by clearing the \"hidden\" bit in its"
     " partition type code."},
    /* uppermem unsupported.  */
    {"uuid", "search -u '%s'\n", 1, {TYPE_VERBATIM}, 0, "UUID",
     "Find root by UUID"},
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
