/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/dl.h>
#include <grub/at_keyboard.h>
#include <grub/keyboard_layouts.h>
#include <grub/command.h>
#include <grub/gzio.h>
#include <grub/i18n.h>

GRUB_AT_KEY_KEYBOARD_MAP (keyboard_map);

static int
get_abstract_code (grub_term_input_t term, int in)
{
  switch (term->flags & GRUB_TERM_INPUT_FLAGS_TYPE_MASK)
    {
    case GRUB_TERM_INPUT_FLAGS_TYPE_TERMCODES:
    default:
      return in;
    case GRUB_TERM_INPUT_FLAGS_TYPE_BIOS:
      {
	unsigned status = 0;
	unsigned flags = 0;

	if (term->getkeystatus)
	  status = term->getkeystatus (term);
	if (status & GRUB_TERM_CAPS)
	  flags |= GRUB_TERM_CAPS;

	if ((0x5600 | '\\') == (in & 0xffff))
	  return GRUB_TERM_KEY_102 | flags;

	if ((0x5600 | '|') == (in & 0xffff))
	  return GRUB_TERM_KEY_SHIFT_102 | flags;

	if ((in & 0xff00) == 0x3500 || (in & 0xff00) == 0x3700
	    || (in & 0xff00) == 0x4500
	    || ((in & 0xff00) >= 0x4700 && (in & 0xff00) <= 0x5300))
	  flags |= GRUB_TERM_KEYPAD;

	/* Detect CTRL'ed keys.  */
	if ((in & 0xff) > 0 && (in & 0xff) < 0x20
	    && ((in & 0xffff) != (0x0100 | '\e'))
	    && ((in & 0xffff) != (0x0f00 | '\t'))
	    && ((in & 0xffff) != (0x0e00 | '\b'))
	    && ((in & 0xffff) != (0x1c00 | '\r'))
	    && ((in & 0xffff) != (0x1c00 | '\n')))
	  return ((in & 0xff) - 1 + 'a') | flags | GRUB_TERM_CTRL;
	/* Detect ALT'ed keys.  */
	/* XXX no way to distinguish left and right ALT.  */
	if (((in & 0xff) == 0) && keyboard_map[(in & 0xff00) >> 8] >= 'a'
	    && keyboard_map[(in & 0xff00) >> 8] <= 'z')
	  return keyboard_map[(in & 0xff00) >> 8] | flags | GRUB_TERM_ALT_GR;

	if ((in & 0xff) == 0)
	  return keyboard_map[(in & 0xff00) >> 8] | flags;

	return (in & 0xff) | flags;
      }
    }
}

static grub_uint32_t mapping[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];

static unsigned
clear_internal_flags (unsigned in)
{
  if (in & GRUB_TERM_ALT_GR)
    in = (in & ~GRUB_TERM_ALT_GR) | GRUB_TERM_ALT;
  return in & ~GRUB_TERM_CAPS & ~GRUB_TERM_KEYPAD;
}

static unsigned
map (grub_term_input_t term __attribute__ ((unused)), unsigned in)
{
  if (in & GRUB_TERM_KEYPAD)
    return clear_internal_flags (in);

  /* AltGr isn't supported yet.  */
  if (in & GRUB_TERM_ALT_GR)
    in = (in & ~GRUB_TERM_ALT_GR) | GRUB_TERM_ALT;

  if ((in & GRUB_TERM_EXTENDED) || (in & GRUB_TERM_KEY_MASK) == '\b'
      || (in & GRUB_TERM_KEY_MASK) == '\n' || (in & GRUB_TERM_KEY_MASK) == ' '
      || (in & GRUB_TERM_KEY_MASK) == '\t' || (in & GRUB_TERM_KEY_MASK) == '\e'
      || (in & GRUB_TERM_KEY_MASK) == '\r' 
      || (in & GRUB_TERM_KEY_MASK) >= ARRAY_SIZE (mapping))
    return clear_internal_flags (in);

  return mapping[in & GRUB_TERM_KEY_MASK]
    | clear_internal_flags (in & ~GRUB_TERM_KEY_MASK);
}

