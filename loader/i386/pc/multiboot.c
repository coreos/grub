/* multiboot.c - boot a multiboot OS image. */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Jeroen Dekkers  <jeroen@dekkers.cx>
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

/* 
 *  FIXME: The following features from the Multiboot specification still
 *         need to be implemented:
 *  - VBE support
 *  - a.out support
 *  - boot device
 *  - symbol table
 *  - memory map
 *  - drives table
 *  - ROM configuration table
 *  - APM table
 */

#include <pupa/loader.h>
#include <pupa/machine/loader.h>
#include <pupa/machine/multiboot.h>
#include <pupa/machine/init.h>
#include <pupa/elf.h>
#include <pupa/file.h>
#include <pupa/err.h>
#include <pupa/rescue.h>
#include <pupa/dl.h>
#include <pupa/mm.h>
#include <pupa/misc.h>

static pupa_dl_t my_mod;
static struct pupa_multiboot_info *mbi;
static pupa_addr_t entry;

static pupa_err_t
pupa_multiboot_boot (void)
{
  pupa_multiboot_real_boot (entry, mbi);

  /* Not reached.  */
  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_multiboot_unload (void)
{
  if (mbi)
    {
      int i;
      for (i = 0; i < mbi->mods_count; i++)
	{
	  pupa_free ((void *)
		     ((struct pupa_mod_list *) mbi->mods_addr)[i].mod_start);
	  pupa_free ((void *)
		     ((struct pupa_mod_list *) mbi->mods_addr)[i].cmdline);
	}
      pupa_free ((void *) mbi->mods_addr);
      pupa_free ((void *) mbi->cmdline);
      pupa_free (mbi);
    }


  mbi = 0;
  pupa_dl_unref (my_mod);

  return PUPA_ERR_NONE;
}

void
pupa_rescue_cmd_multiboot (int argc, char *argv[])
{
  pupa_file_t file = 0;
  char buffer[PUPA_MB_SEARCH], *cmdline = 0, *p;
  struct pupa_multiboot_header *header;
  pupa_ssize_t len;
  int i;
  Elf32_Ehdr *ehdr;

  pupa_dl_ref (my_mod);

  pupa_loader_unset();
    
  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "No kernel specified");
      goto fail;
    }

  file = pupa_file_open (argv[0]);
  if (!file)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "Couldn't open file");
      goto fail;
    }

  len = pupa_file_read (file, buffer, PUPA_MB_SEARCH);
  if (len < 32)
    {
      pupa_error (PUPA_ERR_BAD_OS, "File too small");
      goto fail;
    }

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct pupa_multiboot_header *) buffer; 
       ((char *) header <= buffer + len - 12) || (header = 0);
       (char *)header += 4)
    {
      if (header->magic == PUPA_MB_MAGIC 
	  && !(header->magic + header->flags + header->checksum))
	  break;
    }
  
  if (header == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "No multiboot header found");
      goto fail;
    }

  if (header->flags & PUPA_MB_UNSUPPORTED)
    {
      pupa_error (PUPA_ERR_UNKNOWN_OS, "Unsupported flag: 0x%x", header->flags);
      goto fail;
    }

  ehdr = (Elf32_Ehdr *) buffer;

  if (!((ehdr->e_ident[EI_MAG0] == ELFMAG0) 
	&& (ehdr->e_ident[EI_MAG1] == ELFMAG1)
	&& (ehdr->e_ident[EI_MAG2] == ELFMAG2) 
	&& (ehdr->e_ident[EI_MAG3] == ELFMAG3)
	&& (ehdr->e_ident[EI_CLASS] == ELFCLASS32) 
	&& (ehdr->e_ident[EI_DATA] == ELFDATA2LSB)
	&& (ehdr->e_ident[EI_VERSION] == EV_CURRENT) 
	&& (ehdr->e_type == ET_EXEC) && (ehdr->e_machine == EM_386) 
	&& (ehdr->e_version == EV_CURRENT)))
    {
      pupa_error (PUPA_ERR_UNKNOWN_OS, "No valid ELF header found");
      goto fail;
    }

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > PUPA_MB_SEARCH)
    {
      pupa_error (PUPA_ERR_UNKNOWN_OS, "Program header at a too high offset");
      goto fail;
    }

  entry = ehdr->e_entry;

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      Elf32_Phdr *phdr;
      phdr = (Elf32_Phdr *) (buffer + ehdr->e_phoff + i * ehdr->e_phentsize);

      if (phdr->p_type == PT_LOAD)
	{
	  /* The segment should fit in the area reserved for the OS.  */
	  if ((phdr->p_paddr < pupa_os_area_addr) 
	      || (phdr->p_paddr + phdr->p_memsz
		  > pupa_os_area_addr + pupa_os_area_size))
	    {
	      pupa_error (PUPA_ERR_BAD_OS, 
			  "Segment doesn't fit in memory reserved for the OS");
	      goto fail;
	    }
	  
	  if (pupa_file_seek (file, phdr->p_offset) == -1)
	    {
	      pupa_error (PUPA_ERR_BAD_OS, "Invalid offset in program header");
	      goto fail;
	    }

	  if (pupa_file_read (file, (void *) phdr->p_paddr, phdr->p_filesz) 
	      != (pupa_ssize_t) phdr->p_filesz)
	    {
	      pupa_error (PUPA_ERR_BAD_OS, "Couldn't read segment from file");
	      goto fail;
	    }

	  if (phdr->p_filesz < phdr->p_memsz)
	    pupa_memset ((char *) phdr->p_paddr + phdr->p_filesz, 0, 
			 phdr->p_memsz - phdr->p_filesz);
	}
    }

  mbi = pupa_malloc (sizeof (struct pupa_multiboot_info));
  if (!mbi)
    goto fail;

  mbi->flags = PUPA_MB_INFO_MEMORY;

  /* Convert from bytes to kilobytes.  */
  mbi->mem_lower = pupa_lower_mem / 1024;
  mbi->mem_upper = pupa_upper_mem / 1024;

  for (i = 0, len = 0; i < argc; i++)
    len += pupa_strlen (argv[i]) + 1;
  
  cmdline = p = pupa_malloc (len);
  if (!cmdline)
    goto fail;
  
  for (i = 0; i < argc; i++)
    {
      p = pupa_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }
  
  /* Remove the space after the last word.  */
  *(--p) = '\0';
  
  mbi->flags |= PUPA_MB_INFO_CMDLINE;
  mbi->cmdline = (pupa_uint32_t) cmdline;

  mbi->flags |= PUPA_MB_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = (pupa_uint32_t) pupa_strdup (PACKAGE_STRING);

  pupa_loader_set (pupa_multiboot_boot, pupa_multiboot_unload);

 fail:
  if (file)
    pupa_file_close (file);

  if (pupa_errno != PUPA_ERR_NONE)
    {
      pupa_free (cmdline);
      pupa_free (mbi);
      pupa_dl_unref (my_mod);
    }
}


