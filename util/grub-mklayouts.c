/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <grub/util/misc.h>
#include <grub/i18n.h>
#include <grub/term.h>
#include <grub/keyboard_layouts.h>
#include <grub/at_keyboard.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "progname.h"

#define CKBCOMP "ckbcomp"

static struct option options[] = {
  {"output", required_argument, 0, 'o'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

struct console_grub_equivalence
{
  char *layout;
  grub_uint32_t grub;
};

static struct console_grub_equivalence console_grub_equivalences[] = {
  {"KP_1", '1'},
  {"KP_2", '2'},
  {"KP_3", '3'},
  {"KP_4", '4'},
  {"KP_5", '5'},
  {"KP_6", '6'},
  {"KP_7", '7'},
  {"KP_8", '8'},
  {"KP_9", '9'},

  {"KP_Multiply", '*'},
  {"KP_Substract", '-'},
  {"KP_Add", '+'},
  {"KP_Divide", '/'},

  {"KP_Enter", '\n'},
  {"Return", '\n'},
  {"", '\0'}
};

GRUB_AT_KEY_KEYBOARD_MAP (us_keyboard_map);
GRUB_AT_KEY_KEYBOARD_MAP_SHIFT (us_keyboard_map_shifted);

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTIONS] LAYOUT\n\
  -o, --output		set output base name file. Default is LAYOUT.gkb\n\
  -h, --help		display this message and exit.\n\
  -V, --version		print version information and exit.\n\
  -v, --verbose		print verbose messages.\n\
\n\
Report bugs to <%s>.\n", program_name, PACKAGE_BUGREPORT);

  exit (status);
}

char
lookup (char *code)
{
  int i;

  for (i = 0; console_grub_equivalences[i].grub != '\0'; i++)
    if (strcmp (code, console_grub_equivalences[i].layout) == 0)
      return console_grub_equivalences[i].grub;

  return '\0';
}

unsigned int
get_grub_code (char *layout_code)
{
  unsigned int code;

  if (strncmp (layout_code, "U+", sizeof ("U+") - 1) == 0)
    sscanf (layout_code, "U+%x", &code);
  else if (strncmp (layout_code, "+U+", sizeof ("+U+") - 1) == 0)
    sscanf (layout_code, "+U+%x", &code);
  else
    code = lookup (layout_code);
  return code;
}

void
write_file (char* filename, grub_uint32_t *keyboard_map,
	    grub_uint32_t *keyboard_map_alt)
{
  FILE *fp_output;
  grub_uint32_t version;
  unsigned i;

  version = grub_cpu_to_le32 (GRUB_KEYBOARD_LAYOUTS_VERSION);
  
  for (i = 0; i < GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE; i++)
    keyboard_map[i] = grub_cpu_to_le32 (keyboard_map[i]);

  for (i = 0; i < GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE; i++)
    keyboard_map_alt[i] = grub_cpu_to_le32 (keyboard_map_alt[i]);

  fp_output = fopen (filename, "w");
  
  if (!fp_output)
    {
      grub_util_error ("cannot open `%s'", filename);
      exit (1);
    }

  fwrite (GRUB_KEYBOARD_LAYOUTS_FILEMAGIC, 1,
	  GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE, fp_output);
  fwrite (&version, sizeof (version), 1, fp_output);
  fwrite (keyboard_map, sizeof (keyboard_map[0]),
	  GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE, fp_output);
  fwrite (keyboard_map_alt, sizeof (keyboard_map_alt[0]),
	  GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE, fp_output);
  fclose (fp_output);
}

void
write_keymaps (char *keymap, char *file_basename)
{
  grub_uint32_t keyboard_map[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  grub_uint32_t keyboard_map_alt[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];

  char line[2048];
  pid_t pid;
  int pipe_communication[2];
  int ok;

  FILE *fp_pipe;

  if (pipe (pipe_communication) == -1)
    {
      grub_util_error ("cannot prepare the pipe");
      exit (2);
    }

  pid = fork ();
  if (pid < 0)
    {
      grub_util_error ("cannot fork");
      exit (2);
    }
  else if (pid == 0)
    {
      close (1);
      dup (pipe_communication[1]);
      close (pipe_communication[0]);
      execlp (CKBCOMP, CKBCOMP, keymap, NULL);
      grub_util_error ("%s %s cannot be executed", CKBCOMP, keymap);
      exit (3);
    }
  close (pipe_communication[1]);
  fp_pipe = fdopen (pipe_communication[0], "r");

  memset (keyboard_map, 0, sizeof (keyboard_map));

  /* Process the ckbcomp output and prepare the layouts.  */
  ok = 0;
  while (fgets (line, sizeof (line), fp_pipe))
    {
      if (strncmp (line, "keycode", sizeof ("keycode") - 1) == 0)
	{
	  unsigned keycode;
	  char normal[64];
	  char shift[64];
	  char normalalt[64];
	  char shiftalt[64];

	  sscanf (line, "keycode %u = %60s %60s %60s %60s", &keycode,
		  normal, shift, normalalt, shiftalt);
	  if (keycode < ARRAY_SIZE (us_keyboard_map)
	      && us_keyboard_map[keycode] < ARRAY_SIZE (keyboard_map))
	    {
	      keyboard_map[us_keyboard_map[keycode]] = get_grub_code (normal);
	      keyboard_map_alt[us_keyboard_map[keycode]]
		= get_grub_code (normalalt);
	      ok = 1;
	    }
	  if (keycode < ARRAY_SIZE (us_keyboard_map_shifted)
	      && us_keyboard_map_shifted[keycode] < ARRAY_SIZE (keyboard_map))
	    {
	      keyboard_map[us_keyboard_map_shifted[keycode]]
		= get_grub_code (shift);
	      keyboard_map_alt[us_keyboard_map_shifted[keycode]]
		= get_grub_code (shiftalt);
	      ok = 1;
	    }
	}
    }

  if (ok == 0)
    {
      fprintf (stderr, "ERROR: no keycodes found. Check output of %s %s.\n",
	       CKBCOMP, keymap);
      exit (1);
    }

  write_file (file_basename, keyboard_map, keyboard_map_alt);
}

int
main (int argc, char *argv[])
{
  int verbosity;
  char *file_basename = NULL;

  set_program_name (argv[0]);

  verbosity = 0;

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "o:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'o':
	    file_basename = optarg;
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME,
		    PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain LAYOUT.  */
  if (optind >= argc)
    {
      fprintf (stderr, "No layout is specified.\n");
      usage (1);
    }

  if (file_basename == NULL)
    {
      file_basename = xasprintf ("%s.gkb", argv[optind]);
      write_keymaps (argv[optind], file_basename);
      free (file_basename);
    }
  else
    write_keymaps (argv[optind], file_basename);

  return 0;
}
