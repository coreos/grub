/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <argp.h>
#include <string.h>

#include <pupa/mm.h>
#include <pupa/setjmp.h>
#include <pupa/fs.h>
#include <pupa/i386/pc/util/biosdisk.h>
#include <pupa/dl.h>
#include <pupa/machine/console.h>
#include <pupa/util/misc.h>
#include <pupa/kernel.h>
#include <pupa/normal.h>
#include <pupa/util/getroot.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/pupa"
#else
# define DEFAULT_DIRECTORY	"/boot/pupa"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

/* XXX.  */
pupa_addr_t pupa_end_addr = -1;
pupa_addr_t pupa_total_module_size = 0;

int
pupa_arch_dl_check_header (void *ehdr, pupa_size_t size)
{
  (void) ehdr;
  (void) size;

  return PUPA_ERR_BAD_MODULE;
}

pupa_err_t
pupa_arch_dl_relocate_symbols (pupa_dl_t mod, void *ehdr)
{
  (void) mod;
  (void) ehdr;

  return PUPA_ERR_BAD_MODULE;
}

void
pupa_machine_init (void)
{
  pupa_console_init ();
}


const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;
static char doc[] = "PUPA emulator";

static struct argp_option options[] = {
  {"root-device", 'r', "DEV",  0, "use DEV as the root device [default=guessed]"},
  {"device-map",  'm', "FILE", 0, "use FILE as the device map"},
  {"directory",   'd', "DIR",  0, "use PUPA files in the directory DIR"},
  {"verbose",     'v', 0     , 0, "print verbose messages"},
  { 0 }
};

struct arguments
{
  char *root_dev;
  char *dev_map;
  char *dir;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;
  
  switch (key)
    {
    case 'r':
      args->root_dev = arg;
      break;
    case 'd':
      args->dir = arg;
      break;
    case 'm':
      args->dev_map = arg;
      break;
    case 'v':
      verbosity++;
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {options, parse_opt, 0, doc};


int
main (int argc, char *argv[])
{
  char *prefix = 0;
  char rootprefix[100];
  struct arguments args =
    {
      .dir = DEFAULT_DIRECTORY,
      .dev_map = DEFAULT_DEVICE_MAP
    };

  argp_parse (&argp, argc, argv, 0, 0, &args);

  /* More sure there is a root device.  */
  if (! args.root_dev)
    {
      args.root_dev = pupa_guess_root_device (args.dir ? : DEFAULT_DIRECTORY);
      if (! args.root_dev)
	{
	  pupa_util_info ("guessing the root device failed, because of `%s'",
			  pupa_errmsg);
	  pupa_util_error ("Cannot guess the root device. Specify the option ``--root-device''.");
	}
    }

  prefix = pupa_get_prefix (args.dir ? : DEFAULT_DIRECTORY);
  sprintf (rootprefix, "%s%s", args.root_dev, prefix);
  pupa_dl_set_prefix (rootprefix);
  
  /* XXX: This is a bit unportable.  */
  pupa_util_biosdisk_init (args.dev_map);

  /* Initialize the default modules.  */
  pupa_fat_init ();
  pupa_ext2_init ();

  /* XXX: Should normal mode be started by default?  */
  pupa_normal_init ();

  /* Start PUPA!  */
  pupa_main ();

  pupa_util_biosdisk_fini ();
  pupa_normal_fini ();
  pupa_ext2_fini ();
  pupa_fat_fini ();

  return 0;
}
