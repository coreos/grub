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
#include <errno.h>

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
  {"Escape", GRUB_TERM_ESC},
  {"Tab", GRUB_TERM_TAB},
  {"Delete", GRUB_TERM_BACKSPACE},

  {"KP_Multiply", '*'},
  {"KP_Subtract", '-'},
  {"KP_Add", '+'},
  {"KP_Divide", '/'},

  {"KP_Enter", '\n'},
  {"Return", '\n'},

  {"F1", GRUB_TERM_KEY_F1},
  {"F2", GRUB_TERM_KEY_F2},
  {"F3", GRUB_TERM_KEY_F3},
  {"F4", GRUB_TERM_KEY_F4},
  {"F5", GRUB_TERM_KEY_F5},
  {"F6", GRUB_TERM_KEY_F6},
  {"F7", GRUB_TERM_KEY_F7},
  {"F8", GRUB_TERM_KEY_F8},
  {"F9", GRUB_TERM_KEY_F9},
  {"F10", GRUB_TERM_KEY_F10},
  {"F11", GRUB_TERM_KEY_F11},
  {"F12", GRUB_TERM_KEY_F12},
  {"F13", GRUB_TERM_KEY_F1 | GRUB_TERM_SHIFT},
  {"F14", GRUB_TERM_KEY_F2 | GRUB_TERM_SHIFT},
  {"F15", GRUB_TERM_KEY_F3 | GRUB_TERM_SHIFT},
  {"F16", GRUB_TERM_KEY_F4 | GRUB_TERM_SHIFT},
  {"F17", GRUB_TERM_KEY_F5 | GRUB_TERM_SHIFT},
  {"F18", GRUB_TERM_KEY_F6 | GRUB_TERM_SHIFT},
  {"F19", GRUB_TERM_KEY_F7 | GRUB_TERM_SHIFT},
  {"F20", GRUB_TERM_KEY_F8 | GRUB_TERM_SHIFT},
  {"F21", GRUB_TERM_KEY_F9 | GRUB_TERM_SHIFT},
  {"F22", GRUB_TERM_KEY_F10 | GRUB_TERM_SHIFT},
  {"F23", GRUB_TERM_KEY_F11 | GRUB_TERM_SHIFT},
  {"F24", GRUB_TERM_KEY_F12 | GRUB_TERM_SHIFT},
  {"Console_13", GRUB_TERM_KEY_F1 | GRUB_TERM_ALT},
  {"Console_14", GRUB_TERM_KEY_F2 | GRUB_TERM_ALT},
  {"Console_15", GRUB_TERM_KEY_F3 | GRUB_TERM_ALT},
  {"Console_16", GRUB_TERM_KEY_F4 | GRUB_TERM_ALT},
  {"Console_17", GRUB_TERM_KEY_F5 | GRUB_TERM_ALT},
  {"Console_18", GRUB_TERM_KEY_F6 | GRUB_TERM_ALT},
  {"Console_19", GRUB_TERM_KEY_F7 | GRUB_TERM_ALT},
  {"Console_20", GRUB_TERM_KEY_F8 | GRUB_TERM_ALT},
  {"Console_21", GRUB_TERM_KEY_F9 | GRUB_TERM_ALT},
  {"Console_22", GRUB_TERM_KEY_F10 | GRUB_TERM_ALT},
  {"Console_23", GRUB_TERM_KEY_F11 | GRUB_TERM_ALT},
  {"Console_24", GRUB_TERM_KEY_F12 | GRUB_TERM_ALT},
  {"Console_25", GRUB_TERM_KEY_F1 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_26", GRUB_TERM_KEY_F2 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_27", GRUB_TERM_KEY_F3 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_28", GRUB_TERM_KEY_F4 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_29", GRUB_TERM_KEY_F5 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_30", GRUB_TERM_KEY_F6 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_31", GRUB_TERM_KEY_F7 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_32", GRUB_TERM_KEY_F8 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_33", GRUB_TERM_KEY_F9 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_34", GRUB_TERM_KEY_F10 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_35", GRUB_TERM_KEY_F11 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},
  {"Console_36", GRUB_TERM_KEY_F12 | GRUB_TERM_SHIFT | GRUB_TERM_ALT},

  {"Insert", GRUB_TERM_KEY_INSERT},
  {"Down", GRUB_TERM_KEY_DOWN},
  {"Up", GRUB_TERM_KEY_UP},
  {"Home", GRUB_TERM_KEY_HOME},
  {"End", GRUB_TERM_KEY_END},
  {"Right", GRUB_TERM_KEY_RIGHT},
  {"Left", GRUB_TERM_KEY_LEFT},
  {"VoidSymbol", 0},

  /* "Undead" keys since no dead key support in GRUB.  */
  {"dead_acute", '\''},
  {"dead_circumflex", '^'},
  {"dead_grave", '`'},
  {"dead_tilde", '~'},
  {"dead_diaeresis", '"'},
  
  /* Following ones don't provide any useful symbols for shell.  */
  {"dead_cedilla", 0},
  {"dead_ogonek", 0},
  {"dead_caron", 0},
  {"dead_breve", 0},
  {"dead_doubleacute", 0},

  /* NumLock not supported yet.  */
  {"KP_0", GRUB_TERM_KEY_INSERT},
  {"KP_1", GRUB_TERM_KEY_END},
  {"KP_2", GRUB_TERM_KEY_DOWN},
  {"KP_3", GRUB_TERM_KEY_NPAGE},
  {"KP_4", GRUB_TERM_KEY_LEFT},
  {"KP_5", 0},
  {"KP_6", GRUB_TERM_KEY_RIGHT},
  {"KP_7", GRUB_TERM_KEY_HOME},
  {"KP_8", GRUB_TERM_KEY_UP},
  {"KP_9", GRUB_TERM_KEY_PPAGE},
  {"KP_Period", GRUB_TERM_KEY_DC},

  /* Unused in GRUB.  */
  {"Pause", 0},
  {"Remove", 0},
  {"Next", 0},
  {"Prior", 0},
  {"Scroll_Forward", 0},
  {"Scroll_Backward", 0},
  {"Hex_0", 0},
  {"Hex_1", 0},
  {"Hex_2", 0},
  {"Hex_3", 0},
  {"Hex_4", 0},
  {"Hex_5", 0},
  {"Hex_6", 0},
  {"Hex_7", 0},
  {"Hex_8", 0},
  {"Hex_9", 0},
  {"Hex_A", 0},
  {"Hex_B", 0},
  {"Hex_C", 0},
  {"Hex_D", 0},
  {"Hex_E", 0},
  {"Hex_F", 0},
  {"Scroll_Lock", 0},
  {"Show_Memory", 0},
  {"Show_Registers", 0},
  {"Control_backslash", 0},
  {"Compose", 0},

  /* Keys currently not remappable.  */
  {"CtrlL_Lock", 0},
  {"Caps_Lock", 0},
  {"ShiftL", 0},
  {"Num_Lock", 0},
  {"Alt", 0},
  {"AltGr", 0},
  {"Control", 0},
  {"Shift", 0},

  {NULL, '\0'}
};

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

