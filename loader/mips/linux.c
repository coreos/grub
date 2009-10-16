/* linux.c - boot Linux */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005, 2007, 2009  Free Software Foundation, Inc.
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

#include <grub/elf.h>
#include <grub/elfload.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/machine/loader.h>
#include <grub/command.h>
#include <grub/mips/relocator.h>

#define ELF32_LOADMASK (0x00000000UL)
#define ELF64_LOADMASK (0x0000000000000000ULL)

static grub_dl_t my_mod;

static int loaded;

static grub_size_t initrd_size;
static grub_size_t linux_size;

static char *linux_args;

static grub_uint8_t *playground;
static grub_addr_t target_addr, entry_addr, initrd_addr, args_addr;

static grub_err_t
grub_linux_boot (void)
{
  grub_ssize_t actual;
  struct grub_relocator32_state state;

  /* Boot the kernel.  */
  state.gpr[1] = entry_addr;
  state.jumpreg = 1;
  grub_relocator32_boot (playground, target_addr, state);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_release_mem (void)
{
  grub_relocator32_free (playground);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_unload (void)
{
  grub_err_t err;

  err = grub_linux_release_mem ();
  grub_dl_unref (my_mod);

  loaded = 0;

  return err;
}

static grub_err_t
grub_linux_load32 (grub_elf_t elf, char *args)
{
  Elf32_Addr base;
  int found_addr = 0;
  int argsoff;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry_addr = elf->ehdr.ehdr32.e_entry & ~ELF32_LOADMASK;

  linux_size = grub_elf32_size (elf, &base);
  if (linux_size == 0)
    return grub_errno;
  target_addr = base;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;
  argsoff = linux_size;
  args_addr = target_addr + argsoff;
  linux_size += grub_strlen (args) + 1;

  playground = grub_relocator32_alloc (linux_size);
  if (!playground)
    return grub_errno;

  grub_memcpy (playground + argsoff, args, grub_strlen (args) + 1);

  /* Now load the segments into the area we claimed.  */
  auto grub_err_t offset_phdr (Elf32_Phdr *phdr, grub_addr_t *addr, int *do_load);
  grub_err_t offset_phdr (Elf32_Phdr *phdr, grub_addr_t *addr, int *do_load)
    {
      if (phdr->p_type != PT_LOAD)
	{
	  *do_load = 0;
	  return 0;
	}
      *do_load = 1;

      /* Linux's program headers incorrectly contain virtual addresses.
       * Translate those to physical, and offset to the area we claimed.  */
      *addr = phdr->p_paddr - base + playground;
      return 0;
    }
  return grub_elf32_load (elf, offset_phdr, 0, 0);
}

static grub_err_t
grub_linux_load64 (grub_elf_t elf)
{
  Elf64_Addr base;
  int found_addr = 0;
  int argsoff;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry_addr = elf->ehdr.ehdr64.e_entry & ~ELF64_LOADMASK;

  linux_size = grub_elf64_size (elf, &base);
  if (linux_size == 0)
    return grub_errno;
  target_addr = base;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;
  argsoff = linux_size;
  args_addr = target_addr + argsoff;
  linux_size += grub_strlen (args) + 1;

  playground = grub_relocator32_alloc (linux_size);
  if (!playground)
    return grub_errno;

  grub_memcpy (playground + argsoff, args, grub_strlen (args) + 1);

  /* Now load the segments into the area we claimed.  */
  auto grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load);
  grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load)
    {
      if (phdr->p_type != PT_LOAD)
	{
	  *do_load = 0;
	  return 0;
	}
      *do_load = 1;
      /* Linux's program headers incorrectly contain virtual addresses.
       * Translate those to physical, and offset to the area we claimed.  */
      *addr = phdr->p_paddr - base + playground;
      return 0;
    }
  return grub_elf64_load (elf, offset_phdr, 0, 0);
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_elf_t elf = 0;
  int i;
  int size;
  char *dest;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto out;
    }

  elf = grub_elf_open (argv[0]);
  if (! elf)
    goto out;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "This ELF file is not of the right type\n");
      goto out;
    }

  /* Release the previously used memory.  */
  grub_loader_unset ();

  size = sizeof ("BOOT_IMAGE=") + grub_strlen (argv[0]);
  for (i = 0; i < argc; i++)
    size += grub_strlen (argv[i]) + 1;

  linux_args = grub_malloc (size);
  if (! linux_args)
    goto out;

  /* Specify the boot file.  */
  dest = grub_stpcpy (linux_args, "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);

  for (i = 1; i < argc; i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

  if (grub_elf_is_elf32 (elf))
    grub_linux_load32 (elf, linux_args);
  else
  if (grub_elf_is_elf64 (elf))
    grub_linux_load64 (elf, linux_args);
  else
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "Unknown ELF class");
      goto out;
    }

out:

  if (elf)
    grub_elf_close (elf);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_linux_release_mem ();
      grub_dl_unref (my_mod);
      loaded = 0;
    }
  else
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 1);
      initrd_addr = 0;
      loaded = 1;
    }

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t addr;
  int found_addr = 0;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no initrd specified");
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    return grub_errno;

  size = grub_file_size (file);

  playground = grub_relocator32_realloc (playground, linux_size + size);
  if (!playground)
    {
      grub_file_close (file);
      return grub_errno;
    }

  grub_dprintf ("loader", "Loading initrd at 0x%x, size 0x%x\n", addr, size);

  if (grub_file_read (file, playground + linux_size, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      grub_file_close (file);

      return grub_errno;
    }

  initrd_addr = target_addr + linux_size;
  initrd_size = size;

  grub_file_close (file);

  return GRUB_ERR_NONE;
}

static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, "load a linux kernel");
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, "load an initrd");
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
