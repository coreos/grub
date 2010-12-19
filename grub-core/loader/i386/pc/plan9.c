/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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
#include <grub/file.h>
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/partition.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/video.h>
#include <grub/mm.h>
#include <grub/cpu/relocator.h>

static grub_dl_t my_mod;
static struct grub_relocator *rel;
static grub_uint32_t eip = 0xffffffff;

#define GRUB_PLAN9_TARGET            0x100000
#define GRUB_PLAN9_ALIGN             4096
#define GRUB_PLAN9_CONFIG_ADDR       0x001200
#define GRUB_PLAN9_CONFIG_PATH_SIZE  0x000040
#define GRUB_PLAN9_CONFIG_MAGIC      "ZORT 0\r\n"

struct grub_plan9_header
{
  grub_uint32_t magic;
#define GRUB_PLAN9_MAGIC 0x1eb
  grub_uint32_t text_size;
  grub_uint32_t data_size;
  grub_uint32_t bss_size;
  grub_uint32_t sectiona;
  grub_uint32_t entry_addr;
  grub_uint32_t zero;
  grub_uint32_t sectionb;
};

static grub_err_t
grub_plan9_boot (void)
{
  struct grub_relocator32_state state = { 
    .eax = 0,
    .eip = eip,
    .ebx = 0,
    .ecx = 0,
    .edx = 0,
    .edi = 0,
    .esp = 0,
    .ebp = 0,
    .esi = 0
    /*   grub_uint32_t ebp;
  grub_uint32_t esi;
*/
  };
  grub_video_set_mode ("text", 0, 0);

  return grub_relocator32_boot (rel, state);
}

static grub_err_t
grub_plan9_unload (void)
{
  grub_relocator_unload (rel);
  rel = NULL;
  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_plan9 (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_err_t err;
  void *mem;
  grub_uint8_t *ptr;
  grub_size_t memsize, padsize;
  struct grub_plan9_header hdr;
  char *config, *configptr;
  grub_size_t configsize;
  int i;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");

  grub_dl_ref (my_mod);

  rel = grub_relocator_new ();
  if (!rel)
    goto fail;

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  if (grub_file_read (file, &hdr, sizeof (hdr)) != (grub_ssize_t) sizeof (hdr))
    goto fail;

  if (grub_be_to_cpu32 (hdr.magic) != GRUB_PLAN9_MAGIC
      || hdr.zero)
    {
      grub_error (GRUB_ERR_BAD_OS, "unsupported Plan9");
      goto fail;
    }

  memsize = ALIGN_UP (grub_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
		       GRUB_PLAN9_ALIGN);
  memsize += ALIGN_UP (grub_be_to_cpu32 (hdr.data_size), GRUB_PLAN9_ALIGN);
  memsize += ALIGN_UP(grub_be_to_cpu32 (hdr.bss_size), GRUB_PLAN9_ALIGN);
  eip = grub_be_to_cpu32 (hdr.entry_addr) & 0xfffffff;

  /* path */
  configsize = GRUB_PLAN9_CONFIG_PATH_SIZE;
  /* magic */
  configsize += sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1;
  for (i = 1; i < argc; i++)
    configsize += grub_strlen (argv[i]) + 1;
  /* Terminating \0.  */
  configsize++;

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (rel, &ch, GRUB_PLAN9_CONFIG_ADDR,
					   configsize);
    if (err)
      goto fail;
    config = get_virtual_current_address (ch);
  }

  grub_memset (config, 0, GRUB_PLAN9_CONFIG_PATH_SIZE);
  configptr = config + GRUB_PLAN9_CONFIG_PATH_SIZE;
  grub_memcpy (configptr, GRUB_PLAN9_CONFIG_MAGIC,
	       sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1);
  configptr += sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1;
  for (i = 1; i < argc; i++)
    {
      configptr = grub_stpcpy (configptr, argv[i]);
      *configptr++ = '\n';
    }
  *configptr = 0;

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (rel, &ch, GRUB_PLAN9_TARGET,
					   memsize);
    if (err)
      goto fail;
    mem = get_virtual_current_address (ch);
  }

  ptr = mem;
  grub_memcpy (ptr, &hdr, sizeof (hdr));
  ptr += sizeof (hdr);

  if (grub_file_read (file, ptr, grub_be_to_cpu32 (hdr.text_size))
      != (grub_ssize_t) grub_be_to_cpu32 (hdr.text_size))
    goto fail;
  ptr += grub_be_to_cpu32 (hdr.text_size);
  padsize = ALIGN_UP (grub_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
		      GRUB_PLAN9_ALIGN) - grub_be_to_cpu32 (hdr.text_size)
    - sizeof (hdr);

  grub_memset (ptr, 0, padsize);
  ptr += padsize;

  if (grub_file_read (file, ptr, grub_be_to_cpu32 (hdr.data_size))
      != (grub_ssize_t) grub_be_to_cpu32 (hdr.data_size))
    goto fail;
  ptr += grub_be_to_cpu32 (hdr.data_size);
  padsize = ALIGN_UP (grub_be_to_cpu32 (hdr.data_size), GRUB_PLAN9_ALIGN)
    - grub_be_to_cpu32 (hdr.data_size);

  grub_memset (ptr, 0, padsize);
  ptr += padsize;
  grub_memset (ptr, 0, ALIGN_UP(grub_be_to_cpu32 (hdr.bss_size), GRUB_PLAN9_ALIGN));

  grub_loader_set (grub_plan9_boot, grub_plan9_unload, 1);
  return GRUB_ERR_NONE;

 fail:

  if (file)
    grub_file_close (file);

  grub_plan9_unload ();

  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(plan9)
{
  cmd = grub_register_command ("plan9", grub_cmd_plan9,
			       0, N_("Load Plan9 kernel."));
  my_mod = mod;
}

GRUB_MOD_FINI(plan9)
{
  grub_unregister_command (cmd);
}
