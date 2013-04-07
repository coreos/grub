/* linux.c - boot Linux */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/file.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/cache.h>
#include <grub/cpu/linux.h>
#include <grub/lib/cmdline.h>

#include <libfdt.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_dl_t my_mod;

static grub_addr_t initrd_start;
static grub_size_t initrd_end;

static grub_addr_t linux_addr;
static grub_size_t linux_size;

static char *linux_args;

static grub_addr_t firmware_boot_data;
static grub_addr_t boot_data;
static grub_uint32_t machine_type;

/*
 * linux_prepare_fdt():
 *   Prepares a loaded FDT for being passed to Linux.
 *   Merges in command line parameters and sets up initrd addresses.
 */
static grub_err_t
linux_prepare_fdt (void)
{
  int node;
  int retval;
  int tmp_size;
  void *tmp_fdt;

  tmp_size = fdt_totalsize ((void *) boot_data) + FDT_ADDITIONAL_ENTRIES_SIZE;
  tmp_fdt = grub_malloc (tmp_size);
  if (!tmp_fdt)
    return GRUB_ERR_OUT_OF_MEMORY;

  fdt_open_into ((void *) boot_data, tmp_fdt, tmp_size);

  /* Find or create '/chosen' node */
  node = fdt_subnode_offset (tmp_fdt, 0, "chosen");
  if (node < 0)
    {
      grub_printf ("No 'chosen' node in FDT - creating.\n");
      node = fdt_add_subnode (tmp_fdt, 0, "chosen");
      if (node < 0)
	goto failure;
    }

  grub_printf ("linux_args: '%s'\n", linux_args);

  /* Generate and set command line */
  retval = fdt_setprop (tmp_fdt, node, "bootargs", linux_args,
			grub_strlen (linux_args) + 1);
  if (retval)
    goto failure;

  if (initrd_start && initrd_end)
    {
      /*
       * We're using physical addresses, so even if we have LPAE, we're
       * restricted to a 32-bit address space.
       */
      grub_uint32_t fdt_initrd_start = cpu_to_fdt32 (initrd_start);
      grub_uint32_t fdt_initrd_end = cpu_to_fdt32 (initrd_end);

      grub_dprintf ("loader", "Initrd @ 0x%08x-0x%08x\n",
		    initrd_start, initrd_end);

      retval = fdt_setprop (tmp_fdt, node, "linux,initrd-start",
			    &fdt_initrd_start, sizeof (fdt_initrd_start));
      if (retval)
	goto failure;
      retval = fdt_setprop (tmp_fdt, node, "linux,initrd-end",
			    &fdt_initrd_end, sizeof (fdt_initrd_end));
      if (retval)
	goto failure;
    }

  /* Copy updated FDT to its launch location */
  fdt_move (tmp_fdt, (void *) boot_data, fdt_totalsize (tmp_fdt));
  grub_free (tmp_fdt);
  fdt_pack ((void *) boot_data);

  grub_dprintf ("loader", "FDT updated for Linux boot\n");

  return GRUB_ERR_NONE;

failure:
  grub_free (tmp_fdt);
  return GRUB_ERR_BAD_ARGUMENT;
}

static grub_err_t
linux_boot (void)
{
  kernel_entry_t linuxmain;
  grub_err_t err = GRUB_ERR_NONE;

  grub_arch_sync_caches ((void *) linux_addr, linux_size);

  grub_dprintf ("loader", "Kernel at: 0x%x\n", linux_addr);

  if (!boot_data)
    {
      if (firmware_boot_data)
	{
	  grub_printf ("Using firmware-supplied boot data @ 0x%08x\n",
		       firmware_boot_data);
	  boot_data = firmware_boot_data;
	}
      else
	{
	  return GRUB_ERR_FILE_NOT_FOUND;
	}
    }

  grub_dprintf ("loader", "Boot data at: 0x%x\n", boot_data);

  if (fdt32_to_cpu (*(grub_uint32_t *) (boot_data)) == FDT_MAGIC)
    {
      grub_dprintf ("loader", "FDT @ 0x%08x\n", (grub_addr_t) boot_data);
      if (linux_prepare_fdt () != GRUB_ERR_NONE)
	{
	  grub_dprintf ("loader", "linux_prepare_fdt() failed\n");
	  return GRUB_ERR_FILE_NOT_FOUND;
	}
    }

  grub_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.
   *   Arguments to kernel:
   *     r0 - 0
   *     r1 - machine type (possibly passed from firmware)
   *     r2 - address of DTB or ATAG list
   */
  linuxmain = (kernel_entry_t) linux_addr;

#ifdef GRUB_MACHINE_EFI
  err = grub_efi_prepare_platform();
  if (err != GRUB_ERR_NONE)
    return err;
#endif

  linuxmain (0, machine_type, (void *) boot_data);

  return err;
}

/*
 * Only support zImage, so no relocations necessary
 */
