/* boot.c - load and bootstrap a kernel */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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


#include "shared.h"

#include "freebsd.h"
#include "imgact_aout.h"
#include "i386-elf.h"

static int cur_addr;
entry_func entry_addr;
static struct mod_list mll[99];


/*
 *  The next two functions, 'load_image' and 'load_module', are the building
 *  blocks of the multiboot loader component.  They handle essentially all
 *  of the gory details of loading in a bootable image and the modules.
 */

kernel_t
load_image (char *kernel, char *arg)
{
  int len, i, exec_type = 0, align_4k = 1;
  kernel_t type = KERNEL_TYPE_NONE;
  unsigned long flags = 0, text_len = 0, data_len = 0, bss_len = 0;
  char *str = 0, *str2 = 0;
  union
    {
      struct multiboot_header *mb;
      struct exec *aout;
      Elf32_Ehdr *elf;
    }
  pu;
  /* presuming that MULTIBOOT_SEARCH is large enough to encompass an
     executable header */
  unsigned char buffer[MULTIBOOT_SEARCH];

  /* sets the header pointer to point to the beginning of the
     buffer by default */
  pu.aout = (struct exec *) buffer;

  if (!grub_open (kernel))
    return KERNEL_TYPE_NONE;

  if (!(len = grub_read (buffer, MULTIBOOT_SEARCH)) || len < 32)
    {
      grub_close ();
      
      if (!errnum)
	errnum = ERR_EXEC_FORMAT;

      return KERNEL_TYPE_NONE;
    }

  for (i = 0; i < len; i++)
    {
      if (MULTIBOOT_FOUND ((int) (buffer + i), len - i))
	{
	  flags = ((struct multiboot_header *) (buffer + i))->flags;
	  if (flags & MULTIBOOT_UNSUPPORTED)
	    {
	      grub_close ();
	      errnum = ERR_BOOT_FEATURES;
	      return KERNEL_TYPE_NONE;
	    }
	  type = KERNEL_TYPE_MULTIBOOT;
	  str2 = "Multiboot";
	  break;
	}
    }

  /* ELF loading supported if multiboot and FreeBSD.  */
  if ((type == KERNEL_TYPE_MULTIBOOT
       || grub_strcmp (pu.elf->e_ident + EI_BRAND, "FreeBSD") == 0)
      && len > sizeof (Elf32_Ehdr)
      && BOOTABLE_I386_ELF ((*((Elf32_Ehdr *) buffer))))
    {
      if (type == KERNEL_TYPE_MULTIBOOT)
	entry_addr = (entry_func) pu.elf->e_entry;
      else
	entry_addr = (entry_func) (pu.elf->e_entry & 0xFFFFFF);

      if (((int) entry_addr) < 0x100000)
	errnum = ERR_BELOW_1MB;

      /* don't want to deal with ELF program header at some random
         place in the file -- this generally won't happen */
      if (pu.elf->e_phoff == 0 || pu.elf->e_phnum == 0
	  || ((pu.elf->e_phoff + (pu.elf->e_phentsize * pu.elf->e_phnum))
	      >= len))
	errnum = ERR_EXEC_FORMAT;
      str = "elf";

      if (type == KERNEL_TYPE_NONE)
	{
	  str2 = "FreeBSD";
	  type = KERNEL_TYPE_FREEBSD;
	}
    }
  else if (flags & MULTIBOOT_AOUT_KLUDGE)
    {
      pu.mb = (struct multiboot_header *) (buffer + i);
      entry_addr = (entry_func) pu.mb->entry_addr;
      cur_addr = pu.mb->load_addr;
      /* first offset into file */
      filepos = i - (pu.mb->header_addr - cur_addr);
      text_len = pu.mb->load_end_addr - cur_addr;
      data_len = 0;
      bss_len = pu.mb->bss_end_addr - pu.mb->load_end_addr;

      if (pu.mb->header_addr < pu.mb->load_addr
	  || pu.mb->load_end_addr <= pu.mb->load_addr
	  || pu.mb->bss_end_addr < pu.mb->load_end_addr
	  || (pu.mb->header_addr - pu.mb->load_addr) > i)
	errnum = ERR_EXEC_FORMAT;

      if (cur_addr < 0x100000)
	errnum = ERR_BELOW_1MB;

      pu.aout = (struct exec *) buffer;
      exec_type = 2;
      str = "kludge";
    }
  else if (len > sizeof (struct exec) && !N_BADMAG ((*(pu.aout))))
    {
      entry_addr = (entry_func) pu.aout->a_entry;

      if (type == KERNEL_TYPE_NONE)
	{
	  /*
	   *  If it doesn't have a Multiboot header, then presume
	   *  it is either a FreeBSD or NetBSD executable.  If so,
	   *  then use a magic number of normal ordering, ZMAGIC to
	   *  determine if it is FreeBSD.
	   *
	   *  This is all because freebsd and netbsd seem to require
	   *  masking out some address bits...  differently for each
	   *  one...  plus of course we need to know which booting
	   *  method to use.
	   */
	  entry_addr = (entry_func) ((int) entry_addr & 0xFFFFFF);
	  
	  if (buffer[0] == 0xb && buffer[1] == 1)
	    {
	      type = KERNEL_TYPE_FREEBSD;
	      cur_addr = (int) entry_addr;
	      str2 = "FreeBSD";
	    }
	  else
	    {
	      type = KERNEL_TYPE_NETBSD;
	      cur_addr = (int) entry_addr & 0xF00000;
	      if (N_GETMAGIC ((*(pu.aout))) != NMAGIC)
		align_4k = 0;
	      str2 = "NetBSD";
	    }
	}

      /* first offset into file */
      filepos = N_TXTOFF ((*(pu.aout)));
      text_len = pu.aout->a_text;
      data_len = pu.aout->a_data;
      bss_len = pu.aout->a_bss;

      if (cur_addr < 0x100000)
	errnum = ERR_BELOW_1MB;

      exec_type = 1;
      str = "a.out";
    }
  else if ((*((unsigned short *) (buffer + BOOTSEC_SIG_OFFSET))
	    == BOOTSEC_SIGNATURE)
	   && ((data_len
		= (((long) *((unsigned char *)
			     (buffer + LINUX_SETUP_LEN_OFFSET))) << 9))
	       <= LINUX_SETUP_MAXLEN)
	   && ((text_len
		= (((long) *((unsigned short *)
			     (buffer + LINUX_KERNEL_LEN_OFFSET))) << 4)),
	       (data_len + text_len + SECTOR_SIZE) <= ((filemax + 15) & 0xFFFFFFF0)))
    {
      int big_linux = buffer[LINUX_SETUP_LOAD_FLAGS] & LINUX_FLAG_BIG_KERNEL;
      buffer[LINUX_SETUP_LOADER] = 0x70;
      if (! big_linux && text_len > LINUX_KERNEL_MAXLEN)
	{
	  printf (" linux 'zImage' kernel too big, try 'make bzImage'\n");
	  grub_close ();
	  errnum = ERR_WONT_FIT;
	  return KERNEL_TYPE_NONE;
	}

      printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
	      (big_linux ? "bzImage" : "zImage"), data_len, text_len);

      if (mbi.mem_lower >= 608)
	{
	  /* Video mode selection support. What a shit!  */
	  {
	    char *vga;

	    /* Find the substring "vga=".  */
	    vga = grub_strstr (arg, "vga=");
	    if (vga)
	      {
		char *value = vga + 4;
		char *vga_end;
		int vid_mode;

		/* Handle special strings.  */
		if (substring ("normal", value) < 1)
		  vid_mode = LINUX_VID_MODE_NORMAL;
		else if (substring ("ext", value) < 1)
		  vid_mode = LINUX_VID_MODE_EXTENDED;
		else if (substring ("ask", value) < 1)
		  vid_mode = LINUX_VID_MODE_ASK;
		else if (safe_parse_maxint (&value, &vid_mode))
		  ;
		else
		  {
		    /* ERRNUM is already set inside the function
		       safe_parse_maxint.  */
		    return KERNEL_TYPE_NONE;
		    grub_close ();
		  }

		/* Set the vid mode to VID_MODE. Note that this can work
		   because i386 architecture is little-endian.  */
		grub_memmove (buffer + LINUX_VID_MODE_OFFSET,
			      (char *) &vid_mode,
			      sizeof (unsigned short));
		
		/* Remove the "vga=...".  */
		vga_end = skip_to (0, vga);
		grub_memmove (vga, vga_end, grub_strlen (vga_end) + 1);
	      }
	  }
		
	  memmove ((char *) LINUX_SETUP, buffer, data_len + SECTOR_SIZE);

	  /* copy command-line plus memory hack to staging area */
	  {
	    char *src = arg;
	    char *dest = (char *) (CL_MY_LOCATION + 4);

	    memmove ((char *) CL_MY_LOCATION, "mem=", 4);

	    *((unsigned short *) CL_OFFSET) = CL_MY_LOCATION - CL_BASE_ADDR;
	    *((unsigned short *) CL_MAGIC_ADDR) = CL_MAGIC;

	    dest = convert_to_ascii (dest, 'u', (mbi.mem_upper + 0x400));
	    *(dest++) = 'K';
	    *(dest++) = ' ';

	    while (*src && *src != ' ')
	      src++;

	    while (((int) dest) < CL_MY_END_ADDR && (*(dest++) = *(src++)));

	    *dest = 0;
	  }

	  /* offset into file */
	  filepos = data_len + SECTOR_SIZE;

	  cur_addr = LINUX_STAGING_AREA + text_len;
	  if (grub_read ((char *) LINUX_STAGING_AREA, text_len)
	      >= (text_len - 16))
	    {
	      grub_close ();
	      return big_linux ? KERNEL_TYPE_BIG_LINUX : KERNEL_TYPE_LINUX;
	    }
	  else if (!errnum)
	    errnum = ERR_EXEC_FORMAT;
	}
      else
	errnum = ERR_WONT_FIT;
    }
  else				/* no recognizable format */
    errnum = ERR_EXEC_FORMAT;

  /* return if error */
  if (errnum)
    {
      grub_close ();
      return KERNEL_TYPE_NONE;
    }

  /* fill the multiboot info structure */
  mbi.cmdline = (int) arg;
  mbi.mods_count = 0;
  mbi.mods_addr = 0;
  mbi.boot_device = (current_drive << 24) | current_partition;
  mbi.flags &= ~(MB_INFO_MODS | MB_INFO_AOUT_SYMS | MB_INFO_ELF_SHDR);
  mbi.syms.a.tabsize = 0;
  mbi.syms.a.strsize = 0;
  mbi.syms.a.addr = 0;
  mbi.syms.a.pad = 0;

  printf ("   [%s-%s", str2, str);

  str = "";

  if (exec_type)		/* can be loaded like a.out */
    {
      if (flags & MULTIBOOT_AOUT_KLUDGE)
	str = "-and-data";

      printf (", loadaddr=0x%x, text%s=0x%x", cur_addr, str, text_len);

      /* read text, then read data */
      if (grub_read ((char *) RAW_ADDR (cur_addr), text_len) == text_len)
	{
	  cur_addr += text_len;

	  if (!(flags & MULTIBOOT_AOUT_KLUDGE))
	    {
	      /* we have to align to a 4K boundary */
	      if (align_4k)
		cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;
	      else
		printf (", C");

	      printf (", data=0x%x", data_len);

	      if ((grub_read ((char *) RAW_ADDR (cur_addr), data_len)
		   != data_len)
		  && !errnum)
		errnum = ERR_EXEC_FORMAT;
	      cur_addr += data_len;
	    }

	  if (!errnum)
	    {
	      memset ((char *) RAW_ADDR (cur_addr), 0, bss_len);
	      cur_addr += bss_len;

	      printf (", bss=0x%x", bss_len);
	    }
	}
      else if (!errnum)
	errnum = ERR_EXEC_FORMAT;

      if (!errnum && pu.aout->a_syms
	  && pu.aout->a_syms < (filemax - filepos))
	{
	  int symtab_err, orig_addr = cur_addr;

	  /* we should align to a 4K boundary here for good measure */
	  if (align_4k)
	    cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

	  mbi.syms.a.addr = cur_addr;

	  *((int *) RAW_ADDR (cur_addr)) = pu.aout->a_syms;
	  cur_addr += sizeof (int);
	  
	  printf (", symtab=0x%x", pu.aout->a_syms);

	  if (grub_read ((char *) RAW_ADDR (cur_addr), pu.aout->a_syms)
	      == pu.aout->a_syms)
	    {
	      cur_addr += pu.aout->a_syms;
	      mbi.syms.a.tabsize = pu.aout->a_syms;

	      if (grub_read ((char *) &i, sizeof (int)) == sizeof (int))
		{
		  *((int *) RAW_ADDR (cur_addr)) = i;
		  cur_addr += sizeof (int);

		  mbi.syms.a.strsize = i;

		  i -= sizeof (int);

		  printf (", strtab=0x%x", i);

		  symtab_err = (grub_read ((char *) RAW_ADDR (cur_addr), i)
				!= i);
		  cur_addr += i;
		}
	      else
		symtab_err = 1;
	    }
	  else
	    symtab_err = 1;

	  if (symtab_err)
	    {
	      printf ("(bad)");
	      cur_addr = orig_addr;
	      mbi.syms.a.tabsize = 0;
	      mbi.syms.a.strsize = 0;
	      mbi.syms.a.addr = 0;
	    }
	  else
	    mbi.flags |= MB_INFO_AOUT_SYMS;
	}
    }
  else
    /* ELF executable */
    {
      int loaded = 0, memaddr, memsiz, filesiz;
      Elf32_Phdr *phdr;

      /* reset this to zero for now */
      cur_addr = 0;

      /* scan for program segments */
      for (i = 0; i < pu.elf->e_phnum; i++)
	{
	  phdr = (Elf32_Phdr *)
	    (pu.elf->e_phoff + ((int) buffer)
	     + (pu.elf->e_phentsize * i));
	  if (phdr->p_type == PT_LOAD)
	    {
	      /* offset into file */
	      filepos = phdr->p_offset;
	      filesiz = phdr->p_filesz;
	      
	      if (type == KERNEL_TYPE_FREEBSD)
		memaddr = RAW_ADDR (phdr->p_paddr & 0xFFFFFF);
	      else
		memaddr = RAW_ADDR (phdr->p_paddr);
	      
	      memsiz = phdr->p_memsz;
	      if (memaddr < RAW_ADDR (0x100000))
		errnum = ERR_BELOW_1MB;
	      /* make sure we only load what we're supposed to! */
	      if (filesiz > memsiz)
		filesiz = memsiz;
	      /* mark memory as used */
	      if (cur_addr < memaddr + memsiz)
		cur_addr = memaddr + memsiz;
	      printf (", <0x%x:0x%x:0x%x>", memaddr, filesiz,
		      memsiz - filesiz);
	      /* increment number of segments */
	      loaded++;

	      /* load the segment */
	      if (memcheck (memaddr, memsiz)
		  && grub_read ((char *) memaddr, filesiz) == filesiz)
		{
		  if (memsiz > filesiz)
		    memset ((char *) (memaddr + filesiz), 0, memsiz - filesiz);
		}
	      else
		break;
	    }
	}

      if (!errnum)
	{
	  if (!loaded)
	    errnum = ERR_EXEC_FORMAT;
	  else
	    {
	      /* FIXME: load ELF symbols */
	    }
	}
    }

  if (!errnum)
    printf (", entry=0x%x]\n", (int) entry_addr);
  else
    {
      putchar ('\n');
      type = KERNEL_TYPE_NONE;
    }

  grub_close ();
  return type;
}

