/* boot.c - command to boot an operating system */
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

#include <pupa/normal.h>
#include <pupa/dl.h>
#include <pupa/arg.h>
#include <pupa/misc.h>
#include <pupa/loader.h>

static pupa_err_t
pupa_cmd_boot (struct pupa_arg_list *state __attribute__ ((unused)),
	       int argc, char **args __attribute__ ((unused)))
{
  if (argc)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "too many arguments");
  
  pupa_loader_boot ();
  
  return 0;
}


#ifdef PUPA_UTIL
void
pupa_boot_init (void)
{
  pupa_register_command ("boot", pupa_cmd_boot, PUPA_COMMAND_FLAG_BOTH,
			 "boot", "Boot an operating system", 0);
}

void
pupa_boot_fini (void)
{
  pupa_unregister_command ("boot");
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("boot", pupa_cmd_boot, PUPA_COMMAND_FLAG_BOTH,
			 "boot", "Boot an operating system", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("boot");
}
#endif /* ! PUPA_UTIL */
