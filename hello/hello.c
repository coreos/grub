/* hello.c - test module for dynamic loading */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *  Copyright (C) 2003  NIIBE Yutaka <gniibe@m17n.org>
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/types.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/dl.h>
#include <pupa/normal.h>

static pupa_err_t
pupa_cmd_hello (struct pupa_arg_list *state __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  pupa_printf ("Hello World\n");
  return 0;
}

PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("hello", pupa_cmd_hello, PUPA_COMMAND_FLAG_BOTH,
			 "hello", "Say hello", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("hello");
}
