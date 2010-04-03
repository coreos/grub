/* multiboot.c - boot a multiboot OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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

/*
 *  FIXME: The following features from the Multiboot specification still
 *         need to be implemented:
 *  - VBE support
 *  - symbol table
 *  - drives table
 *  - ROM configuration table
 *  - APM table
 */

#include <grub/loader.h>
#include <grub/command.h>
#include <grub/machine/loader.h>
#include <grub/multiboot.h>
#include <grub/cpu/multiboot.h>
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/env.h>
#include <grub/cpu/relocator.h>
#include <grub/video.h>
#include <grub/memory.h>
#include <grub/i18n.h>

#ifdef GRUB_MACHINE_EFI
#include <grub/efi/efi.h>
#endif

#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU)
#define DEFAULT_VIDEO_MODE "text"
#else
#define DEFAULT_VIDEO_MODE "auto"
#endif

grub_size_t grub_multiboot_alloc_mbi;

char *grub_multiboot_payload_orig;
grub_addr_t grub_multiboot_payload_dest;
grub_size_t grub_multiboot_pure_size;
grub_uint32_t grub_multiboot_payload_eip;
static int accepts_video;
static int accepts_ega_text;
static int console_required;
static grub_dl_t my_mod;


/* Return the length of the Multiboot mmap that will be needed to allocate
   our platform's map.  */
grub_uint32_t
grub_get_multiboot_mmap_count (void)
{
  grub_size_t count = 0;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr __attribute__ ((unused)),
			     grub_uint64_t size __attribute__ ((unused)),
			     grub_uint32_t type __attribute__ ((unused)))
    {
      count++;
      return 0;
    }

  grub_mmap_iterate (hook);

  return count;
}

grub_err_t
grub_multiboot_set_video_mode (void)
{
  grub_err_t err;
  const char *modevar;

  if (accepts_video || !GRUB_MACHINE_HAS_VGA_TEXT)
    {
      modevar = grub_env_get ("gfxpayload");
      if (! modevar || *modevar == 0)
	err = grub_video_set_mode (DEFAULT_VIDEO_MODE, 0, 0);
      else
	{
	  char *tmp;
	  tmp = grub_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
	  if (! tmp)
	    return grub_errno;
	  err = grub_video_set_mode (tmp, 0, 0);
	  grub_free (tmp);
	}
    }
  else
    err = grub_video_set_mode ("text", 0, 0);

  return err;
}

