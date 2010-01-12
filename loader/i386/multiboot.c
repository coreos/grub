/* multiboot.c - boot a multiboot OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

/* The bits in the required part of flags field we don't support.  */
#define UNSUPPORTED_FLAGS			0x0000fff0

#include <grub/loader.h>
#include <grub/multiboot.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
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
#include <grub/i386/relocator.h>

extern grub_dl_t my_mod;
struct grub_relocator *grub_multiboot_relocator = NULL;
grub_uint32_t grub_multiboot_payload_eip;

static grub_err_t
grub_multiboot_boot (void)
{
  grub_err_t err;
  struct grub_relocator32_state state =
    {
      .eax = MULTIBOOT_BOOTLOADER_MAGIC,
      .ecx = 0,
      .edx = 0,
      .eip = grub_multiboot_payload_eip,
      /* Set esp to some random location in low memory to avoid breaking
	 non-compliant kernels.  */
      .esp = 0x7ff00
    };

  err = grub_multiboot_make_mbi (&state.ebx);
  if (err)
    return err;

  grub_relocator32_boot (grub_multiboot_relocator, state);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_multiboot_unload (void)
{
  grub_multiboot_free_mbi ();

  grub_relocator_unload (grub_multiboot_relocator);
  grub_multiboot_relocator = NULL;

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
static grub_err_t
grub_multiboot_load_elf (grub_file_t file, void *buffer)
{
  if (grub_multiboot_is_elf32 (buffer))
    return grub_multiboot_load_elf32 (file, buffer);
  else if (grub_multiboot_is_elf64 (buffer))
    return grub_multiboot_load_elf64 (file, buffer);

  return grub_error (GRUB_ERR_UNKNOWN_OS, "unknown ELF class");
}

void
grub_multiboot (int argc, char *argv[])
{
  grub_file_t file = 0;
  char buffer[MULTIBOOT_SEARCH];
  struct multiboot_header *header;
  grub_ssize_t len;
  grub_err_t err;

  grub_loader_unset ();

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

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      grub_error (GRUB_ERR_BAD_OS, "file too small");
      goto fail;
    }

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
       ((char *) header <= buffer + len - 12) || (header = 0);
       header = (struct multiboot_header *) ((char *) header + 4))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
	  && !(header->magic + header->flags + header->checksum))
	break;
    }

  if (header == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no multiboot header found");
      goto fail;
    }

  if (header->flags & UNSUPPORTED_FLAGS)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "unsupported flag: 0x%x", header->flags);
      goto fail;
    }

  grub_relocator_unload (grub_multiboot_relocator);
  grub_multiboot_relocator = NULL;

  /* Skip filename.  */
  grub_multiboot_init_mbi (argc - 1, argv + 1);

  grub_multiboot_relocator = grub_relocator_new ();

  if (!grub_multiboot_relocator)
    goto fail;

  if (header->flags & MULTIBOOT_AOUT_KLUDGE)
    {
      int offset = ((char *) header - buffer -
		    (header->header_addr - header->load_addr));
      int load_size = ((header->load_end_addr == 0) ? file->size - offset :
		       header->load_end_addr - header->load_addr);
      grub_size_t code_size;
      void *source;

      if (header->bss_end_addr)
	code_size = (header->bss_end_addr - header->load_addr);
      else
	code_size = load_size;

      err = grub_relocator_alloc_chunk_addr (grub_multiboot_relocator, 
					     &source, header->load_addr,
					     code_size);
      if (err)
	{
	  grub_dprintf ("multiboot_loader", "Error loading aout kludge\n");
	  goto fail;
	}

      if ((grub_file_seek (file, offset)) == (grub_off_t) -1)
	goto fail;

      grub_file_read (file, source, load_size);
      if (grub_errno)
	goto fail;

      if (header->bss_end_addr)
	grub_memset ((grub_uint32_t *) source + load_size, 0,
		     header->bss_end_addr - header->load_addr - load_size);

      grub_multiboot_payload_eip = header->entry_addr;
    }
  else if (grub_multiboot_load_elf (file, buffer) != GRUB_ERR_NONE)
    goto fail;

  grub_multiboot_set_bootdev ();

  grub_loader_set (grub_multiboot_boot, grub_multiboot_unload, 0);

 fail:
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_relocator_unload (grub_multiboot_relocator);
      grub_dl_unref (my_mod);
    }
}

void
grub_module  (int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  void *module = NULL;
  grub_addr_t target;
  grub_err_t err;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      goto fail;
    }

  if (!grub_multiboot_relocator)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  "you need to load the multiboot kernel first");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  err = grub_relocator_alloc_chunk_align (grub_multiboot_relocator, &module, 
					  &target,
					  0, (0xffffffff - size) + 1,
					  size, MULTIBOOT_MOD_ALIGN,
					  GRUB_RELOCATOR_PREFERENCE_NONE);
  if (err)
    goto fail;

  err = grub_multiboot_add_module (target, size, argc - 1, argv + 1);
  if (err)
    goto fail;

  if (grub_file_read (file, module, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

 fail:
  if (file)
    grub_file_close (file);
}

