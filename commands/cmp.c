/* cmd.c - command to cmp an operating system */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/file.h>

static grub_err_t
grub_cmd_cmp (struct grub_arg_list *state __attribute__ ((unused)),
	      int argc, char **args)
{
  grub_file_t file1;
  grub_file_t file2;

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "two arguments required");

  grub_printf ("Compare `%s' and `%s':\n", args[0],
	       args[1]);

  file1 = grub_file_open (args[0]);
  if (! file1)
    return grub_errno;

  file2 = grub_file_open (args[1]);
  if (! file2)
    {
      grub_file_close (file2);
      return grub_errno;
    }

  if (grub_file_size (file1) != grub_file_size (file2))
    grub_printf ("Differ in size: %d [%s], %d [%s]\n", 
		 grub_file_size (file1), args[0], 
		 grub_file_size (file2), args[1]);
  
  else
    {
      char buf1[512];
      char buf2[512];
      grub_ssize_t rd1, rd2;
      grub_uint32_t pos = 0;
     
      do
	{
	  int i;
	  rd1 = grub_file_read (file1, buf1, 512);
	  rd2 = grub_file_read (file2, buf2, 512);

	  if (rd1 != rd2)
	    return 0;

	  for (i = 0; i < 512; i++)
	    {
	      if (buf1[i] != buf2[i])
		{
		  grub_printf ("Differ at the offset %d: 0x%x [%s], 0x%x [%s]\n",
			       i + pos, buf1[i], args[0],
			       buf2[i], args[1]);

		  grub_file_close (file1);
		  grub_file_close (file2);
		  return 0;
		}
	    }
	  pos += 512;
	  
	} while (rd2);
    }

  grub_file_close (file1);
  grub_file_close (file2);

  grub_printf ("The files are identical.\n");

  return 0;
}


#ifdef GRUB_UTIL
void
grub_cmp_init (void)
{
  grub_register_command ("cmp", grub_cmd_cmp, GRUB_COMMAND_FLAG_BOTH,
			 "cmp FILE1 FILE2", "Compare two files", 0);
}

void
grub_cmp_fini (void)
{
  grub_unregister_command ("cmp");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("cmp", grub_cmd_cmp, GRUB_COMMAND_FLAG_BOTH,
			 "cmp FILE1 FILE2", "Compare two files", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("cmp");
}
#endif /* ! GRUB_UTIL */