static grub_err_t
linux_load (const char *filename)
{
  grub_file_t file;
  int size;

  file = grub_file_open (filename);
  if (!file)
    return GRUB_ERR_FILE_NOT_FOUND;

  size = grub_file_size (file);
  if (size == 0)
    return GRUB_ERR_FILE_READ_ERROR;

#ifdef GRUB_MACHINE_EFI
  linux_addr = (grub_addr_t) grub_efi_allocate_loader_memory (LINUX_PHYS_OFFSET, size);
#else
  linux_addr = LINUX_ADDRESS;
#endif
  grub_dprintf ("loader", "Loading Linux to 0x%08x\n",
		(grub_addr_t) linux_addr);

  if (grub_file_read (file, (void *) linux_addr, size) != size)
    {
      grub_printf ("Kernel read failed!\n");
      return GRUB_ERR_FILE_READ_ERROR;
    }

  if (*(grub_uint32_t *) (linux_addr + LINUX_ZIMAGE_OFFSET)
      != LINUX_ZIMAGE_MAGIC)
    {
      return grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("invalid zImage"));
    }

  linux_size = size;

  return GRUB_ERR_NONE;
}

static grub_err_t
linux_unload (void)
{
  grub_dl_unref (my_mod);

  grub_free (linux_args);
  linux_args = NULL;

  initrd_start = initrd_end = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  int size, retval;
  grub_file_t file;
  grub_dl_ref (my_mod);

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = grub_file_open (argv[0]);
  if (!file)
    goto fail;

  retval = linux_load (argv[0]);
  grub_file_close (file);
  if (retval != GRUB_ERR_NONE)
    goto fail;

  grub_loader_set (linux_boot, linux_unload, 1);

  size = grub_loader_cmdline_size (argc, argv);
  linux_args = grub_malloc (size + sizeof (LINUX_IMAGE));
  if (!linux_args)
    goto fail;

  /* Create kernel command line.  */
  grub_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  grub_create_loader_cmdline (argc, argv,
			      linux_args + sizeof (LINUX_IMAGE) - 1, size);

  return GRUB_ERR_NONE;

fail:
  grub_dl_unref (my_mod);
  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file;
  int size;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = grub_file_open (argv[0]);
  if (!file)
    return grub_errno;

  size = grub_file_size (file);
  if (size == 0)
    goto fail;

#ifdef GRUB_MACHINE_EFI
  initrd_start = (grub_addr_t) grub_efi_allocate_loader_memory (LINUX_INITRD_PHYS_OFFSET, size);
#else
  initrd_start = LINUX_INITRD_ADDRESS;
#endif
  grub_dprintf ("loader", "Loading initrd to 0x%08x\n",
		(grub_addr_t) initrd_start);

  if (grub_file_read (file, (void *) initrd_start, size) != size)
    goto fail;

  initrd_end = initrd_start + size;

  return GRUB_ERR_NONE;

fail:
  grub_file_close (file);

  return grub_errno;
}

static void *
load_dtb (grub_file_t dtb, int size)
{
  void *fdt;

  fdt = grub_malloc (size);
  if (!fdt)
    return NULL;

  if (grub_file_read (dtb, fdt, size) != size)
    {
      grub_free (fdt);
      return NULL;
    }

  if (fdt_check_header (fdt) != 0)
    {
      grub_free (fdt);
      return NULL;
    }

  return fdt;
}

static grub_err_t
grub_cmd_devicetree (grub_command_t cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  grub_file_t dtb;
  void *blob;
  int size;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  dtb = grub_file_open (argv[0]);
  if (!dtb)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, N_("failed to open file"));

  size = grub_file_size (dtb);
  if (size == 0)
    goto out;

  blob = load_dtb (dtb, size);
  if (!blob)
    return GRUB_ERR_FILE_NOT_FOUND;

#ifdef GRUB_MACHINE_EFI
  boot_data = (grub_addr_t) grub_efi_allocate_loader_memory (LINUX_FDT_PHYS_OFFSET, size);
#else
  boot_data = LINUX_FDT_ADDRESS;
#endif
  grub_dprintf ("loader", "Loading device tree to 0x%08x\n",
		(grub_addr_t) boot_data);
  fdt_move (blob, (void *) boot_data, fdt_totalsize (blob));
  grub_free (blob);

  /* 
   * We've successfully loaded an FDT, so any machine type passed
   * from firmware is now obsolete.
   */
  machine_type = ARM_FDT_MACHINE_TYPE;

  return GRUB_ERR_NONE;

 out:
  grub_file_close (dtb);

  return grub_errno;
}

static grub_command_t cmd_linux, cmd_initrd, cmd_devicetree;

GRUB_MOD_INIT (linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, N_("Load initrd."));
  cmd_devicetree = grub_register_command ("devicetree", grub_cmd_devicetree,
					  0, N_("Load DTB file."));
  my_mod = mod;
  firmware_boot_data = firmware_get_boot_data ();

  boot_data = (grub_addr_t) NULL;
  machine_type = firmware_get_machine_type ();
}

GRUB_MOD_FINI (linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
  grub_unregister_command (cmd_devicetree);
}
