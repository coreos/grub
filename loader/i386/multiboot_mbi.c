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

#include <grub/machine/memory.h>
#include <grub/memory.h>
#ifdef GRUB_MACHINE_PCBIOS
#include <grub/machine/biosnum.h>
#endif
#include <grub/multiboot.h>
#include <grub/cpu/multiboot.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/partition.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/video.h>

struct module
{
  struct module *next;
  grub_addr_t start;
  grub_size_t size;
  char *cmdline;
  int cmdline_size;
};

struct module *modules, *modules_last;
static grub_size_t cmdline_size;
static grub_size_t total_modcmd;
static unsigned modcnt;
static char *cmdline = NULL;
static grub_uint32_t bootdev;
static int bootdev_set;

grub_size_t
grub_multiboot_get_mbi_size (void)
{
  return sizeof (struct multiboot_info) + ALIGN_UP (cmdline_size, 4)
    + modcnt * sizeof (struct multiboot_mod_list) + total_modcmd
    + ALIGN_UP (sizeof(PACKAGE_STRING), 4) + grub_get_multiboot_mmap_len ()
#if GRUB_MACHINE_HAS_VBE
    + sizeof (struct grub_vbe_info_block)
    + sizeof (struct grub_vbe_mode_info_block)
#endif
    + 256 * sizeof (struct multiboot_color);
}

#if GRUB_MACHINE_HAS_VBE
static grub_err_t
fill_vbe_info (struct multiboot_info *mbi,
	       struct grub_vbe_mode_info_block **vbe_mode_info_out,
	       grub_uint8_t *ptrorig, grub_addr_t ptrdest)
{
  struct grub_vbe_info_block *vbe_control_info;
  struct grub_vbe_mode_info_block *vbe_mode_info;
  grub_err_t err;

  vbe_control_info = (struct grub_vbe_info_block *) ptrorig;
  mbi->vbe_control_info = ptrdest;
  ptrorig += sizeof (struct grub_vbe_info_block);
  ptrdest += sizeof (struct grub_vbe_info_block);
  vbe_mode_info = (struct grub_vbe_mode_info_block *) ptrorig;
  mbi->vbe_mode_info = ptrdest;
  ptrorig += sizeof (struct grub_vbe_mode_info_block);
  ptrdest += sizeof (struct grub_vbe_mode_info_block);
  
  err = grub_multiboot_fill_vbe_info_real (vbe_control_info, vbe_mode_info,
					   &mbi->vbe_mode,
					   &mbi->vbe_interface_seg,
					   &mbi->vbe_interface_off,
					   &mbi->vbe_interface_len);
  if (err)
    return err;
  mbi->flags |= MULTIBOOT_INFO_VBE_INFO;
  if (vbe_mode_info_out)
    *vbe_mode_info_out = vbe_mode_info;
  return GRUB_ERR_NONE;
}

#endif

static grub_err_t
retrieve_video_parameters (struct multiboot_info *mbi,
			   grub_uint8_t *ptrorig, grub_uint32_t ptrdest)
{
  grub_err_t err;
  struct grub_video_mode_info mode_info;
  void *framebuffer;
  grub_video_driver_id_t driv_id;
  struct grub_video_palette_data palette[256];

  err = grub_multiboot_set_video_mode ();
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }

  grub_video_get_palette (0, ARRAY_SIZE (palette), palette);

  driv_id = grub_video_get_driver_id ();
#if GRUB_MACHINE_HAS_VGA_TEXT
  if (driv_id == GRUB_VIDEO_DRIVER_NONE)
    {
      struct grub_vbe_mode_info_block *vbe_mode_info;
      err = fill_vbe_info (mbi, &vbe_mode_info, ptrorig, ptrdest);
      if (err)
	return err;
      if (vbe_mode_info->memory_model == GRUB_VBE_MEMORY_MODEL_TEXT)
	{
	  mbi->framebuffer_addr = 0xb8000;
	  
	  mbi->framebuffer_pitch = 2 * vbe_mode_info->x_resolution;	
	  mbi->framebuffer_width = vbe_mode_info->x_resolution;
	  mbi->framebuffer_height = vbe_mode_info->y_resolution;

	  mbi->framebuffer_bpp = 16;
	  
	  mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;

	  mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;
	}
      return GRUB_ERR_NONE;
    }
#else
  if (driv_id == GRUB_VIDEO_DRIVER_NONE)
    return GRUB_ERR_NONE;
