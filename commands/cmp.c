/* cmd.c - command to cmp an operating system */
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
#include <pupa/file.h>

static pupa_err_t
pupa_cmd_cmp (struct pupa_arg_list *state __attribute__ ((unused)),
	      int argc, char **args)
{
  pupa_file_t file1;
  pupa_file_t file2;

  if (argc != 2)
    return pupa_error (PUPA_ERR_BAD_ARGUMENT, "two arguments required");

  pupa_printf ("Compare `%s' and `%s':\n", args[0],
	       args[1]);

  file1 = pupa_file_open (args[0]);
  if (! file1)
    return pupa_errno;

  file2 = pupa_file_open (args[1]);
  if (! file2)
    {
      pupa_file_close (file2);
      return pupa_errno;
    }

  if (pupa_file_size (file1) != pupa_file_size (file2))
    pupa_printf ("Differ in size: %d [%s], %d [%s]\n", 
		 pupa_file_size (file1), args[0], 
		 pupa_file_size (file2), args[1]);
  
  else
    {
      char buf1[512];
      char buf2[512];
      pupa_ssize_t rd1, rd2;
      pupa_uint32_t pos = 0;
     
      do
	{
	  int i;
	  rd1 = pupa_file_read (file1, buf1, 512);
	  rd2 = pupa_file_read (file2, buf2, 512);

	  if (rd1 != rd2)
	    return 0;

	  for (i = 0; i < 512; i++)
	    {
	      if (buf1[i] != buf2[i])
		{
		  pupa_printf ("Differ at the offset %d: 0x%x [%s], 0x%x [%s]\n",
			       i + pos, buf1[i], args[0],
			       buf2[i], args[1]);

		  pupa_file_close (file1);
		  pupa_file_close (file2);
		  return 0;
		}
	    }
	  pos += 512;
	  
	} while (rd2);
    }

  pupa_file_close (file1);
  pupa_file_close (file2);

  pupa_printf ("The files are identical.\n");

  return 0;
}


#ifdef PUPA_UTIL
void
pupa_cmp_init (void)
{
  pupa_register_command ("cmp", pupa_cmd_cmp, PUPA_COMMAND_FLAG_BOTH,
			 "cmp FILE1 FILE2", "Compare two files", 0);
}

void
pupa_cmp_fini (void)
{
  pupa_unregister_command ("cmp");
}
#else /* ! PUPA_UTIL */
PUPA_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  pupa_register_command ("cmp", pupa_cmd_cmp, PUPA_COMMAND_FLAG_BOTH,
			 "cmp FILE1 FILE2", "Compare two files", 0);
}

PUPA_MOD_FINI
{
  pupa_unregister_command ("cmp");
}
#endif /* ! PUPA_UTIL */
