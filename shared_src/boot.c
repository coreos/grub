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

char *cur_cmdline;
static int cur_addr;
entry_func entry_addr;
static struct mod_list mll[99];


/*
 *  The next two functions, 'load_image' and 'load_module', are the building
 *  blocks of the multiboot loader component.  They handle essentially all
 *  of the gory details of loading in a bootable image and the modules.
 */

int
load_image (void)
{
  int len, i, exec_type = 0, align_4k = 1, type = 0;
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

  if (!grub_open (cur_cmdline))
    return 0;

  if (!(len = grub_read (buffer, MULTIBOOT_SEARCH)) || len < 32)
    {
      if (!errnum)
	errnum = ERR_EXEC_FORMAT;

      return 0;
    }

  for (i = 0; i < len; i++)
    {
      if (MULTIBOOT_FOUND ((int) (buffer + i), len - i))
	{
	  flags = ((struct multiboot_header *) (buffer + i))->flags;
	  if (flags & MULTIBOOT_UNSUPPORTED)
	    {
	      errnum = ERR_BOOT_FEATURES;
	      return 0;
	    }
	  type = 'm';
	  str2 = "Multiboot";
	  break;
	}
    }

  /* ELF loading only supported if kernel using multiboot */
  if (type == 'm' && len > sizeof (Elf32_Ehdr)
      && BOOTABLE_I386_ELF ((*((Elf32_Ehdr *) buffer))))
    {
      entry_addr = (entry_func) pu.elf->e_entry;

      if (((int) entry_addr) < 0x100000)
	errnum = ERR_BELOW_1MB;

      /* don't want to deal with ELF program header at some random
         place in the file -- this generally won't happen */
      if (pu.elf->e_phoff == 0 || pu.elf->e_phnum == 0
	  || ((pu.elf->e_phoff + (pu.elf->e_phentsize * pu.elf->e_phnum))
	      >= len))
	errnum = ERR_EXEC_FORMAT;
      str = "elf";
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

      if (!type)
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
	  if (buffer[0] == 0xb && buffer[1] == 1)
	    {
	      type = 'f';
	      entry_addr = (entry_func) (((int) entry_addr) & 0xFFFFFF);
	      str2 = "FreeBSD";
	    }
	  else
	    {
	      type = 'n';
	      entry_addr = (entry_func) (((int) entry_addr) & 0xF00000);
	      if (N_GETMAGIC ((*(pu.aout))) != NMAGIC)
		align_4k = 0;
	      str2 = "NetBSD";
	    }
	}

      cur_addr = (int) entry_addr;
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
      if (!big_linux && text_len > LINUX_KERNEL_MAXLEN)
	{
	  printf (" linux 'zImage' kernel too big, try 'make bzImage'\n");
	  errnum = ERR_WONT_FIT;
	  return 0;
	}

      printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
	      (big_linux ? "bzImage" : "zImage"), data_len, text_len);

      if (mbi.mem_lower >= 608)
	{
	  bcopy (buffer, (char *) LINUX_SETUP, data_len + SECTOR_SIZE);

	  /* copy command-line plus memory hack to staging area */
	  {
	    char *src = cur_cmdline;
	    char *dest = (char *) (CL_MY_LOCATION + 4);

	    bcopy ("mem=", (char *) CL_MY_LOCATION, 4);

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
	  if (grub_read ((char *) LINUX_STAGING_AREA, text_len) >= (text_len - 16))
	    return (big_linux ? 'L' : 'l');
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
    return 0;

  /* fill the multiboot info structure */
  mbi.cmdline = (int) cur_cmdline;
  mbi.mods_count = 0;
  mbi.mods_addr = 0;
  mbi.boot_device = (saved_drive << 24) | saved_partition;
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
      if (grub_read ((char *) cur_addr, text_len) == text_len)
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

	      if (grub_read ((char *) cur_addr, data_len) != data_len &&
		  !errnum)
		errnum = ERR_EXEC_FORMAT;
	      cur_addr += data_len;
	    }

	  if (!errnum)
	    {
	      bzero ((char *) cur_addr, bss_len);
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
	  cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

	  mbi.syms.a.addr = cur_addr;

	  *(((int *) cur_addr)++) = pu.aout->a_syms;

	  printf (", symtab=0x%x", pu.aout->a_syms);

	  if (grub_read ((char *) cur_addr, pu.aout->a_syms) == pu.aout->a_syms)
	    {
	      cur_addr += pu.aout->a_syms;
	      mbi.syms.a.tabsize = pu.aout->a_syms;

	      if (grub_read ((char *) &i, sizeof (int)) == sizeof (int))
		{
		  *(((int *) cur_addr)++) = i;

		  mbi.syms.a.strsize = i;

		  i -= sizeof (int);

		  printf (", strtab=0x%x", i);

		  symtab_err = (grub_read ((char *) cur_addr, i) != i);
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
	      memaddr = RAW_ADDR (phdr->p_vaddr);
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
	      memaddr = RAW_ADDR (memaddr);
	      if (memcheck (memaddr, memsiz)
		  && grub_read ((char *) memaddr, filesiz) == filesiz)
		{
		  if (memsiz > filesiz)
		    bzero ((char *) (memaddr + filesiz), memsiz - filesiz);
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
      type = 0;
    }

  return type;
}

int
load_module (void)
{
  int len;

  /* if we are supposed to load on 4K boundaries */
  cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

  if (!grub_open (cur_cmdline) || !(len = grub_read ((char *) cur_addr, -1)))
    return 0;

  printf ("   [Multiboot-module @ 0x%x, 0x%x bytes]\n", cur_addr, len);

  /* these two simply need to be set if any modules are loaded at all */
  mbi.flags |= MB_INFO_MODS;
  mbi.mods_addr = (int) mll;

  mll[mbi.mods_count].cmdline = (int) cur_cmdline;
  mll[mbi.mods_count].mod_start = cur_addr;
  cur_addr += len;
  mll[mbi.mods_count].mod_end = cur_addr;
  mll[mbi.mods_count].pad = 0;

  /* increment number of modules included */
  mbi.mods_count++;

  return 1;
}

int
load_initrd (void)
{
  int len;
  long *ramdisk, moveto;

  if (!grub_open (cur_cmdline) || !(len = grub_read ((char *) cur_addr, -1)))
    return 0;

  moveto = ((mbi.mem_upper + 0x400) * 0x400 - len) & 0xfffff000;
  bcopy ((void *) cur_addr, (void *) moveto, len);

  printf ("   [Linux-initrd @ 0x%x, 0x%x bytes]\n", moveto, len);

  ramdisk = (long *) (LINUX_SETUP + LINUX_SETUP_INITRD);
  ramdisk[0] = moveto;
  ramdisk[1] = len;

  return 1;
}


/*
 *  All "*_boot" commands depend on the images being loaded into memory
 *  correctly, the variables in this file being set up correctly, and
 *  the root partition being set in the 'saved_drive' and 'saved_partition'
 *  variables.
 */


void
bsd_boot (int type, int bootdev)
{
  char *str;
  int clval = 0, i;
  struct bootinfo bi;

  stop_floppy ();

  while (*(++cur_cmdline) && *cur_cmdline != ' ');
  str = cur_cmdline;
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

  if (type == 'f')
    {
      clval |= RB_BOOTINFO;

      bi.bi_version = BOOTINFO_VERSION;

      *cur_cmdline = 0;
      while ((--cur_cmdline) > (char *) (mbi.cmdline) && *cur_cmdline != '/');
      if (*cur_cmdline == '/')
	bi.bi_kernelname = cur_cmdline + 1;
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
      bi.bi_basemem = mbi.mem_lower;
      bi.bi_extmem = mbi.mem_upper;
      bi.bi_symtab = mbi.syms.a.addr;
      bi.bi_esymtab = mbi.syms.a.addr + 4
	+ mbi.syms.a.tabsize + mbi.syms.a.strsize;

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