#endif

  err = grub_video_get_info_and_fini (&mode_info, &framebuffer);
  if (err)
    return err;

  mbi->framebuffer_addr = (grub_addr_t) framebuffer;
  mbi->framebuffer_pitch = mode_info.pitch;

  mbi->framebuffer_width = mode_info.width;
  mbi->framebuffer_height = mode_info.height;

  mbi->framebuffer_bpp = mode_info.bpp;
      
  if (mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      struct multiboot_color *mb_palette;
      unsigned i;
      mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED;
      mbi->framebuffer_palette_addr = ptrdest;
      mbi->framebuffer_palette_num_colors = mode_info.number_of_colors;
      if (mbi->framebuffer_palette_num_colors > ARRAY_SIZE (palette))
	mbi->framebuffer_palette_num_colors = ARRAY_SIZE (palette);
      mb_palette = (struct multiboot_color *) ptrorig;
      for (i = 0; i < mbi->framebuffer_palette_num_colors; i++)
	{
	  mb_palette[i].red = palette[i].r;
	  mb_palette[i].green = palette[i].g;
	  mb_palette[i].blue = palette[i].b;
	}
      ptrorig += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      ptrdest += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
    }
  else
    {
      mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      mbi->framebuffer_red_field_position = mode_info.green_field_pos;
      mbi->framebuffer_red_mask_size = mode_info.green_mask_size;
      mbi->framebuffer_green_field_position = mode_info.green_field_pos;
      mbi->framebuffer_green_mask_size = mode_info.green_mask_size;
      mbi->framebuffer_blue_field_position = mode_info.blue_field_pos;
      mbi->framebuffer_blue_mask_size = mode_info.blue_mask_size;
    }

  mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;

#if GRUB_MACHINE_HAS_VBE
  if (driv_id == GRUB_VIDEO_DRIVER_VBE)
    {
      err = fill_vbe_info (mbi, NULL, ptrorig, ptrdest);
      if (err)
	return err;
    }
#endif

  return GRUB_ERR_NONE;
}

grub_err_t
grub_multiboot_make_mbi (void *orig, grub_uint32_t dest, grub_off_t buf_off,
			 grub_size_t bufsize)
{
  grub_uint8_t *ptrorig = (grub_uint8_t *) orig + buf_off;
  grub_uint32_t ptrdest = dest + buf_off;
  struct multiboot_info *mbi;
  struct multiboot_mod_list *modlist;
  unsigned i;
  struct module *cur;
  grub_size_t mmap_size;
  grub_err_t err;

  if (bufsize < grub_multiboot_get_mbi_size ())
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "mbi buffer is too small");

  mbi = (struct multiboot_info *) ptrorig;
  ptrorig += sizeof (*mbi);
  ptrdest += sizeof (*mbi);
  grub_memset (mbi, 0, sizeof (*mbi));

  grub_memcpy (ptrorig, cmdline, cmdline_size);
  mbi->flags |= MULTIBOOT_INFO_CMDLINE;
  mbi->cmdline = ptrdest;
  ptrorig += ALIGN_UP (cmdline_size, 4);
  ptrdest += ALIGN_UP (cmdline_size, 4);

  grub_memcpy (ptrorig, PACKAGE_STRING, sizeof(PACKAGE_STRING));
  mbi->flags |= MULTIBOOT_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = ptrdest;
  ptrorig += ALIGN_UP (sizeof(PACKAGE_STRING), 4);
  ptrdest += ALIGN_UP (sizeof(PACKAGE_STRING), 4);

  if (modcnt)
    {
      mbi->flags |= MULTIBOOT_INFO_MODS;
      mbi->mods_addr = ptrdest;
      mbi->mods_count = modcnt;
      modlist = (struct multiboot_mod_list *) ptrorig;
      ptrorig += modcnt * sizeof (struct multiboot_mod_list);
      ptrdest += modcnt * sizeof (struct multiboot_mod_list);

      for (i = 0, cur = modules; i < modcnt; i++, cur = cur->next)
	{
	  modlist[i].mod_start = cur->start;
	  modlist[i].mod_end = modlist[i].mod_start + cur->size;
	  modlist[i].cmdline = ptrdest;
	  grub_memcpy (ptrorig, cur->cmdline, cur->cmdline_size);
	  ptrorig += ALIGN_UP (cur->cmdline_size, 4);
	  ptrdest += ALIGN_UP (cur->cmdline_size, 4);
	}
    }
  else
    {
      mbi->mods_addr = 0;
      mbi->mods_count = 0;
    }

  mmap_size = grub_get_multiboot_mmap_len (); 
  grub_fill_multiboot_mmap ((struct multiboot_mmap_entry *) ptrorig);
  mbi->mmap_length = mmap_size;
  mbi->mmap_addr = ptrdest;
  mbi->flags |= MULTIBOOT_INFO_MEM_MAP;
  ptrorig += mmap_size;
  ptrdest += mmap_size;

  /* Convert from bytes to kilobytes.  */
  mbi->mem_lower = grub_mmap_get_lower () / 1024;
  mbi->mem_upper = grub_mmap_get_upper () / 1024;
  mbi->flags |= MULTIBOOT_INFO_MEMORY;

  if (bootdev_set)
    {
      mbi->boot_device = bootdev;
      mbi->flags |= MULTIBOOT_INFO_BOOTDEV;
    }

  err = retrieve_video_parameters (mbi, ptrorig, ptrdest);
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }
#if GRUB_MACHINE_HAS_VBE
  ptrorig += sizeof (struct grub_vbe_info_block);
  ptrdest += sizeof (struct grub_vbe_info_block);
  ptrorig += sizeof (struct grub_vbe_mode_info_block);
  ptrdest += sizeof (struct grub_vbe_mode_info_block);