int
load_module (char *module, char *arg)
{
  int len;

  /* if we are supposed to load on 4K boundaries */
  cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

  if (!grub_open (module))
    return 0;

  len = grub_read ((char *) cur_addr, -1);
  if (! len)
    {
      grub_close ();
      return 0;
    }

  printf ("   [Multiboot-module @ 0x%x, 0x%x bytes]\n", cur_addr, len);

  /* these two simply need to be set if any modules are loaded at all */
  mbi.flags |= MB_INFO_MODS;
  mbi.mods_addr = (int) mll;

  mll[mbi.mods_count].cmdline = (int) arg;
  mll[mbi.mods_count].mod_start = cur_addr;
  cur_addr += len;
  mll[mbi.mods_count].mod_end = cur_addr;
  mll[mbi.mods_count].pad = 0;

  /* increment number of modules included */
  mbi.mods_count++;

  grub_close ();
  return 1;
}

int
load_initrd (char *initrd)
{
  int len;
  unsigned long *ramdisk, moveto;

  if (! grub_open (initrd))
    return 0;

  len = grub_read ((char *) cur_addr, -1);
  if (! len)
    {
      grub_close ();
      return 0;
    }

  moveto = ((mbi.mem_upper + 0x400) * 0x400 - len) & 0xfffff000;
  memmove ((void *) RAW_ADDR (moveto), (void *) cur_addr, len);

  printf ("   [Linux-initrd @ 0x%x, 0x%x bytes]\n", moveto, len);

  ramdisk = (unsigned long *) (LINUX_SETUP + LINUX_SETUP_INITRD);
  ramdisk[0] = RAW_ADDR (moveto);
  ramdisk[1] = len;

  grub_close ();
  return 1;
}


