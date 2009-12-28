/* font_cmd.c - Font command definition. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/bufio.h>
#include <grub/font.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/kernel.h>
#include <grub/mm.h>

static grub_err_t
loadfont_command (grub_command_t cmd __attribute__ ((unused)),
		  int argc,
		  char **args)
{
  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no font specified");

  while (argc--)
    {
      grub_file_t file = 0;

      file = grub_buffile_open (*args++, 1024);
      if (!file)
	return grub_errno;

      if (grub_font_load (file) != 0)
	return GRUB_ERR_BAD_FONT;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
lsfonts_command (grub_command_t cmd __attribute__ ((unused)),
                 int argc __attribute__ ((unused)),
                 char **args __attribute__ ((unused)))
{
  struct grub_font_node *node;

  grub_printf ("Loaded fonts:\n");
  for (node = grub_font_list; node; node = node->next)
    {
      grub_font_t font = node->value;
      grub_printf ("%s\n", grub_font_get_name (font));
    }

  return GRUB_ERR_NONE;
}

static grub_ssize_t 
pseudo_file_read (struct grub_file *file, char *buf, grub_size_t len)
{
  grub_memcpy (buf, (char *) file->data + file->offset, len);
  return len;
}

static grub_err_t 
pseudo_file_close (struct grub_file *file)
{
  grub_free (file->data);
  file->data = 0;
  return GRUB_ERR_NONE;
}


/* Filesystem descriptor.  */
static struct grub_fs pseudo_fs =
{
  .name = "Font Loader",
  .read = pseudo_file_read,
  .close = pseudo_file_close
};


static int
load_font_module (struct grub_module_header *header)
{
  grub_file_t file;
  if (header->type != OBJ_TYPE_FONT)
    return 0;

  file = grub_malloc (sizeof (*file));

  file->read_hook = 0;

  file->offset = 0;
  file->size = header->size - sizeof (struct grub_module_header);
  file->data = grub_malloc (header->size - sizeof (struct grub_module_header));
  if (!file->data)
    return 0;
  grub_memcpy (file->data, (char *) header + sizeof (struct grub_module_header),
	       file->size);

  file->device = 0;
  file->fs = &pseudo_fs;

  grub_font_load (file);

  return 0;
}


static grub_command_t cmd_loadfont, cmd_lsfonts;

GRUB_MOD_INIT(font_manager)
{
  grub_font_loader_init ();

  grub_module_iterate (load_font_module);

  cmd_loadfont =
    grub_register_command ("loadfont", loadfont_command,
			 "FILE...",
			 "Specify one or more font files to load.");
  cmd_lsfonts =
    grub_register_command ("lsfonts", lsfonts_command,
			   0, "List the loaded fonts.");
}

GRUB_MOD_FINI(font_manager)
{
  /* TODO: Determine way to free allocated resources.
     Warning: possible pointer references could be in use.  */

  grub_unregister_command (cmd_loadfont);
  grub_unregister_command (cmd_lsfonts);
}
