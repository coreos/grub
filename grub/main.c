/* main.c - experimental GRUB stage2 that runs under Unix */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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

/* Simulator entry point. */
int grub_stage2 (void);

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

char *program_name = 0;

#define OPT_HELP -2
#define OPT_VERSION -3
#define OPT_HOLD -4
#define OPTSTRING ""

static struct option longopts[] =
{
  {"help", no_argument, 0, OPT_HELP},
  {"version", no_argument, 0, OPT_VERSION},
  {"hold", no_argument, 0, OPT_HOLD},
  {0},
};


static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``%s --help'' for more information.\n",
	     program_name);
  else
    printf ("\
Usage: %s [OPTION]...\n\
\n\
Enter the GRand Unified Bootloader command shell.\n\
\n\
    --help                display this message and exit\n\
    --hold                wait forever so that a debugger may be attached\n\
    --version             print version information and exit\n\
",
	    program_name);

  exit (status);
}


int
main (int argc, char **argv)
{
  int c;
  int hold = 0;
  program_name = argv[0];

  /* Parse command-line options. */
  do
    {
      c = getopt_long (argc, argv, OPTSTRING, longopts, 0);
      switch (c)
	{
	case EOF:
	  /* Fall through the bottom of the loop. */
	  break;

	case OPT_HOLD:
	  hold = 1;
	  break;

	case OPT_HELP:
	  usage (0);
	  break;

	case OPT_VERSION:
	  printf ("GNU GRUB " VERSION "\n");
	  exit (0);
	  break;

	default:
	  usage (1);
	}
    }
  while (c != EOF);

  /* Wait until the HOLD variable is cleared by an attached debugger. */
  while (hold)
    sleep (1);

  /* Transfer control to the stage2 simulator. */
  exit (grub_stage2 ());
}