static int
translate (grub_term_input_t term, int in)
{
  int code, flags;
  code = get_abstract_code (term, in);
  if ((code & GRUB_TERM_CAPS) && (code & GRUB_TERM_KEY_MASK) >= 'a'
      && (code & GRUB_TERM_KEY_MASK) <= 'z')
    code = (code & GRUB_TERM_KEY_MASK) + 'A' - 'a';
  else if ((code & GRUB_TERM_CAPS) && (code & GRUB_TERM_KEY_MASK) >= 'A'
	   && (code & GRUB_TERM_KEY_MASK) <= 'Z')
    code = (code & GRUB_TERM_KEY_MASK) + 'a' - 'A';    

  flags = code & ~(GRUB_TERM_KEY_MASK | GRUB_TERM_ALT_GR | GRUB_TERM_KEYPAD);
  code &= (GRUB_TERM_KEY_MASK | GRUB_TERM_ALT_GR | GRUB_TERM_KEYPAD);
  code = map (term, code);
  /* Transform unconsumed AltGr into Alt.  */
  if (code & GRUB_TERM_ALT_GR)
    {
      flags |= GRUB_TERM_ALT;
      code &= ~GRUB_TERM_ALT_GR;
    }
  if ((flags & GRUB_TERM_CAPS) && code >= 'a' && code <= 'z')
    code += 'A' - 'a';
  else if ((flags & GRUB_TERM_CAPS) && code >= 'A'
	   && code <= 'Z')
    code += 'a' - 'A';
  return code | flags;
}

static int
grub_getkey_smart (void)
{
  grub_term_input_t term;

  grub_refresh ();

  while (1)
    {
      FOR_ACTIVE_TERM_INPUTS(term)
      {
	int key = term->checkkey (term);
	if (key != -1)
	  return translate (term, term->getkey (term));
      }

      grub_cpu_idle ();
    }
}

int
grub_checkkey (void)
{
  grub_term_input_t term;

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    int key = term->checkkey (term);
    if (key != -1)
      return translate (term, key);
  }

  return -1;
}

static grub_err_t
grub_cmd_keymap (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  char *filename;
  grub_file_t file;
  grub_uint32_t version;
  grub_uint8_t magic[GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE];
  grub_uint32_t newmapping[GRUB_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  unsigned i;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file or layout name required");
  if (argv[0][0] != '(' && argv[0][0] != '/' && argv[0][0] != '+')
    {
      const char *prefix = grub_env_get ("prefix");
      if (!prefix)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "No prefix set");	
      filename = grub_xasprintf ("%s/layouts/%s.gkb", prefix, argv[0]);
      if (!filename)
	return grub_errno;
    }
  else
    filename = argv[0];

  file = grub_gzfile_open (filename, 1);
  if (! file)
    goto fail;

  if (grub_file_read (file, magic, sizeof (magic)) != sizeof (magic))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  if (grub_memcmp (magic, GRUB_KEYBOARD_LAYOUTS_FILEMAGIC,
		   GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE) != 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid magic");
      goto fail;
    }

  if (grub_file_read (file, &version, sizeof (version)) != sizeof (version))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  if (grub_le_to_cpu32 (version) != GRUB_KEYBOARD_LAYOUTS_VERSION)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid version");
      goto fail;
    }

  if (grub_file_read (file, newmapping, sizeof (newmapping))
      != sizeof (newmapping))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  for (i = 0; i < ARRAY_SIZE (mapping); i++)
    mapping[i] = grub_le_to_cpu32(newmapping[i]);

  return GRUB_ERR_NONE;

 fail:
  if (filename != argv[0])
    grub_free (filename);
  if (file)
    grub_file_close (file);
  return grub_errno;
}

static int (*grub_getkey_saved) (void);

static grub_command_t cmd;

GRUB_MOD_INIT(keylayouts)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (mapping); i++)
    mapping[i] = i;
  mapping[GRUB_TERM_KEY_102] = '\\';
  mapping[GRUB_TERM_KEY_SHIFT_102] = '|';

  grub_getkey_saved = grub_getkey;
  grub_getkey = grub_getkey_smart;

  cmd = grub_register_command ("keymap", grub_cmd_keymap,
			       0, N_("Load a keyboard layout."));
}

GRUB_MOD_FINI(keylayouts)
{
  grub_getkey = grub_getkey_saved;
  grub_unregister_command (cmd);
}
