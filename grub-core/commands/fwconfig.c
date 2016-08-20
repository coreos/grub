/* fwconfig.c - command to read config from qemu fwconfig  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2015  CoreOS, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/env.h>
#include <grub/cpu/io.h>
#include <grub/i18n.h>
#include <grub/mm.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define SELECTOR 0x510
#define DATA 0x511

#define SIGNATURE_INDEX 0x00
#define DIRECTORY_INDEX 0x19

static grub_extcmd_t cmd_read_fwconfig;

struct grub_qemu_fwcfgfile {
  grub_uint32_t size;
  grub_uint16_t select;
  grub_uint16_t reserved;
  char name[56];
};

static const struct grub_arg_option options[] =
  {
    {0, 'v', 0, N_("Save read value into variable VARNAME."),
     N_("VARNAME"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_fwconfig (grub_extcmd_context_t ctxt __attribute__ ((unused)),
                   int argc, char **argv)
{
  grub_uint32_t i, j, value = 0;
  struct grub_qemu_fwcfgfile file;
  char fwsig[4], signature[4] = { 'Q', 'E', 'M', 'U' };

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  /* Verify that we have meaningful hardware here */
  grub_outw(SIGNATURE_INDEX, SELECTOR);
  for (i=0; i<sizeof(fwsig); i++)
      fwsig[i] = grub_inb(DATA);

  if (grub_memcmp(fwsig, signature, sizeof(signature)) != 0)
    return grub_error (GRUB_ERR_BAD_DEVICE, N_("invalid fwconfig hardware signature: got 0x%x%x%x%x"), fwsig[0], fwsig[1], fwsig[2], fwsig[3]);

  /* Find out how many file entries we have */
  grub_outw(DIRECTORY_INDEX, SELECTOR);
  value = grub_inb(DATA) | grub_inb(DATA) << 8 | grub_inb(DATA) << 16 | grub_inb(DATA) << 24;
  value = grub_be_to_cpu32(value);
  /* Read the file description for each file */
  for (i=0; i<value; i++)
    {
      for (j=0; j<sizeof(file); j++)
	{
	  ((char *)&file)[j] = grub_inb(DATA);
	}
      /* Check whether it matches what we're looking for, and if so read the file */
      if (grub_strncmp(file.name, argv[0], sizeof(file.name)) == 0)
	{
	  grub_uint32_t filesize = grub_be_to_cpu32(file.size);
	  grub_uint16_t location = grub_be_to_cpu16(file.select);
	  char *data = grub_malloc(filesize+1);

	  if (!data)
	      return grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("can't allocate buffer for data"));

	  grub_outw(location, SELECTOR);
	  for (j=0; j<filesize; j++)
	    {
	      data[j] = grub_inb(DATA);
	    }

	  data[filesize] = '\0';

	  grub_env_set (argv[1], data);

	  grub_free(data);
	  return 0;
	}
    }

  return grub_error (GRUB_ERR_FILE_NOT_FOUND, N_("couldn't find entry %s"), argv[0]);
}

GRUB_MOD_INIT(fwconfig)
{
  cmd_read_fwconfig =
    grub_register_extcmd ("fwconfig", grub_cmd_fwconfig, 0,
			  N_("PATH VAR"),
			  N_("Set VAR to the contents of fwconfig PATH"),
			  options);
}

GRUB_MOD_FINI(fwconfig)
{
  grub_unregister_extcmd (cmd_read_fwconfig);
}
