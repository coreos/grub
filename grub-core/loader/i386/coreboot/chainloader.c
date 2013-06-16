/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011  Free Software Foundation, Inc.
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

#include <grub/loader.h>
#include <grub/memory.h>
#include <grub/i386/memory.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/elfload.h>
#include <grub/video.h>
#include <grub/relocator.h>
#include <grub/i386/relocator.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/cbfs_core.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_addr_t entry;
static struct grub_relocator *relocator = NULL;

static grub_err_t
grub_chain_boot (void)
{
  struct grub_relocator32_state state;

  grub_video_set_mode ("text", 0, 0);

  state.eip = entry;
  return grub_relocator32_boot (relocator, state, 0);
}

static grub_err_t
grub_chain_unload (void)
{
  grub_relocator_unload (relocator);
  relocator = NULL;

  return GRUB_ERR_NONE;
}

static grub_err_t
load_elf (grub_file_t file, const char *filename)
{
  grub_elf_t elf;
  Elf32_Phdr *phdr;
  grub_err_t err;

  elf = grub_elf_file (file, filename);
  if (!elf)
    return grub_errno;

  if (!grub_elf_is_elf32 (elf))
    return grub_error (GRUB_ERR_BAD_OS, "only ELF32 can be coreboot payload");

  entry = elf->ehdr.ehdr32.e_entry;

  FOR_ELF32_PHDRS(elf, phdr)
    {
      grub_uint8_t *load_addr;
      grub_relocator_chunk_t ch;

      if (phdr->p_type != PT_LOAD)
	continue;

      err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					     phdr->p_paddr, phdr->p_memsz);
      if (err)
	{
	  elf->file = 0;
	  grub_elf_close (elf);
	  return err;
	}

      load_addr = get_virtual_current_address (ch);

      if (grub_file_seek (elf->file, phdr->p_offset) == (grub_off_t) -1)
	{
	  elf->file = 0;
	  grub_elf_close (elf);
	  return grub_errno;
	}

      if (phdr->p_filesz)
	{
	  grub_ssize_t read;
	  read = grub_file_read (elf->file, load_addr, phdr->p_filesz);
	  if (read != (grub_ssize_t) phdr->p_filesz)
	    {
	      if (!grub_errno)
		grub_error (GRUB_ERR_FILE_READ_ERROR,
			    N_("premature end of file %s"),
			    filename);
	      elf->file = 0;
	      grub_elf_close (elf);
	      return grub_errno;
	    }
	}

      if (phdr->p_filesz < phdr->p_memsz)
	grub_memset ((load_addr + phdr->p_filesz),
		     0, phdr->p_memsz - phdr->p_filesz);
    }

  elf->file = 0;
  grub_elf_close (elf);
  return GRUB_ERR_NONE;
}

static grub_err_t
load_segment (grub_file_t file, const char *filename,
	      void *load_addr, grub_uint32_t comp,
	      grub_size_t size)
{
  switch (comp)
    {
    case grub_cpu_to_be32_compile_time (CBFS_COMPRESS_NONE):
      if (grub_file_read (file, load_addr, size)
	  != (grub_ssize_t) size)
	{
	  if (!grub_errno)
	    grub_error (GRUB_ERR_FILE_READ_ERROR,
			N_("premature end of file %s"),
			filename);
	      return grub_errno;
	}
      return GRUB_ERR_NONE;
    default:
      return grub_error (GRUB_ERR_BAD_OS, "unsupported compression %d",
			 grub_be_to_cpu32 (comp));
    }
}