void
pupa_rescue_cmd_module  (int argc, char *argv[])
{
  pupa_file_t file = 0;
  pupa_ssize_t size, len = 0;
  char *module = 0, *cmdline = 0, *p;
  int i;

  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }

  if (!mbi)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, 
		  "You need to load the multiboot kernel first");
      goto fail;
    }

  file = pupa_file_open (argv[0]);
  if (!file)
    goto fail;

  size = pupa_file_size (file);
  module = pupa_memalign (PUPA_MB_MOD_ALIGN, size);
  if (!module)
    goto fail;

  if (pupa_file_read (file, module, size) != size)
    {
      pupa_error (PUPA_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }
  
  for (i = 0; i < argc; i++)
    len += pupa_strlen (argv[i]) + 1;
  
  cmdline = p = pupa_malloc (len);
  if (!cmdline)
    goto fail;
  
  for (i = 0; i < argc; i++)
    {
      p = pupa_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }
  
  /* Remove the space after the last word.  */
  *(--p) = '\0';

  if (mbi->flags & PUPA_MB_INFO_MODS)
    {
      struct pupa_mod_list *modlist = (struct pupa_mod_list *) mbi->mods_addr;

      modlist = pupa_realloc (modlist, (mbi->mods_count + 1) 
			               * sizeof (struct pupa_mod_list));
      if (!modlist)
	goto fail;
      mbi->mods_addr = (pupa_uint32_t) modlist;
      modlist += mbi->mods_count;
      modlist->mod_start = (pupa_uint32_t) module;
      modlist->mod_end = (pupa_uint32_t) module + size;
      modlist->cmdline = (pupa_uint32_t) cmdline;
      modlist->pad = 0;
      mbi->mods_count++;
    }
  else
    {
      struct pupa_mod_list *modlist = pupa_malloc (sizeof (struct pupa_mod_list));
      if (!modlist)
	goto fail;
      modlist->mod_start = (pupa_uint32_t) module;
      modlist->mod_end = (pupa_uint32_t) module + size;
      modlist->cmdline = (pupa_uint32_t) cmdline;
      modlist->pad = 0;
      mbi->mods_count = 1;
      mbi->mods_addr = (pupa_uint32_t) modlist;
      mbi->flags |= PUPA_MB_INFO_MODS;
    }

 fail:
  if (file)
    pupa_file_close (file);

  if (pupa_errno != PUPA_ERR_NONE)
    {
      pupa_free (module);
      pupa_free (cmdline);
    }
}


PUPA_MOD_INIT
{
  pupa_rescue_register_command ("multiboot", pupa_rescue_cmd_multiboot,
				"load a multiboot kernel");
  pupa_rescue_register_command ("module", pupa_rescue_cmd_module,
				"load a multiboot module");
  my_mod = mod;
}

PUPA_MOD_FINI
{
  pupa_rescue_unregister_command ("multiboot");
  pupa_rescue_unregister_command ("module");
}
