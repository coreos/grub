/* cat.c - command to show the contents of a file  */
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
#include <pupa/file.h>
#include <pupa/disk.h>
#include <pupa/term.h>
#include <pupa/misc.h>

static pupa_err_t
pupa_cmd_cat (struct pupa_arg_list *state __attribute__ ((unused)),
	      int argc, char **args)

{
  pupa_file_t file;
  char buf[PUPA_DISK_SECTOR_SIZE];
  pupa_ssize_t size;

  if (argc != 1)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "file name required");

  file = pupa_file_open (args[0]);
  if (! file)
    return 0;
  
  while ((size = pupa_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;
      
      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];
	  
	  if (pupa_isprint (c) || pupa_isspace (c))
	    pupa_putchar (c);
	  else
	    {
	      pupa_setcolorstate (PUPA_TERM_COLOR_HIGHLIGHT);
	      pupa_printf ("<%x>", (int) c);
	      pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);
	    }
	}
    }

  pupa_putchar ('\n');
  pupa_refresh ();
  pupa_file_close (file);
  
  return 0;
}


#ifdef PUPA_UTIL
void
pupa_cat_init (void)
{
  pupa_register_command ("cat", pupa_cmd_cat, PUPA_COMMAND_FLAG_BOTH,
			 "cat FILE", "Show the contents of a file", 0);
}

void
pupa_cat_fini (void)
{
  pupa_unregister_command ("cat");
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("cat", pupa_cmd_cat, PUPA_COMMAND_FLAG_BOTH,
			 "cat FILE", "Show the contents of a file", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("cat");
}
#endif /* ! PUPA_UTIL */