static grub_err_t
load_chewed (grub_file_t file, const char *filename)
{
  grub_size_t i;
  for (i = 0;; i++)
    {
      struct cbfs_payload_segment segment;
      grub_err_t err;

      if (grub_file_seek (file, sizeof (segment) * i) == (grub_off_t) -1
	  || grub_file_read (file, &segment, sizeof (segment))
	  != sizeof (segment))
	{
	  if (!grub_errno)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "payload is too short");
	  return grub_errno;
	}

      switch (segment.type)
	{
	case PAYLOAD_SEGMENT_PARAMS:
	  break;

	case PAYLOAD_SEGMENT_ENTRY:
	  entry = grub_be_to_cpu64 (segment.load_addr);
	  return GRUB_ERR_NONE;

	case PAYLOAD_SEGMENT_BSS:
	  segment.len = 0;
	  segment.offset = 0;
	  segment.len = 0;
	case PAYLOAD_SEGMENT_CODE:
	case PAYLOAD_SEGMENT_DATA:
	  {
	    grub_uint32_t target = grub_be_to_cpu64 (segment.load_addr);
	    grub_uint32_t memsize = grub_be_to_cpu32 (segment.mem_len);
	    grub_uint32_t filesize = grub_be_to_cpu32 (segment.len);
	    grub_uint8_t *load_addr;
	    grub_relocator_chunk_t ch;

	    if (memsize < filesize)
	      memsize = filesize;

	    err = grub_relocator_alloc_chunk_addr (relocator, &ch,
						   target, memsize);
	    if (err)
	      return err;

	    load_addr = get_virtual_current_address (ch);

	    if (filesize)
	      {
		if (grub_file_seek (file, grub_be_to_cpu32 (segment.offset))
		    == (grub_off_t) -1)
		  return grub_errno;

		err = load_segment (file, filename, load_addr,
				    segment.compression, filesize);
		if (err)
		  return err;
	      }

	    if (filesize < memsize)
	      grub_memset ((load_addr + filesize),
			   0, memsize - filesize);
	  }
	}
    }
}

static grub_err_t
grub_cmd_chain (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_err_t err;
  grub_file_t file;
  grub_uint32_t head;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_loader_unset ();

  file = grub_file_open (argv[0]);
  if (!file)
    return grub_errno;

  relocator = grub_relocator_new ();
  if (!relocator)
    {
      grub_file_close (file);
      return grub_errno;
    }

  if (grub_file_read (file, &head, sizeof (head)) != sizeof (head)
      || grub_file_seek (file, 0) == (grub_off_t) -1)
    {
      grub_file_close (file);
      grub_relocator_unload (relocator);
      relocator = 0;
      if (!grub_errno)
	return grub_error (GRUB_ERR_BAD_OS,
			   "payload is too short");
      return grub_errno;
    }
      
  switch (head)
    {
    case ELFMAG0 | (ELFMAG1 << 8) | (ELFMAG2 << 16) | (ELFMAG3 << 24):
      err = load_elf (file, argv[0]);
      break;
    case PAYLOAD_SEGMENT_CODE:
    case PAYLOAD_SEGMENT_DATA:
    case PAYLOAD_SEGMENT_PARAMS:
    case PAYLOAD_SEGMENT_BSS:
    case PAYLOAD_SEGMENT_ENTRY:
      err = load_chewed (file, argv[0]);
      break;

    default:
      err = grub_error (GRUB_ERR_BAD_OS, "unrecognised payload type");
      break;
    }
  grub_file_close (file);
  if (err)
    {
      grub_relocator_unload (relocator);
      relocator = 0;
      return err;
    }

  grub_loader_set (grub_chain_boot, grub_chain_unload, 0);
  return GRUB_ERR_NONE;
}

static grub_command_t cmd_chain;

GRUB_MOD_INIT (chain)
{
  cmd_chain = grub_register_command ("chainloader", grub_cmd_chain,
				     N_("FILE"),
				     /* TRANSLATORS: "payload" is a term used
					by coreboot and must be translated in
					sync with coreboot. If unsure,
					let it untranslated.  */
				     N_("Load another coreboot payload"));
}

GRUB_MOD_FINI (chain)
{
  grub_unregister_command (cmd_chain);
  grub_chain_unload ();
}
