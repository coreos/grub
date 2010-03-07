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

#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU)
#include <grub/i386/pc/vbe.h>
#define DEFAULT_VIDEO_MODE "text"
#define HAS_VGA_TEXT 1
#define HAS_VBE 1
#else
#define DEFAULT_VIDEO_MODE "auto"
#define HAS_VGA_TEXT 0
#define HAS_VBE 0
#endif

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
static int bootdev_set;
static grub_uint32_t biosdev, slice, part;

grub_size_t
grub_multiboot_get_mbi_size (void)
{
  return 2 * sizeof (grub_uint32_t) + sizeof (struct multiboot_tag)
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (cmdline_size, MULTIBOOT_TAG_ALIGN))
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (sizeof (PACKAGE_STRING), MULTIBOOT_TAG_ALIGN))
    + (modcnt * sizeof (struct multiboot_tag_module) + total_modcmd)
    + sizeof (struct multiboot_tag_basic_meminfo)
    + ALIGN_UP (sizeof (struct multiboot_tag_bootdev), MULTIBOOT_TAG_ALIGN)
    + (sizeof (struct multiboot_tag_mmap) + grub_get_multiboot_mmap_count ()
       * sizeof (struct multiboot_mmap_entry))
    + sizeof (struct multiboot_tag_vbe) + MULTIBOOT_TAG_ALIGN - 1;
}

#ifdef GRUB_MACHINE_HAS_VBE

static grub_err_t
fill_vbe_info (struct grub_vbe_mode_info_block **vbe_mode_info_out,
	       grub_uint8_t **ptrorig)
{
  struct multiboot_tag_vbe *tag = (struct multiboot_tag_vbe *) *ptrorig;
  grub_err_t err;

  tag->type = MULTIBOOT_TAG_TYPE_VBE;
  tag->size = 0;
  err = grub_multiboot_fill_vbe_info_real ((struct grub_vbe_info_block *)
					   &(tag->vbe_control_info),
					   (struct grub_vbe_mode_info_block *) 
					   &(tag->vbe_mode_info),
					   &(tag->vbe_mode),
					   &(tag->vbe_interface_seg),
					   &(tag->vbe_interface_off),
					   &(tag->vbe_interface_len));
  if (err)
    return err;
  if (vbe_mode_info_out)
    *vbe_mode_info_out = (struct grub_vbe_mode_info_block *) 
      &(tag->vbe_mode_info);
  tag->size = sizeof (struct multiboot_tag_vbe);
  *ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  return GRUB_ERR_NONE;
}

#endif

/* Fill previously allocated Multiboot mmap.  */
static void
grub_fill_multiboot_mmap (struct multiboot_tag_mmap *tag)
{
  struct multiboot_mmap_entry *mmap_entry = tag->entries;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
    {
      mmap_entry->addr = addr;
      mmap_entry->len = size;
      switch (type)
	{
	case GRUB_MACHINE_MEMORY_AVAILABLE:
 	  mmap_entry->type = MULTIBOOT_MEMORY_AVAILABLE;
 	  break;

#ifdef GRUB_MACHINE_MEMORY_ACPI_RECLAIMABLE
	case GRUB_MACHINE_MEMORY_ACPI_RECLAIMABLE:
 	  mmap_entry->type = MULTIBOOT_MEMORY_ACPI_RECLAIMABLE;
 	  break;
#endif

#ifdef GRUB_MACHINE_MEMORY_NVS
	case GRUB_MACHINE_MEMORY_NVS:
 	  mmap_entry->type = MULTIBOOT_MEMORY_NVS;
 	  break;
#endif	  
	  
 	default:
 	  mmap_entry->type = MULTIBOOT_MEMORY_RESERVED;
 	  break;
 	}
      mmap_entry++;

      return 0;
    }

  tag->type = MULTIBOOT_TAG_TYPE_MMAP;
  tag->size = sizeof (struct multiboot_tag_mmap)
    + sizeof (struct multiboot_mmap_entry) * grub_get_multiboot_mmap_count (); 
  tag->entry_size = sizeof (struct multiboot_mmap_entry);
  tag->entry_version = 0;

  grub_mmap_iterate (hook);
}

static grub_err_t
retrieve_video_parameters (grub_uint8_t **ptrorig)
{
  grub_err_t err;
  struct grub_video_mode_info mode_info;
  void *framebuffer;
  grub_video_driver_id_t driv_id;
  struct grub_video_palette_data palette[256];
  struct multiboot_tag_framebuffer *tag
    = (struct multiboot_tag_framebuffer *) *ptrorig;

  err = grub_multiboot_set_video_mode ();
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }

  grub_video_get_palette (0, ARRAY_SIZE (palette), palette);

  driv_id = grub_video_get_driver_id ();
