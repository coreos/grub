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

#include <grub/legacy_parse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int
main (int argc, char **argv)
{
  FILE *in, *out;
  char *entryname = NULL;
  char *buf = NULL;
  size_t bufsize = 0;

  if (argc >= 2 && argv[1][0] == '-')
    {
      fprintf (stdout, "Usage: %s [INFILE [OUTFILE]]\n", argv[0]);
      return 0;
    }

  if (argc >= 2)
    {
      in = fopen (argv[1], "r");
      if (!in)
	{
	  fprintf (stderr, "Couldn't open %s for reading: %s\n",
		   argv[1], strerror (errno));
	  return 1;
	}
    }
  else
    in = stdin;

  if (argc >= 3)
    {
      out = fopen (argv[2], "w");
      if (!out)
	{					
	  if (in != stdin)
	    fclose (in);
	  fprintf (stderr, "Couldn't open %s for writing: %s\n",
		   argv[2], strerror (errno));
	  return 1;
	}
    }
  else
    out = stdout;

  while (1)
    {
      char *parsed;

      if (getline (&buf, &bufsize, in) < 0)
	break;

      {
	char *oldname = NULL;

	oldname = entryname;
	parsed = grub_legacy_parse (buf, &entryname);
	if (oldname != entryname && oldname)
	  fprintf (out, "}\n\n");
	if (oldname != entryname)
	  fprintf (out, "menuentry \'%s\' {\n",
		   grub_legacy_escape (entryname, strlen (entryname)));
      }

      if (parsed)
	fprintf (out, "%s%s", entryname ? "  " : "", parsed);
    }

  if (entryname)
    fprintf (out, "}\n\n");


  if (in != stdin)
    fclose (in);
  if (out != stdout)
    fclose (out);

  return 0;
}
