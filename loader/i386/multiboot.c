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
#define UNSUPPORTED_FLAGS			0x0000fffc

#include <grub/loader.h>
#include <grub/machine/loader.h>
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
static grub_size_t code_size, alloc_mbi;

char *grub_multiboot_payload_orig;
grub_addr_t grub_multiboot_payload_dest;
grub_size_t grub_multiboot_pure_size;
grub_uint32_t grub_multiboot_payload_eip;

static grub_err_t
grub_multiboot_boot (void)
{
  grub_size_t mbi_size;
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

  mbi_size = grub_multiboot_get_mbi_size ();
  if (alloc_mbi < mbi_size)
    {
      grub_multiboot_payload_orig
	= grub_relocator32_realloc (grub_multiboot_payload_orig,
				    grub_multiboot_pure_size + mbi_size);
      if (!grub_multiboot_payload_orig)
	return grub_errno;
      alloc_mbi = mbi_size;
    }

  state.ebx = grub_multiboot_payload_dest + grub_multiboot_pure_size;
  err = grub_multiboot_make_mbi (grub_multiboot_payload_orig,
				 grub_multiboot_payload_dest,
				 grub_multiboot_pure_size, mbi_size);
  if (err)
    return err;

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

  alloc_mbi = 0;

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

  grub_relocator32_free (grub_multiboot_payload_orig);
  grub_multiboot_payload_orig = NULL;

  /* Skip filename.  */
  grub_multiboot_init_mbi (argc - 1, argv + 1);

  if (header->flags & MULTIBOOT_AOUT_KLUDGE)
    {
      int offset = ((char *) header - buffer -
		    (header->header_addr - header->load_addr));
      int load_size = ((header->load_end_addr == 0) ? file->size - offset :
		       header->load_end_addr - header->load_addr);

      if (header->bss_end_addr)
	code_size = (header->bss_end_addr - header->load_addr);
      else
	code_size = load_size;
      grub_multiboot_payload_dest = header->load_addr;

      grub_multiboot_pure_size += code_size;

      /* Allocate a bit more to avoid relocations in most cases.  */
      alloc_mbi = grub_multiboot_get_mbi_size () + 65536;
      grub_multiboot_payload_orig
	= grub_relocator32_alloc (grub_multiboot_pure_size + alloc_mbi);

      if (! grub_multiboot_payload_orig)
	goto fail;

      if ((grub_file_seek (file, offset)) == (grub_off_t) -1)
	goto fail;

      grub_file_read (file, (void *) grub_multiboot_payload_orig, load_size);
      if (grub_errno)
	goto fail;

      if (header->bss_end_addr)
	grub_memset ((void *) (grub_multiboot_payload_orig + load_size), 0,
		     header->bss_end_addr - header->load_addr - load_size);

      grub_multiboot_payload_eip = header->entry_addr;

    }
  else if (grub_multiboot_load_elf (file, buffer) != GRUB_ERR_NONE)
    goto fail;

  if (header->flags & MULTIBOOT_VIDEO_MODE)
    {
      switch (header->mode_type)
	{
	case 1:
	  grub_env_set ("gfxpayload", "text");
	  break;

	case 0:
	  {
	    char buf[sizeof ("XXXXXXXXXXxXXXXXXXXXXxXXXXXXXXXX,XXXXXXXXXXxXXXXXXXXXX,auto")];
	    if (header->depth && header->width && header->height)
	      grub_sprintf (buf, "%dx%dx%d,%dx%d,auto", header->width,
			    header->height, header->depth, header->width,
			    header->height);
	    else if (header->width && header->height)
	      grub_sprintf (buf, "%dx%d,auto", header->width, header->height);
	    else
	      grub_sprintf (buf, "auto");

	    grub_env_set ("gfxpayload", buf);
	    break;
	  }
	}
    }

  grub_multiboot_set_bootdev ();

  grub_loader_set (grub_multiboot_boot, grub_multiboot_unload, 0);

 fail:
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_relocator32_free (grub_multiboot_payload_orig);
      grub_dl_unref (my_mod);
    }
}

void
grub_module  (int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  char *module = 0;
  grub_err_t err;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      goto fail;
    }

  if (!grub_multiboot_payload_orig)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  "you need to load the multiboot kernel first");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  module = grub_memalign (MULTIBOOT_MOD_ALIGN, size);
  if (! module)
    goto fail;

  err = grub_multiboot_add_module ((grub_addr_t) module, size,
				   argc - 1, argv + 1);
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