#if HAS_VGA_TEXT
  if (driv_id == GRUB_VIDEO_DRIVER_NONE)
    {
      struct grub_vbe_mode_info_block *vbe_mode_info;
      err = fill_vbe_info (&vbe_mode_info, ptrorig);
      if (err)
	return err;
      if (vbe_mode_info->memory_model == GRUB_VBE_MEMORY_MODEL_TEXT)
	{
	  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
	  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
	  tag->common.size = 0;

	  tag->common.framebuffer_addr = 0xb8000;
	  
	  tag->common.framebuffer_pitch = 2 * vbe_mode_info->x_resolution;	
	  tag->common.framebuffer_width = vbe_mode_info->x_resolution;
	  tag->common.framebuffer_height = vbe_mode_info->y_resolution;

	  tag->common.framebuffer_bpp = 16;
	  
	  tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
	  tag->common.size = sizeof (tag->common);
	  tag->common.reserved = 0;
	  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN);
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

  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
  tag->common.size = 0;

  tag->common.framebuffer_addr = (grub_addr_t) framebuffer;
  tag->common.framebuffer_pitch = mode_info.pitch;

  tag->common.framebuffer_width = mode_info.width;
  tag->common.framebuffer_height = mode_info.height;

  tag->common.framebuffer_bpp = mode_info.bpp;

  tag->common.reserved = 0;
      
  if (mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      unsigned i;
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED;
      tag->framebuffer_palette_num_colors = mode_info.number_of_colors;
      if (tag->framebuffer_palette_num_colors > ARRAY_SIZE (palette))
	tag->framebuffer_palette_num_colors = ARRAY_SIZE (palette);
      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common)
	+ sizeof (multiboot_uint16_t) + tag->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      for (i = 0; i < tag->framebuffer_palette_num_colors; i++)
	{
	  tag->framebuffer_palette[i].red = palette[i].r;
	  tag->framebuffer_palette[i].green = palette[i].g;
	  tag->framebuffer_palette[i].blue = palette[i].b;
	}
    }
  else
    {
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      tag->framebuffer_red_field_position = mode_info.green_field_pos;
      tag->framebuffer_red_mask_size = mode_info.green_mask_size;
      tag->framebuffer_green_field_position = mode_info.green_field_pos;
      tag->framebuffer_green_mask_size = mode_info.green_mask_size;
      tag->framebuffer_blue_field_position = mode_info.blue_field_pos;
      tag->framebuffer_blue_mask_size = mode_info.blue_mask_size;

      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common) + 6;
    }
  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN);

#if HAS_VBE
  if (driv_id == GRUB_VIDEO_DRIVER_VBE)
    {
      err = fill_vbe_info (NULL, ptrorig);
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
  grub_uint8_t *ptrorig;
  grub_uint8_t *mbistart = (grub_uint8_t *) orig + buf_off 
    + (ALIGN_UP (dest + buf_off, MULTIBOOT_TAG_ALIGN) - (dest + buf_off));
  grub_err_t err;

  if (bufsize < grub_multiboot_get_mbi_size ())
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "mbi buffer is too small");

  ptrorig = mbistart + 2 * sizeof (grub_uint32_t);

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_CMDLINE;
    tag->size = sizeof (struct multiboot_tag_string) + cmdline_size; 
    grub_memcpy (tag->string, cmdline, cmdline_size);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  }

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME;
    tag->size = sizeof (struct multiboot_tag_string) + sizeof (PACKAGE_STRING); 
    grub_memcpy (tag->string, PACKAGE_STRING, sizeof (PACKAGE_STRING));
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  }

  {
    unsigned i;
    struct module *cur;

    for (i = 0, cur = modules; i < modcnt; i++, cur = cur->next)
      {
	struct multiboot_tag_module *tag
	  = (struct multiboot_tag_module *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_MODULE;
	tag->size = sizeof (struct multiboot_tag_module) + cur->cmdline_size;
	tag->mod_start = dest + cur->start;
	tag->mod_end = tag->mod_start + cur->size;
	grub_memcpy (tag->cmdline, cur->cmdline, cur->cmdline_size);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
      }
  }

  {
    struct multiboot_tag_mmap *tag = (struct multiboot_tag_mmap *) ptrorig;
    grub_fill_multiboot_mmap (tag);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  }

  {
    struct multiboot_tag_basic_meminfo *tag
      = (struct multiboot_tag_basic_meminfo *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_BASIC_MEMINFO;
    tag->size = sizeof (struct multiboot_tag_basic_meminfo); 

    /* Convert from bytes to kilobytes.  */
    tag->mem_lower = grub_mmap_get_lower () / 1024;
    tag->mem_upper = grub_mmap_get_upper () / 1024;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  }

  if (bootdev_set)
    {
      struct multiboot_tag_bootdev *tag
	= (struct multiboot_tag_bootdev *) ptrorig;
      tag->type = MULTIBOOT_TAG_TYPE_BOOTDEV;
      tag->size = sizeof (struct multiboot_tag_bootdev); 

      tag->biosdev = biosdev;
      tag->slice = slice;
      tag->part = part;
      ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
    }

  {
    err = retrieve_video_parameters (&ptrorig);
    if (err)
      {
	grub_print_error ();
	grub_errno = GRUB_ERR_NONE;
      }
  }
  
  {
    struct multiboot_tag *tag = (struct multiboot_tag *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_END;
    tag->size = sizeof (struct multiboot_tag);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN);
  }

  ((grub_uint32_t *) mbistart)[0] = ptrorig - mbistart;
  ((grub_uint32_t *) mbistart)[1] = 0;

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
  total_modcmd += ALIGN_UP (len, MULTIBOOT_TAG_ALIGN);

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
  grub_device_t dev;

  slice = ~0;
  part = ~0;

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
      char *p0;
      p = p0 = dev->disk->partition->partmap->get_name (dev->disk->partition);
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
      grub_free (p0);
    }
  if (dev)
    grub_device_close (dev);

  bootdev_set = 1;
}
