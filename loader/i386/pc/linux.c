/* linux.c - boot Linux zImage or bzImage */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
 *  Copyright (C) 2003  Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/loader.h>
#include <pupa/machine/loader.h>
#include <pupa/file.h>
#include <pupa/err.h>
#include <pupa/device.h>
#include <pupa/disk.h>
#include <pupa/misc.h>
#include <pupa/types.h>
#include <pupa/machine/init.h>
#include <pupa/machine/memory.h>
#include <pupa/rescue.h>
#include <pupa/dl.h>
#include <pupa/machine/linux.h>

static pupa_dl_t my_mod;

static int big_linux;
static pupa_size_t linux_mem_size;
static int loaded;

static pupa_err_t
pupa_linux_boot (void)
{
  if (big_linux)
    pupa_linux_boot_bzimage ();
  else
    pupa_linux_boot_zimage ();

  /* Never reach here.  */
  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_linux_unload (void)
{
  pupa_dl_unref (my_mod);
  loaded = 0;
  return PUPA_ERR_NONE;
}

void
pupa_rescue_cmd_linux (int argc, char *argv[])
{
  pupa_file_t file = 0;
  struct linux_kernel_header lh;
  pupa_uint8_t setup_sects;
  pupa_size_t real_size, prot_size;
  pupa_ssize_t len;
  int i;
  char *dest;

  pupa_dl_ref (my_mod);
  
  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = pupa_file_open (argv[0]);
  if (! file)
    goto fail;

  if (pupa_file_size (file) > (pupa_ssize_t) pupa_os_area_size)
    {
      pupa_error (PUPA_ERR_OUT_OF_RANGE, "too big kernel");
      goto fail;
    }

  if (pupa_file_read (file, (char *) &lh, sizeof (lh)) != sizeof (lh))
    {
      pupa_error (PUPA_ERR_READ_ERROR, "cannot read the linux header");
      goto fail;
    }

  if (lh.boot_flag != pupa_cpu_to_le16 (0xaa55))
    {
      pupa_error (PUPA_ERR_BAD_OS, "invalid magic number");
      goto fail;
    }

  if (lh.setup_sects > PUPA_LINUX_MAX_SETUP_SECTS)
    {
      pupa_error (PUPA_ERR_BAD_OS, "too many setup sectors");
      goto fail;
    }

  big_linux = 0;
  setup_sects = lh.setup_sects;
  linux_mem_size = 0;
  
  if (lh.header == pupa_cpu_to_le32 (PUPA_LINUX_MAGIC_SIGNATURE)
      && pupa_le_to_cpu16 (lh.version) >= 0x0200)
    {
      big_linux = (lh.loadflags & PUPA_LINUX_FLAG_BIG_KERNEL);
      lh.type_of_loader = PUPA_LINUX_BOOT_LOADER_TYPE;
      
      /* Put the real mode part at as a high location as possible.  */
      pupa_linux_real_addr = (char *) (pupa_lower_mem
				       - PUPA_LINUX_SETUP_MOVE_SIZE);
      /* But it must not exceed the traditional area.  */
      if (pupa_linux_real_addr > (char *) PUPA_LINUX_OLD_REAL_MODE_ADDR)
	pupa_linux_real_addr = (char *) PUPA_LINUX_OLD_REAL_MODE_ADDR;
      
      if (pupa_le_to_cpu16 (lh.version) >= 0x0201)
	{
	  lh.heap_end_ptr = pupa_cpu_to_le16 (PUPA_LINUX_HEAP_END_OFFSET);
	  lh.loadflags |= PUPA_LINUX_FLAG_CAN_USE_HEAP;
	}
      
      if (pupa_le_to_cpu16 (lh.version) >= 0x0202)
	lh.cmd_line_ptr = pupa_linux_real_addr + PUPA_LINUX_CL_OFFSET;
      else
	{
	  lh.cl_magic = pupa_cpu_to_le16 (PUPA_LINUX_CL_MAGIC);
	  lh.cl_offset = pupa_cpu_to_le16 (PUPA_LINUX_CL_OFFSET);
	  lh.setup_move_size = pupa_cpu_to_le16 (PUPA_LINUX_SETUP_MOVE_SIZE);
	}
    }
  else
    {
      /* Your kernel is quite old...  */
      lh.cl_magic = pupa_cpu_to_le16 (PUPA_LINUX_CL_MAGIC);
      lh.cl_offset = pupa_cpu_to_le16 (PUPA_LINUX_CL_OFFSET);
      
      setup_sects = PUPA_LINUX_DEFAULT_SETUP_SECTS;
      
      pupa_linux_real_addr = (char *) PUPA_LINUX_OLD_REAL_MODE_ADDR;
    }
  
  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = PUPA_LINUX_DEFAULT_SETUP_SECTS;
  
  real_size = setup_sects << PUPA_DISK_SECTOR_BITS;
  prot_size = pupa_file_size (file) - real_size - PUPA_DISK_SECTOR_SIZE;
  
  pupa_linux_tmp_addr = (char *) PUPA_LINUX_BZIMAGE_ADDR + prot_size;

  if (! big_linux
      && prot_size > (pupa_size_t) (pupa_linux_real_addr
				    - (char *) PUPA_LINUX_ZIMAGE_ADDR))
    {
      pupa_error (PUPA_ERR_BAD_OS, "too big zImage, use bzImage instead");
      goto fail;
    }
  
  if (pupa_linux_real_addr + PUPA_LINUX_SETUP_MOVE_SIZE
      > (char *) pupa_lower_mem)
    {
      pupa_error (PUPA_ERR_OUT_OF_RANGE, "too small lower memory");
      goto fail;
    }

  pupa_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
	       big_linux ? "bzImage" : "zImage", real_size, prot_size);

  for (i = 1; i < argc; i++)
    if (pupa_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	pupa_uint16_t vid_mode;
	char *val = argv[i] + 4;

	if (pupa_strcmp (val, "normal") == 0)
	  vid_mode = PUPA_LINUX_VID_MODE_NORMAL;
	else if (pupa_strcmp (val, "ext") == 0)
	  vid_mode = PUPA_LINUX_VID_MODE_EXTENDED;
	else if (pupa_strcmp (val, "ask") == 0)
	  vid_mode = PUPA_LINUX_VID_MODE_ASK;
	else
	  vid_mode = (pupa_uint16_t) pupa_strtoul (val, 0, 0);

	if (pupa_errno)
	  goto fail;

	lh.vid_mode = pupa_cpu_to_le16 (vid_mode);
      }
    else if (pupa_memcmp (argv[i], "mem=", 4) == 0)
      {
	char *val = argv[i] + 4;
	  
	linux_mem_size = pupa_strtoul (val, &val, 0);
	
	if (pupa_errno)
	  {
	    pupa_errno = PUPA_ERR_NONE;
	    linux_mem_size = 0;
	  }
	else
	  {
	    int shift = 0;
	    
	    switch (pupa_tolower (val[0]))
	      {
	      case 'g':
		shift += 10;
	      case 'm':
		shift += 10;
	      case 'k':
		shift += 10;
	      default:
		break;
	      }

	    /* Check an overflow.  */
	    if (linux_mem_size > (~0UL >> shift))
	      linux_mem_size = 0;
	    else
	      linux_mem_size <<= shift;
	  }
      }

  /* Put the real mode code at the temporary address.  */
  pupa_memmove (pupa_linux_tmp_addr, &lh, sizeof (lh));

  len = real_size + PUPA_DISK_SECTOR_SIZE - sizeof (lh);
  if (pupa_file_read (file, pupa_linux_tmp_addr + sizeof (lh), len) != len)
    {
      pupa_error (PUPA_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  if (lh.header != pupa_cpu_to_le32 (PUPA_LINUX_MAGIC_SIGNATURE)
      || pupa_le_to_cpu16 (lh.version) < 0x0200)
    /* Clear the heap space.  */
    pupa_memset (pupa_linux_tmp_addr
		 + ((setup_sects + 1) << PUPA_DISK_SECTOR_BITS),
		 0,
		 ((PUPA_LINUX_MAX_SETUP_SECTS - setup_sects - 1)
		  << PUPA_DISK_SECTOR_BITS));

  /* Copy kernel parameters.  */
  for (i = 1, dest = pupa_linux_tmp_addr + PUPA_LINUX_CL_OFFSET;
       i < argc
	 && dest + pupa_strlen (argv[i]) < (pupa_linux_tmp_addr
					    + PUPA_LINUX_CL_END_OFFSET);
       i++, *dest++ = ' ')
    {
      pupa_strcpy (dest, argv[i]);
      dest += pupa_strlen (argv[i]);
    }

  if (i != 1)
    dest--;

  *dest = '\0';

  len = prot_size;
  if (pupa_file_read (file, (char *) PUPA_LINUX_BZIMAGE_ADDR, len) != len)
    pupa_error (PUPA_ERR_FILE_READ_ERROR, "Couldn't read file");
 
  if (pupa_errno == PUPA_ERR_NONE)
    {
      pupa_linux_prot_size = prot_size;
      pupa_loader_set (pupa_linux_boot, pupa_linux_unload);
      loaded = 1;
    }

 fail:
  
  if (file)
    pupa_file_close (file);

  if (pupa_errno != PUPA_ERR_NONE)
    {
      pupa_dl_unref (my_mod);
      loaded = 0;
    }
}

void
pupa_rescue_cmd_initrd (int argc, char *argv[])
{
  pupa_file_t file = 0;
  pupa_ssize_t size;
  pupa_addr_t addr_max, addr_min, addr;
  struct linux_kernel_header *lh;

  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }
  
  if (!loaded)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  lh = (struct linux_kernel_header *) pupa_linux_tmp_addr;

  if (!(lh->header == pupa_cpu_to_le32 (PUPA_LINUX_MAGIC_SIGNATURE)
	&& pupa_le_to_cpu16 (lh->version) >= 0x0200))
    {
      pupa_error (PUPA_ERR_BAD_OS, "The kernel is too old for initrd.");
      goto fail;
    }

  /* Get the highest address available for the initrd.  */
  if (pupa_le_to_cpu16 (lh->version) >= 0x0203)
    addr_max = pupa_cpu_to_le32 (lh->initrd_addr_max);
  else
    addr_max = PUPA_LINUX_INITRD_MAX_ADDRESS;

  if (!linux_mem_size && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  if (addr_max > pupa_os_area_addr + pupa_os_area_size)
    addr_max = pupa_os_area_addr + pupa_os_area_size;

  addr_min = (pupa_addr_t) pupa_linux_tmp_addr + PUPA_LINUX_CL_END_OFFSET;

  file = pupa_file_open (argv[0]);
  if (!file)
    goto fail;

  size = pupa_file_size (file);

  /* Put the initrd as high as possible, 4Ki aligned.  */
  addr = (addr_max - size) & ~0xFFF;

  if (addr < addr_min)
    {
      pupa_error (PUPA_ERR_OUT_OF_RANGE, "The initrd is too big");
      goto fail;
    }

  if (pupa_file_read (file, (void *)addr, size) != size)
    {
      pupa_error (PUPA_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  lh->ramdisk_image = addr;
  lh->ramdisk_size = size;
  
 fail:
  if (file)
    pupa_file_close (file);
}


PUPA_MOD_INIT
{
  pupa_rescue_register_command ("linux",
				pupa_rescue_cmd_linux,
				"load linux");
  pupa_rescue_register_command ("initrd",
				pupa_rescue_cmd_initrd,
				"load initrd");
  my_mod = mod;
}

PUPA_MOD_FINI
{
  pupa_rescue_unregister_command ("linux");
  pupa_rescue_unregister_command ("initrd");
}
