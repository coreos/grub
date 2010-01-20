/* multiboot_loader.c - boot multiboot kernel image */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/multiboot.h>
#include <grub/elf.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/command.h>
#include <grub/i18n.h>

grub_dl_t my_mod;

static int
find_multi_boot1_header (grub_file_t file)
{
  struct multiboot_header *header;
  char buffer[MULTIBOOT_SEARCH];
  int found_status = 0;
  grub_ssize_t len;

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    return found_status;

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
      ((char *) header <= buffer + len - 12) || (header = 0);
      header = (struct multiboot_header *) ((char *) header + 4))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
          && !(header->magic + header->flags + header->checksum))
        {
           found_status = 1;
           break;
        }
     }

   return found_status;
}

static grub_err_t
grub_cmd_multiboot_loader (grub_command_t cmd __attribute__ ((unused)),
			   int argc, char *argv[])
{
  grub_file_t file = 0;
  int header_multi_ver_found = 0;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "couldn't open file");
      goto fail;
    }

  /* find which header is in the file */
  if (find_multi_boot1_header (file))
    header_multi_ver_found = 1;
  else
    {
      grub_error (GRUB_ERR_BAD_OS, "multiboot header not found");
      goto fail;
    }

  /* close file before calling functions */
  if (file)
    grub_file_close (file);

  /* Launch multi boot with header */

  grub_dprintf ("multiboot_loader",
		"Launching multiboot 1 grub_multiboot() function\n");
  grub_multiboot (argc, argv);

  return grub_errno;

fail:
  if (file)
    grub_file_close (file);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_err_t
grub_cmd_module_loader (grub_command_t cmd __attribute__ ((unused)),
			int argc, char *argv[])
{

  grub_dprintf("multiboot_loader",
	       "Launching multiboot 1 grub_module() function\n");
  grub_module (argc, argv);

  return grub_errno;
}

static grub_command_t cmd_multiboot, cmd_module;

GRUB_MOD_INIT(multiboot)
{
  cmd_multiboot =
#ifdef GRUB_USE_MULTIBOOT2
    grub_register_command ("multiboot2", grub_cmd_multiboot_loader,
			   0, N_("Load a multiboot 2 kernel."));
#else
    grub_register_command ("multiboot", grub_cmd_multiboot_loader,
			   0, N_("Load a multiboot kernel."));
#endif

  cmd_module =
    grub_register_command ("module", grub_cmd_module_loader,
			   0, N_("Load a multiboot module."));

  my_mod = mod;
}

GRUB_MOD_FINI(multiboot)
{
  grub_unregister_command (cmd_multiboot);
  grub_unregister_command (cmd_module);
}