static grub_err_t
grub_multiboot_boot (void)
{
  grub_size_t mbi_size;
  grub_err_t err;
  struct grub_relocator32_state state = MULTIBOOT_INITIAL_STATE;

  state.MULTIBOOT_ENTRY_REGISTER = grub_multiboot_payload_eip;

  mbi_size = grub_multiboot_get_mbi_size ();
  if (grub_multiboot_alloc_mbi < mbi_size)
    {
      grub_multiboot_payload_orig
	= grub_relocator32_realloc (grub_multiboot_payload_orig,
				    grub_multiboot_pure_size + mbi_size);
      if (!grub_multiboot_payload_orig)
	return grub_errno;
      grub_multiboot_alloc_mbi = mbi_size;
    }

  state.MULTIBOOT_MBI_REGISTER = grub_multiboot_payload_dest
    + grub_multiboot_pure_size;
  err = grub_multiboot_make_mbi (grub_multiboot_payload_orig,
				 grub_multiboot_payload_dest,
				 grub_multiboot_pure_size, mbi_size);
  if (err)
    return err;

#ifdef GRUB_MACHINE_EFI
  if (! grub_efi_finish_boot_services ())
     grub_fatal ("cannot exit boot services");
#endif

  grub_relocator32_boot (grub_multiboot_payload_orig,
			 grub_multiboot_payload_dest,
			 state);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_multiboot_unload (void)
{
  grub_multiboot_free_mbi ();

  grub_relocator32_free (grub_multiboot_payload_orig);

  grub_multiboot_alloc_mbi = 0;

  grub_multiboot_payload_orig = NULL;
  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

#define MULTIBOOT_LOAD_ELF64
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF64

#define MULTIBOOT_LOAD_ELF32
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF32

/* Load ELF32 or ELF64.  */
grub_err_t
grub_multiboot_load_elf (grub_file_t file, void *buffer)
{
  if (grub_multiboot_is_elf32 (buffer))
    return grub_multiboot_load_elf32 (file, buffer);
  else if (grub_multiboot_is_elf64 (buffer))
    return grub_multiboot_load_elf64 (file, buffer);

  return grub_error (GRUB_ERR_UNKNOWN_OS, "unknown ELF class");
}

grub_err_t
grub_multiboot_set_console (int console_type, int accepted_consoles,
			    int width, int height, int depth,
			    int console_req)
{
  console_required = console_req;
  if (!(accepted_consoles 
	& (GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER
	   | (GRUB_MACHINE_HAS_VGA_TEXT ? GRUB_MULTIBOOT_CONSOLE_EGA_TEXT : 0))))
    {
      if (console_required)
	return grub_error (GRUB_ERR_BAD_OS,
			   "OS requires a console but none is available");
      grub_printf ("WARNING: no console will be available to OS");
      accepts_video = 0;
      accepts_ega_text = 0;
      return GRUB_ERR_NONE;
    }

  if (console_type == GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER)
    {
      char *buf;
      if (depth && width && height)
	buf = grub_xasprintf ("%dx%dx%d,%dx%d,auto", width,
			      height, depth, width, height);
      else if (width && height)
	buf = grub_xasprintf ("%dx%d,auto", width, height);
      else
	buf = grub_strdup ("auto");

      if (!buf)
	return grub_errno;
      grub_env_set ("gfxpayload", buf);
      grub_free (buf);
    }
 else
   grub_env_set ("gfxpayload", "text");

  accepts_video = !!(accepted_consoles & GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER);
  accepts_ega_text = !!(accepted_consoles & GRUB_MULTIBOOT_CONSOLE_EGA_TEXT);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_multiboot (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_err_t err;

  grub_loader_unset ();

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "couldn't open file");

  grub_dl_ref (my_mod);

  /* Skip filename.  */
  grub_multiboot_init_mbi (argc - 1, argv + 1);

  grub_relocator32_free (grub_multiboot_payload_orig);
  grub_multiboot_payload_orig = NULL;

  err = grub_multiboot_load (file);
  if (err)
    goto fail;

  grub_multiboot_set_bootdev ();

  grub_loader_set (grub_multiboot_boot, grub_multiboot_unload, 0);

 fail:
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_relocator32_free (grub_multiboot_payload_orig);
      grub_multiboot_free_mbi ();
      grub_dl_unref (my_mod);
    }

  return grub_errno;
}

static grub_err_t
grub_cmd_module (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  char *module = 0;
  grub_err_t err;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");

  if (!grub_multiboot_payload_orig)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       "you need to load the multiboot kernel first");

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    return grub_errno;

  size = grub_file_size (file);
  module = grub_memalign (MULTIBOOT_MOD_ALIGN, size);
  if (! module)
    {
      grub_file_close (file);
      return grub_errno;
    }

  err = grub_multiboot_add_module ((grub_addr_t) module, size,
				   argc - 1, argv + 1);
  if (err)
    {
      grub_file_close (file);
      return err;
    }

  if (grub_file_read (file, module, size) != size)
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
    }

  grub_file_close (file);
  return GRUB_ERR_NONE;;
}

static grub_command_t cmd_multiboot, cmd_module;

GRUB_MOD_INIT(multiboot)
{
  cmd_multiboot =
#ifdef GRUB_USE_MULTIBOOT2
    grub_register_command ("multiboot2", grub_cmd_multiboot,
			   0, N_("Load a multiboot 2 kernel."));
#else
    grub_register_command ("multiboot", grub_cmd_multiboot,
			   0, N_("Load a multiboot kernel."));
#endif

  cmd_module =
    grub_register_command ("module", grub_cmd_module,
			   0, N_("Load a multiboot module."));

  my_mod = mod;
}

GRUB_MOD_FINI(multiboot)
{
  grub_unregister_command (cmd_multiboot);
  grub_unregister_command (cmd_module);
}
