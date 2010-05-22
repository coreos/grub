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
#include <grub/cpu/relocator.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/partition.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/video.h>

#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_MULTIBOOT) || defined (GRUB_MACHINE_QEMU)
#include <grub/i386/pc/vbe.h>
#define HAS_VGA_TEXT 1
#else
#define HAS_VGA_TEXT 0
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

grub_err_t
grub_multiboot_load (grub_file_t file)
{
  char *buffer;
  grub_ssize_t len;
  struct multiboot_header *header;
  grub_err_t err;
  struct multiboot_header_tag *tag;
  struct multiboot_header_tag_address *addr_tag = NULL;
  int entry_specified = 0;
  grub_addr_t entry = 0;
  grub_uint32_t console_required = 0;
  struct multiboot_header_tag_framebuffer *fbtag = NULL;
  int accepted_consoles = GRUB_MULTIBOOT_CONSOLE_EGA_TEXT;

  buffer = grub_malloc (MULTIBOOT_SEARCH);
  if (!buffer)
    return grub_errno;

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_BAD_OS, "file too small");
    }

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
       ((char *) header <= buffer + len - 12) || (header = 0);
       header = (struct multiboot_header *) ((char *) header + MULTIBOOT_HEADER_ALIGN))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
	  && !(header->magic + header->architecture
	       + header->header_length + header->checksum)
	  && header->architecture == MULTIBOOT_ARCHITECTURE_CURRENT)
	break;
    }

  if (header == 0)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "no multiboot header found");
    }

  for (tag = (struct multiboot_header_tag *) (header + 1);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_header_tag *) ((char *) tag + tag->size))
    switch (tag->type)
      {
      case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:
	{
	  unsigned i;
	  struct multiboot_header_tag_information_request *request_tag
	    = (struct multiboot_header_tag_information_request *) tag;
	  if (request_tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)
	    break;
	  for (i = 0; i < (request_tag->size - sizeof (request_tag))
		 / sizeof (request_tag->requests[0]); i++)
	    switch (request_tag->requests[i])
	      {
	      case MULTIBOOT_TAG_TYPE_END:
	      case MULTIBOOT_TAG_TYPE_CMDLINE:
	      case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	      case MULTIBOOT_TAG_TYPE_MODULE:
	      case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
	      case MULTIBOOT_TAG_TYPE_BOOTDEV:
	      case MULTIBOOT_TAG_TYPE_MMAP:
	      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
		break;

	      case MULTIBOOT_TAG_TYPE_VBE:
	      case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
	      case MULTIBOOT_TAG_TYPE_APM:
	      default:
		grub_free (buffer);
		return grub_error (GRUB_ERR_UNKNOWN_OS,
				   "unsupported information tag: 0x%x",
				   request_tag->requests[i]);
	      }
	  break;
	}
	       
      case MULTIBOOT_HEADER_TAG_ADDRESS:
	addr_tag = (struct multiboot_header_tag_address *) tag;
	break;

      case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS:
	entry_specified = 1;
	entry = ((struct multiboot_header_tag_entry_address *) tag)->entry_addr;
	break;

      case MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS:
	if (!(((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED))
	  accepted_consoles &= ~GRUB_MULTIBOOT_CONSOLE_EGA_TEXT;
	if (((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED)
	  console_required = 1;
	break;

      case MULTIBOOT_HEADER_TAG_FRAMEBUFFER:
	fbtag = (struct multiboot_header_tag_framebuffer *) tag;
	accepted_consoles |= GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER;
	break;

	/* GRUB always page-aligns modules.  */
      case MULTIBOOT_HEADER_TAG_MODULE_ALIGN:
	break;

      default:
	if (! (tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL))
	  {
	    grub_free (buffer);
	    return grub_error (GRUB_ERR_UNKNOWN_OS,
			       "unsupported tag: 0x%x", tag->type);
	  }
	break;
      }

  if (addr_tag && !entry_specified)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_UNKNOWN_OS,
			 "load address tag without entry address tag");
    }
 
  if (addr_tag)
    {
      int offset = ((char *) header - buffer -
		    (addr_tag->header_addr - addr_tag->load_addr));
      int load_size = ((addr_tag->load_end_addr == 0) ? file->size - offset :
		       addr_tag->load_end_addr - addr_tag->load_addr);
      grub_size_t code_size;

      if (addr_tag->bss_end_addr)
	code_size = (addr_tag->bss_end_addr - addr_tag->load_addr);
      else
	code_size = load_size;
      grub_multiboot_payload_dest = addr_tag->load_addr;

      grub_multiboot_pure_size += code_size;

      /* Allocate a bit more to avoid relocations in most cases.  */
      grub_multiboot_alloc_mbi = grub_multiboot_get_mbi_size () + 65536;
      grub_multiboot_payload_orig
	= grub_relocator32_alloc (grub_multiboot_pure_size + grub_multiboot_alloc_mbi);

      if (! grub_multiboot_payload_orig)
	{
	  grub_free (buffer);
	  return grub_errno;
	}

      if ((grub_file_seek (file, offset)) == (grub_off_t) -1)
	{
	  grub_free (buffer);
	  return grub_errno;
	}

      grub_file_read (file, (void *) grub_multiboot_payload_orig, load_size);
      if (grub_errno)
	{
	  grub_free (buffer);
	  return grub_errno;
	}

      if (addr_tag->bss_end_addr)
	grub_memset ((void *) (grub_multiboot_payload_orig + load_size), 0,
		     addr_tag->bss_end_addr - addr_tag->load_addr - load_size);
    }
  else
    {
      err = grub_multiboot_load_elf (file, buffer);
      if (err)
	{
	  grub_free (buffer);
	  return err;
	}
    }

  if (entry_specified)
    grub_multiboot_payload_eip = entry;

  if (fbtag)
    err = grub_multiboot_set_console (GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER,
				      accepted_consoles,
				      fbtag->width, fbtag->height,
				      fbtag->depth, console_required);
  else
    err = grub_multiboot_set_console (GRUB_MULTIBOOT_CONSOLE_EGA_TEXT,
				      accepted_consoles,
				      0, 0, 0, console_required);
  return err;
}

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
      struct grub_vbe_mode_info_block vbe_mode_info;
      grub_uint32_t vbe_mode;

#if defined (GRUB_MACHINE_PCBIOS)
      {
	grub_vbe_status_t status;
	void *scratch = (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
	status = grub_vbe_bios_get_mode (scratch);
	vbe_mode = *(grub_uint32_t *) scratch;
	if (status != GRUB_VBE_STATUS_OK)
	  return GRUB_ERR_NONE;
      }
#else
      vbe_mode = 3;
#endif

      /* get_mode_info isn't available for mode 3.  */
      if (vbe_mode == 3)
	{
	  grub_memset (&vbe_mode_info, 0,
		       sizeof (struct grub_vbe_mode_info_block));
	  vbe_mode_info.memory_model = GRUB_VBE_MEMORY_MODEL_TEXT;
	  vbe_mode_info.x_resolution = 80;
	  vbe_mode_info.y_resolution = 25;
	}
#if defined (GRUB_MACHINE_PCBIOS)
      else
	{
	  grub_vbe_status_t status;
	  void *scratch = (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
	  status = grub_vbe_bios_get_mode_info (vbe_mode, scratch);
	  if (status != GRUB_VBE_STATUS_OK)
	    return GRUB_ERR_NONE;
	  grub_memcpy (&vbe_mode_info, scratch,
		       sizeof (struct grub_vbe_mode_info_block));
	}
#endif

      if (vbe_mode_info.memory_model == GRUB_VBE_MEMORY_MODEL_TEXT)
	{
	  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
	  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
	  tag->common.size = 0;

	  tag->common.framebuffer_addr = 0xb8000;
	  
	  tag->common.framebuffer_pitch = 2 * vbe_mode_info.x_resolution;	
	  tag->common.framebuffer_width = vbe_mode_info.x_resolution;
	  tag->common.framebuffer_height = vbe_mode_info.y_resolution;

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
      if (dev->disk->partition->parent)
 	{
	  part = dev->disk->partition->number;
	  slice = dev->disk->partition->parent->number;
	}
      else
	slice = dev->disk->partition->number;
    }
  if (dev)
    grub_device_close (dev);

  bootdev_set = 1;
}