void
add_special_keys (struct grub_keyboard_layout *layout)
{
  /* OLPC keys.  */
  layout->keyboard_map[101] = GRUB_TERM_KEY_UP;
  layout->keyboard_map[102] = GRUB_TERM_KEY_DOWN;
  layout->keyboard_map[103] = GRUB_TERM_KEY_LEFT;
  layout->keyboard_map[104] = GRUB_TERM_KEY_RIGHT;
}

static unsigned
lookup (char *code)
{
  int i;

  for (i = 0; console_grub_equivalences[i].layout != NULL; i++)
    if (strcmp (code, console_grub_equivalences[i].layout) == 0)
      return console_grub_equivalences[i].grub;

  fprintf (stderr, "Unknown key %s\n", code);

  return '\0';
}

static unsigned int
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

static void
write_file (FILE *out, struct grub_keyboard_layout *layout)
{
  grub_uint32_t version;
  unsigned i;

  version = grub_cpu_to_le32 (GRUB_KEYBOARD_LAYOUTS_VERSION);
  
  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map); i++)
    layout->keyboard_map[i] = grub_cpu_to_le32(layout->keyboard_map[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_shift); i++)
    layout->keyboard_map_shift[i]
      = grub_cpu_to_le32(layout->keyboard_map_shift[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_l3); i++)
    layout->keyboard_map_l3[i]
      = grub_cpu_to_le32(layout->keyboard_map_l3[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_shift_l3); i++)
    layout->keyboard_map_shift_l3[i]
      = grub_cpu_to_le32(layout->keyboard_map_shift_l3[i]);

  fwrite (GRUB_KEYBOARD_LAYOUTS_FILEMAGIC, 1,
	  GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE, out);
  fwrite (&version, sizeof (version), 1, out);
  fwrite (layout, 1, sizeof (*layout), out);
}

static void
write_keymaps (FILE *in, FILE *out)
{
  struct grub_keyboard_layout layout;
  char line[2048];
  int ok;

  memset (&layout, 0, sizeof (layout));

  /* Process the ckbcomp output and prepare the layouts.  */
  ok = 0;
  while (fgets (line, sizeof (line), in))
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
	  if (keycode < GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE)
	    {
	      layout.keyboard_map[keycode] = get_grub_code (normal);
	      layout.keyboard_map_shift[keycode] = get_grub_code (shift);
	      layout.keyboard_map_l3[keycode] = get_grub_code (normalalt);
	      layout.keyboard_map_shift_l3[keycode]
		= get_grub_code (shiftalt);
	      ok = 1;
	    }
	}
    }

  if (ok == 0)
    {
      fprintf (stderr, "ERROR: no keycodes found. Check output of %s.\n",
	       CKBCOMP);
      exit (1);
    }

  add_special_keys (&layout);

  write_file (out, &layout);
}

int
main (int argc, char *argv[])
{
  int verbosity;
  char *infile_name = NULL;
  char *outfile_name = NULL;
  FILE *in, *out;

  set_program_name (argv[0]);

  verbosity = 0;

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "o:i:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'i':
	    infile_name = optarg;
	    break;

	  case 'o':
	    outfile_name = optarg;
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

  if (infile_name)
    in = fopen (infile_name, "r");
  else
    in = stdin;

  if (!in)
    grub_util_error ("Couldn't open input file: %s\n", strerror (errno));

  if (outfile_name)
    out = fopen (outfile_name, "wb");
  else
    out = stdout;

  if (!out)
    {
      if (in != stdin)
	fclose (in);
      grub_util_error ("Couldn't open output file: %s\n", strerror (errno));
    }

  write_keymaps (in, out);

  if (in != stdin)
    fclose (in);

  if (out != stdout)
    fclose (out);

  return 0;
}
