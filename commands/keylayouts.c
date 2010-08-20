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
#include <grub/keyboard_layouts.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/file.h>

static struct grub_keyboard_layout layout_us = {
  .keyboard_map = {
    /* 0x00 */ '\0', GRUB_TERM_ESC, '1', '2', '3', '4', '5', '6',
    /* 0x08 */ '7', '8', '9', '0', '-', '=', GRUB_TERM_BACKSPACE, GRUB_TERM_TAB,
    /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    /* 0x18 */ 'o', 'p', '[', ']', '\n', '\0', 'a', 's',
    /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 0x28 */ '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
    /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', '\0', '*',
    /* 0x38 */ '\0', ' ', '\0', GRUB_TERM_KEY_F1,
    /* 0x3c */ GRUB_TERM_KEY_F2, GRUB_TERM_KEY_F3,
    /* 0x3e */ GRUB_TERM_KEY_F4, GRUB_TERM_KEY_F5,
    /* 0x40 */ GRUB_TERM_KEY_F6, GRUB_TERM_KEY_F7,
    /* 0x42 */ GRUB_TERM_KEY_F8, GRUB_TERM_KEY_F9,
    /* 0x44 */ GRUB_TERM_KEY_F10, '\0', '\0', GRUB_TERM_KEY_HOME,
    /* 0x48 */ GRUB_TERM_KEY_UP, GRUB_TERM_KEY_NPAGE, '-', GRUB_TERM_KEY_LEFT,
    /* 0x4c */ GRUB_TERM_KEY_CENTER, GRUB_TERM_KEY_RIGHT, 
    /* 0x4e */ '+', GRUB_TERM_KEY_END,
    /* 0x50 */ GRUB_TERM_KEY_DOWN, GRUB_TERM_KEY_PPAGE,
    /* 0x52 */ GRUB_TERM_KEY_INSERT, GRUB_TERM_KEY_DC,
    /* 0x54 */ '\0', '\0', '\\', GRUB_TERM_KEY_F11,
    /* 0x58 */ GRUB_TERM_KEY_F12, '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    /* 0x60 */ '\0', '\0', '\0', '\0', 
    /* 0x64 */ '\0', GRUB_TERM_KEY_UP, GRUB_TERM_KEY_DOWN, GRUB_TERM_KEY_LEFT,
    /* 0x68 */ GRUB_TERM_KEY_RIGHT
  },
  .keyboard_map_shift = {
    '\0', '\0', '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', '\0', '\0',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?',
    [0x56] = '|'
  }
};

static struct grub_keyboard_layout *grub_current_layout = &layout_us;

static int
map_key_core (int code, int status, int *alt_gr_consumed)
{
  *alt_gr_consumed = 0;

  if (status & GRUB_TERM_STATUS_RALT)
    {
      if (status & (GRUB_TERM_STATUS_LSHIFT | GRUB_TERM_STATUS_RSHIFT))
	{
	  if (grub_current_layout->keyboard_map_shift_l3[code])
	    {
	      *alt_gr_consumed = 1;
	      return grub_current_layout->keyboard_map_shift_l3[code];
	    }
	  else if (grub_current_layout->keyboard_map_shift[code])
	    {
	      *alt_gr_consumed = 1;
	      return grub_current_layout->keyboard_map_l3[code]
		| GRUB_TERM_SHIFT;  
	    }
	}
      else if (grub_current_layout->keyboard_map_shift[code])
	{
	  *alt_gr_consumed = 1;
	  return grub_current_layout->keyboard_map_l3[code];  
	}
    }
  if (status & (GRUB_TERM_STATUS_LSHIFT | GRUB_TERM_STATUS_RSHIFT))
    {
      if (grub_current_layout->keyboard_map_shift[code])
	return grub_current_layout->keyboard_map_shift[code];
      else
	return grub_current_layout->keyboard_map[code] | GRUB_TERM_SHIFT;
    }
  else
    return grub_current_layout->keyboard_map[code];
}

unsigned
grub_term_map_key (int code, int status)
{
  int alt_gr_consumed;
  int key;

  key = map_key_core (code, status, &alt_gr_consumed);
  
  if (key == 0 || key == GRUB_TERM_SHIFT)
    grub_dprintf ("atkeyb", "Unknown key 0x%x detected\n", code);
  
  if (status & GRUB_TERM_STATUS_CAPS)
    {
      if ((key >= 'a') && (key <= 'z'))
	key += 'A' - 'a';
      else if ((key >= 'A') && (key <= 'Z'))
	key += 'a' - 'A';
    }
  
  if ((status & GRUB_TERM_STATUS_LALT) || 
      ((status & GRUB_TERM_STATUS_RALT) && !alt_gr_consumed))
    key |= GRUB_TERM_ALT;
  if (status & (GRUB_TERM_STATUS_LCTRL | GRUB_TERM_STATUS_RCTRL))
    key |= GRUB_TERM_CTRL;

  return key;
}

static grub_err_t
grub_cmd_keymap (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  char *filename;
  grub_file_t file;
  grub_uint32_t version;
  grub_uint8_t magic[GRUB_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE];
  struct grub_keyboard_layout *newmap = NULL;
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

  file = grub_file_open (filename);
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

  newmap = grub_malloc (sizeof (*newmap));
  if (!newmap)
    goto fail;

  if (grub_file_read (file, newmap, sizeof (*newmap)) != sizeof (*newmap))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_ARGUMENT, "file is too short");
      goto fail;
    }

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map); i++)
    newmap->keyboard_map[i] = grub_le_to_cpu32(newmap->keyboard_map[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_shift); i++)
    newmap->keyboard_map_shift[i]
      = grub_le_to_cpu32(newmap->keyboard_map_shift[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_l3); i++)
    newmap->keyboard_map_l3[i]
      = grub_le_to_cpu32(newmap->keyboard_map_l3[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_shift_l3); i++)
    newmap->keyboard_map_shift_l3[i]
      = grub_le_to_cpu32(newmap->keyboard_map_shift_l3[i]);

  grub_current_layout = newmap;

  return GRUB_ERR_NONE;

 fail:
  if (filename != argv[0])
    grub_free (filename);
  grub_free (newmap);
  if (file)
    grub_file_close (file);
  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(keylayouts)
{
  cmd = grub_register_command ("keymap", grub_cmd_keymap,
			       0, N_("Load a keyboard layout."));
}

GRUB_MOD_FINI(keylayouts)
{
  grub_unregister_command (cmd);
}