#endif

  return GRUB_ERR_NONE;
}

void
grub_multiboot_free_mbi (void)
{
  struct module *cur, *next;

  cmdline_size = 0;
  total_modcmd = 0;
  modcnt = 0;
  grub_free (cmdline);
  cmdline = NULL;
  bootdev_set = 0;

  for (cur = modules; cur; cur = next)
    {
      next = cur->next;
      grub_free (cur->cmdline);
      grub_free (cur);
    }
  modules = NULL;
  modules_last = NULL;
}

grub_err_t
grub_multiboot_init_mbi (int argc, char *argv[])
{
  grub_ssize_t len = 0;
  char *p;
  int i;

  grub_multiboot_free_mbi ();

  for (i = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;
  if (len == 0)
    len = 1;

  cmdline = p = grub_malloc (len);
  if (! cmdline)
    return grub_errno;
  cmdline_size = len;

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != cmdline)
    p--;
  *p = '\0';

  return GRUB_ERR_NONE;
}

grub_err_t
grub_multiboot_add_module (grub_addr_t start, grub_size_t size,
			   int argc, char *argv[])
{
  struct module *newmod;
  char *p;
  grub_ssize_t len = 0;
  int i;

  newmod = grub_malloc (sizeof (*newmod));
  if (!newmod)
    return grub_errno;
  newmod->start = start;
  newmod->size = size;

  for (i = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;

  if (len == 0)
    len = 1;

  newmod->cmdline = p = grub_malloc (len);
  if (! newmod->cmdline)
    {
      grub_free (newmod);
      return grub_errno;
    }
  newmod->cmdline_size = len;
  total_modcmd += ALIGN_UP (len, 4);

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != newmod->cmdline)
    p--;
  *p = '\0';

  if (modules_last)
    modules_last->next = newmod;
  else
    {
      modules = newmod;
      modules_last->next = NULL;
    }
  modules_last = newmod;

  modcnt++;

  return GRUB_ERR_NONE;
}

void
grub_multiboot_set_bootdev (void)
{
  char *p;
  grub_uint32_t biosdev, slice = ~0, part = ~0;
  grub_device_t dev;

#ifdef GRUB_MACHINE_PCBIOS
  biosdev = grub_get_root_biosnumber ();
#else
  biosdev = 0xffffffff;
#endif

  if (biosdev == 0xffffffff)
    return;

  dev = grub_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {

      p = dev->disk->partition->partmap->get_name (dev->disk->partition);
      if (p)
	{
	  if ((p[0] >= '0') && (p[0] <= '9'))
	    {
	      slice = grub_strtoul (p, &p, 0) - 1;

	      if ((p) && (p[0] == ','))
		p++;
	    }

	  if ((p[0] >= 'a') && (p[0] <= 'z'))
	    part = p[0] - 'a';
	}
    }
  if (dev)
    grub_device_close (dev);

  bootdev = ((biosdev & 0xff) << 24) | ((slice & 0xff) << 16) 
    | ((part & 0xff) << 8) | 0xff;
  bootdev_set = 1;
}