#ifdef GRUB_UTIL
/* Dummy function to fake the *BSD boot.  */
static void
bsd_boot_entry (int flags, int bootdev, int sym_start, int sym_end,
		int mem_upper, int mem_lower)
{
  stop ();
}
#endif


/*
 *  All "*_boot" commands depend on the images being loaded into memory
 *  correctly, the variables in this file being set up correctly, and
 *  the root partition being set in the 'saved_drive' and 'saved_partition'
 *  variables.
 */


void
bsd_boot (kernel_t type, int bootdev, char *arg)
{
  char *str;
  int clval = 0, i;
  struct bootinfo bi;

#ifdef GRUB_UTIL
  entry_addr = (entry_func) bsd_boot_entry;
#else
  stop_floppy ();
#endif

  while (*(++arg) && *arg != ' ');
  str = arg;
  while (*str)
    {
      if (*str == '-')
	{
	  while (*str && *str != ' ')
	    {
	      if (*str == 'C')
		clval |= RB_CDROM;
	      if (*str == 'a')
		clval |= RB_ASKNAME;
	      if (*str == 'b')
		clval |= RB_HALT;
	      if (*str == 'c')
		clval |= RB_CONFIG;
	      if (*str == 'd')
		clval |= RB_KDB;
	      if (*str == 'h')
		clval |= RB_SERIAL;
	      if (*str == 'r')
		clval |= RB_DFLTROOT;
	      if (*str == 's')
		clval |= RB_SINGLE;
	      if (*str == 'v')
		clval |= RB_VERBOSE;
	      str++;
	    }
	  continue;
	}
      str++;
    }

  if (type == KERNEL_TYPE_FREEBSD)
    {
      clval |= RB_BOOTINFO;

      bi.bi_version = BOOTINFO_VERSION;

      *arg = 0;
      while ((--arg) > (char *) MB_CMDLINE_BUF && *arg != '/');
      if (*arg == '/')
	bi.bi_kernelname = arg + 1;
      else
	bi.bi_kernelname = 0;

      bi.bi_nfs_diskless = 0;
      bi.bi_n_bios_used = 0;	/* this field is apparently unused */

      for (i = 0; i < N_BIOS_GEOM; i++)
	{
	  struct geometry geom;

	  /* XXX Should check the return value.  */
	  get_diskinfo (i + 0x80, &geom);
	  /* FIXME: If HEADS or SECTORS is greater than 255, then this will
	     break the geometry information. That is a drawback of BSD
	     but not of GRUB.  */
	  bi.bi_bios_geom[i] = (((geom.cylinders - 1) << 16)
				+ (((geom.heads - 1) & 0xff) << 8)
				+ (geom.sectors & 0xff));
	}

      bi.bi_size = sizeof (struct bootinfo);
      bi.bi_memsizes_valid = 1;
      bi.bi_bios_dev = saved_drive;
      bi.bi_basemem = mbi.mem_lower;
      bi.bi_extmem = mbi.mem_upper;

      if (mbi.flags & MB_INFO_AOUT_SYMS)
	{
	  bi.bi_symtab = mbi.syms.a.addr;
	  bi.bi_esymtab = mbi.syms.a.addr + 4
	    + mbi.syms.a.tabsize + mbi.syms.a.strsize;
	}
#if 0
      else if (mbi.flags & MB_INFO_ELF_SHDR)
	{
	  /* FIXME: Should check if a symbol table exists and, if exists,
	     pass the table to BI.  */
	}
#endif
      else
	{
	  bi.bi_symtab = 0;
	  bi.bi_esymtab = 0;
	}

      /* call entry point */
      (*entry_addr) (clval, bootdev, 0, 0, 0, ((int) (&bi)));
    }
  else
    {
      /*
       *  We now pass the various bootstrap parameters to the loaded
       *  image via the argument list.
       *
       *  This is the official list:
       *
       *  arg0 = 8 (magic)
       *  arg1 = boot flags
       *  arg2 = boot device
       *  arg3 = start of symbol table (0 if not loaded)
       *  arg4 = end of symbol table (0 if not loaded)
       *  arg5 = transfer address from image
       *  arg6 = transfer address for next image pointer
       *  arg7 = conventional memory size (640)
       *  arg8 = extended memory size (8196)
       *
       *  ...in actuality, we just pass the parameters used by the kernel.
       */

      /* call entry point */
      (*entry_addr) (clval, bootdev, 0,
		     (mbi.syms.a.addr + 4
		      + mbi.syms.a.tabsize + mbi.syms.a.strsize),
		     mbi.mem_upper, mbi.mem_lower);
    }
}
