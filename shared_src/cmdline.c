/* cmdline.c - the device-independent GRUB text command line */
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

#ifdef DEBUG

/*
 *  This is the Intel MultiProcessor Spec debugging/display code.
 */

#define IMPS_DEBUG
#define KERNEL_PRINT(x)         printf x
#define CMOS_WRITE_BYTE(x, y)	cmos_write_byte(x, y)
#define CMOS_READ_BYTE(x)	cmos_read_byte(x)
#define PHYS_TO_VIRTUAL(x)	(x)
#define VIRTUAL_TO_PHYS(x)	(x)

static inline unsigned char
inb (unsigned short port)
{
  unsigned char data;

  __asm __volatile ("inb %1,%0":"=a" (data):"d" (port));
  return data;
}

static inline void
outb (unsigned short port, unsigned char val)
{
  __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}


__inline__ static void
cmos_write_byte(int loc, int val)
{
	outb(0x70, loc);
	outb(0x71, val);
}

__inline__ static unsigned
cmos_read_byte(int loc)
{
	outb(0x70, loc);
	return inb(0x71);
}

#include "smp-imps.c"

#endif /* DEBUG */

/*
 *  These are used for determining if the command-line should ask the user
 *  to correct errors during scripts.
 */
int fallback;
char *password;

char *
skip_to(int after_equal, char *cmdline)
{
  while (*cmdline && (*cmdline != (after_equal ? '=' : ' ')))
    cmdline++;

  if (after_equal)
    cmdline++;

  while (*cmdline == ' ')
    cmdline++;

  return cmdline;
}


void
init_cmdline(void)
{
  printf(" [ Minimal BASH-like line editing is supported.  For the first word, TAB
   lists possible command completions.  Anywhere else TAB lists the possible
   completions of a device/filename.  ESC at any time exits. ]\n");
}


#ifdef DEBUG
char commands[] =
 " Possible commands are: \"pause= ...\", \"uppermem= <kbytes>\", \"root= <device>\",
  \"rootnoverify= <device>\", \"chainloader= <file>\", \"kernel= <file> ...\",
  \"testload= <file>\", \"read= <addr>\", \"displaymem\", \"impsprobe\", \"fstest\",
  \"module= <file> ...\", \"modulenounzip= <file> ...\", \"makeactive\", \"boot\", and
  \"install= <stage1_file> [d] <dest_dev> <file> <addr> [p] [<config_file>]\"\n";
#else  /* DEBUG */
char commands[] =
 " Possible commands are: \"pause= ...\", \"uppermem= <kbytes>\", \"root= <device>\",
  \"rootnoverify= <device>\", \"chainloader= <file>\", \"kernel= <file> ...\",
  \"module= <file> ...\", \"modulenounzip= <file> ...\", \"makeactive\", \"boot\", and
  \"install= <stage1_file> [d] <dest_dev> <file> <addr> [p] [<config_file>]\"\n";
#endif /* DEBUG */

#ifdef DEBUG
static void
debug_fs_print_func(int sector)
{
  printf("[%d]", sector);
}
#endif /* DEBUG */


static int installaddr, installlist, installsect;

static void
debug_fs_blocklist_func(int sector)
{
#ifdef DEBUG
  printf("[%d]", sector);
#endif /* DEBUG */

  if (*((unsigned long *)(installlist-4))
      + *((unsigned short *)installlist) != sector
      || installlist == BOOTSEC_LOCATION+STAGE1_FIRSTLIST+4)
    {
      installlist -= 8;

      if (*((unsigned long *)(installlist-8)))
	errnum = ERR_WONT_FIT;
      else
	{
	  *((unsigned short *)(installlist+2)) = (installaddr >> 4);
	  *((unsigned long *)(installlist-4)) = sector;
	}
    }

  *((unsigned short *)installlist) += 1;
  installsect = sector;
  installaddr += 512;
}


int
enter_cmdline (char *script, char *heap)
{
  int bootdev, cmd_len, type = 0, run_cmdline = 1, have_run_cmdline = 0;
  char *cur_heap = heap, *cur_entry = script, *old_entry;

  /* initialization */
  saved_drive = boot_drive;
  saved_partition = install_partition;
  current_drive = 0xFF;
  errnum = 0;

  /* restore memory probe state */
  mbi.mem_upper = saved_mem_upper;
  if (mbi.mmap_length)
    mbi.flags |= MB_INFO_MEM_MAP;

  /* BSD and chainloading evil hacks !! */
  bootdev = set_bootdev(0);

  if (!script)
    {
      init_page();
      init_cmdline();
    }

restart:
  if (script)
    {
      if (errnum)
	{
	  if (fallback < 0)
	    goto returnit;
	  print_error();
	  if (password || errnum == ERR_BOOT_COMMAND)
	    {
	      printf("Press any key to continue...");
	      (void) getc ();
returnit:
	      return 0;
	    }

	  run_cmdline = 1;
	  if (!have_run_cmdline)
	    {
	      have_run_cmdline = 1;
	      putchar('\n');
	      init_cmdline();
	    }
	}
      else
	{
	  run_cmdline = 0;

	  /* update position in the boot script */
	  old_entry = cur_entry;
	  while (*(cur_entry++));

	  /* copy to work area */
	  bcopy(old_entry, cur_heap, ((int)cur_entry) - ((int)old_entry));

	  printf("%s\n", old_entry);
	}
    }
  else
    {
      cur_heap[0] = 0;
      print_error();
    }

  if (run_cmdline && get_cmdline(PACKAGE "> ", commands, cur_heap, 2048))
    return 1;

  if (substring("boot", cur_heap) == 0 || (script && !*cur_heap))
    {
      if ((type == 'f') | (type == 'n'))
	bsd_boot(type, bootdev);
      if (type == 'l')
	linux_boot();
      if (type == 'L')
	big_linux_boot();

      if (type == 'c')
	{
	  gateA20(0);
	  boot_drive = saved_drive;
	  chain_stage1(0, BOOTSEC_LOCATION, BOOTSEC_LOCATION-16);
	}

      if (!type)
	{
	  errnum = ERR_BOOT_COMMAND;
	  goto restart;
	}

      /* this is the final possibility */
      multi_boot((int)entry_addr, (int)(&mbi));
    }

  /* get clipped command-line */
  cur_cmdline = skip_to(1, cur_heap);
  cmd_len = 0;
  while (cur_cmdline[cmd_len++]);

  if (substring("chainloader", cur_heap) < 1)
    {
      if (grub_open (cur_cmdline) &&
	  (grub_read ((char *) BOOTSEC_LOCATION, SECTOR_SIZE) == SECTOR_SIZE) &&
	  (*((unsigned short *) (BOOTSEC_LOCATION+BOOTSEC_SIG_OFFSET))
	      == BOOTSEC_SIGNATURE))
	type = 'c';
      else if (!errnum)
	{
	  errnum = ERR_EXEC_FORMAT;
	  type = 0;
	}
    }
  else if (substring("pause", cur_heap) < 1)
    {
      if (getc() == 27)
	return 1;
    }
  else if (substring("uppermem", cur_heap) < 1)
    {
      if (safe_parse_maxint(&cur_cmdline, (int *)&(mbi.mem_upper)))
	mbi.flags &= ~MB_INFO_MEM_MAP;
    }
  else if (substring("root", cur_heap) < 1)
    {
      int hdbias = 0;
      char *next_cmd = set_device (cur_cmdline);

      if (next_cmd)
	{
	  char *biasptr = skip_to (0, next_cmd);

	  /* this will respond to any "rootn<XXX>" command,
	     but that's OK */
	  if (!errnum && (cur_heap[4] == 'n' || open_device()
			  || errnum == ERR_FSYS_MOUNT))
	    {
	      errnum = 0;
	      saved_partition = current_partition;
	      saved_drive = current_drive;

	      if (cur_heap[4] != 'n')
		{
		  /* BSD and chainloading evil hacks !! */
		  safe_parse_maxint(&biasptr, &hdbias);
		  errnum = 0;
		  bootdev = set_bootdev(hdbias);

		  print_fsys_type();
		}
	      else
		current_drive = -1;
	    }
	}
    }
  else if (substring("kernel", cur_heap) < 1)
    {
      /* make sure it's at the beginning of the boot heap area */
      bcopy(cur_heap, heap, cmd_len + (((int)cur_cmdline) - ((int)cur_heap)));
      cur_cmdline = heap + (((int)cur_cmdline) - ((int)cur_heap));
      cur_heap = heap;
      if ((type = load_image()) != 0)
	cur_heap = cur_cmdline + cmd_len;
    }
  else if (substring("module", cur_heap) < 1)
    {
      if (type == 'm')
	{
#ifndef NO_DECOMPRESSION
	  /* this will respond to any "modulen<XXX>" command,
	     but that's OK */
	  if (cur_heap[6] == 'n')
	    no_decompression = 1;
#endif  /* NO_DECOMPRESSION */

	  if (load_module())
	    cur_heap = cur_cmdline + cmd_len;

#ifndef NO_DECOMPRESSION
	  no_decompression = 0;
#endif  /* NO_DECOMPRESSION */
	}
      else if (type == 'L' || type == 'l')
	{
	  load_initrd ();
	}
      else
	errnum = ERR_NEED_MB_KERNEL;
    }
  else if (substring("initrd", cur_heap) < 1)
    {
      if (type == 'L' || type == 'l')
	{
	  load_initrd ();
	}
      else
	errnum = ERR_NEED_LX_KERNEL;
    }
  else if (substring("install", cur_heap) < 1)
    {
      char *stage1_file = cur_cmdline, *dest_dev, *file, *addr;
      char buffer[SECTOR_SIZE], old_sect[SECTOR_SIZE];
      int new_drive = 0xFF;

      dest_dev = skip_to(0, stage1_file);
      if (*dest_dev == 'd')
	{
	  new_drive = 0;
	  dest_dev = skip_to(0, dest_dev);
	}
      file = skip_to (0, dest_dev);
      addr = skip_to (0, file);

      if (safe_parse_maxint (&addr, &installaddr) &&
	  grub_open (stage1_file) &&
	  grub_read (buffer, SECTOR_SIZE) == SECTOR_SIZE &&
	  set_device (dest_dev) && open_partition () &&
	  devread (0, 0, SECTOR_SIZE, old_sect))
	{
	  int dest_drive = current_drive;
	  struct geometry dest_geom = buf_geom;
	  int dest_sector = part_start, i;

#ifndef NO_DECOMPRESSION
	  no_decompression = 1;
#endif

	  /* copy possible DOS BPB, 59 bytes at byte offset 3 */
	  bcopy(old_sect+BOOTSEC_BPB_OFFSET, buffer+BOOTSEC_BPB_OFFSET,
		BOOTSEC_BPB_LENGTH);

	  /* if for a hard disk, copy possible MBR/extended part table */
	  if ((dest_drive & 0x80) && current_partition == 0xFFFFFF)
	    bcopy(old_sect+BOOTSEC_PART_OFFSET, buffer+BOOTSEC_PART_OFFSET,
		  BOOTSEC_PART_LENGTH);

	  if (*((short *)(buffer+STAGE1_VER_MAJ_OFFS)) != COMPAT_VERSION
	      || (*((unsigned short *) (buffer+BOOTSEC_SIG_OFFSET))
		  != BOOTSEC_SIGNATURE)
	      || (!(dest_drive & 0x80)
		  && (*((unsigned char *) (buffer+BOOTSEC_PART_OFFSET)) == 0x80
		      || buffer[BOOTSEC_PART_OFFSET] == 0)))
	    {
	      errnum = ERR_BAD_VERSION;
	    }
	  else if (grub_open (file))
	    {
	      if (!new_drive)
		new_drive = current_drive;

	      bcopy(buffer, (char*)BOOTSEC_LOCATION, SECTOR_SIZE);

	      *((unsigned char *)(BOOTSEC_LOCATION+STAGE1_FIRSTLIST))
		= new_drive;
	      *((unsigned short *)(BOOTSEC_LOCATION+STAGE1_INSTALLADDR))
		= installaddr;

	      i = BOOTSEC_LOCATION+STAGE1_FIRSTLIST-4;
	      while (*((unsigned long *)i))
		{
		  if (i < BOOTSEC_LOCATION+STAGE1_FIRSTLIST-256
		      || (*((int *)(i-4)) & 0x80000000)
		      || *((unsigned short *)i) >= 0xA00
		      || *((short *) (i+2)) == 0)
		    {
		      errnum = ERR_BAD_VERSION;
		      break;
		    }

		  *((int *)i) = 0;
		  *((int *)(i-4)) = 0;
		  i -= 8;
		}

	      installlist = BOOTSEC_LOCATION+STAGE1_FIRSTLIST+4;
	      debug_fs = debug_fs_blocklist_func;

	      if (!errnum &&
		  grub_read ((char *) SCRATCHADDR, SECTOR_SIZE) == SECTOR_SIZE)
		{
		  if (*((long *)SCRATCHADDR) != 0x8070ea
		      || (*((short *)(SCRATCHADDR+STAGE2_VER_MAJ_OFFS))
			  != COMPAT_VERSION))
		    errnum = ERR_BAD_VERSION;
		  else
		    {
		      int write_stage2_sect = 0, stage2_sect = installsect;
		      char *ptr;

		      ptr = skip_to(0, addr);

		      if (*ptr == 'p')
			{
			  write_stage2_sect++;
			  *((long *)(SCRATCHADDR+STAGE2_INSTALLPART))
			    = current_partition;
			  ptr = skip_to(0, ptr);
			}
		      if (*ptr)
			{
			  char *str
			    = ((char *) (SCRATCHADDR+STAGE2_VER_STR_OFFS));

			  write_stage2_sect++;
			  while (*(str++));      /* find string */
			  while ((*(str++) = *(ptr++)) != 0);    /* do copy */
			}

		      grub_read ((char *) 0x100000, -1);

		      buf_track = -1;

		      if (!errnum
			  && (biosdisk(BIOSDISK_WRITE,
				       dest_drive, &dest_geom,
				       dest_sector, 1, (BOOTSEC_LOCATION>>4))
			      || (write_stage2_sect
				  && biosdisk(BIOSDISK_WRITE,
					      current_drive, &buf_geom,
					      stage2_sect, 1, SCRATCHSEG))))
			  errnum = ERR_WRITE;
		    }
		}

	      debug_fs = NULL;
	    }

#ifndef NO_DECOMPRESSION
	  no_decompression = 0;
#endif
	}

      /* Error running the install script, so drop to command line. */
      if (script)
	{
	  fallback = -1;
	  return 1;
	}
    }
#ifdef DEBUG
  else if (substring("testload", cur_heap) < 1)
    {
      type = 0;
      if (grub_open (cur_cmdline))
	{
	  int i;

	  debug_fs = debug_fs_print_func;

	  /*
	   *  Perform filesystem test on the specified file.
	   */

	  /* read whole file first */
	  printf("Whole file: ");

	  grub_read ((char *) 0x100000, -1);

	  /* now compare two sections of the file read differently */

	  for (i = 0; i < 0x10ac0; i++)
	    {
	      *((unsigned char *)(0x200000+i)) = 0;
	      *((unsigned char *)(0x300000+i)) = 1;
	    }

	  /* first partial read */
	  printf("\nPartial read 1: ");

	  filepos = 0;
	  grub_read ((char *) 0x200000, 0x7);
	  grub_read ((char *) 0x200007, 0x100);
	  grub_read ((char *) 0x200107, 0x10);
	  grub_read ((char *) 0x200117, 0x999);
	  grub_read ((char *) 0x200ab0, 0x10);
	  grub_read ((char *) 0x200ac0, 0x10000);

	  /* second partial read */
	  printf("\nPartial read 2: ");

	  filepos = 0;
	  grub_read ((char *) 0x300000, 0x10000);
	  grub_read ((char *) 0x310000, 0x10);
	  grub_read ((char *) 0x310010, 0x7);
	  grub_read ((char *) 0x310017, 0x10);
	  grub_read ((char *) 0x310027, 0x999);
	  grub_read ((char *) 0x3109c0, 0x100);

	  printf("\nHeader1 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
		 *((int *)0x200000), *((int *)0x200004), *((int *)0x200008),
		 *((int *)0x20000c));

	  printf("Header2 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
		 *((int *)0x300000), *((int *)0x300004), *((int *)0x300008),
		 *((int *)0x30000c));

	  for (i = 0; i < 0x10ac0 && *((unsigned char *)(0x200000+i))
		 == *((unsigned char *)(0x300000+i)); i++);

	  printf("Max is 0x10ac0: i=0x%x, filepos=0x%x\n", i, filepos);

	  debug_fs = NULL;
	}
    }
  else if (substring("read", cur_heap) < 1)
    {
      int myaddr;
      if (safe_parse_maxint(&cur_cmdline, &myaddr))
	printf("Address 0x%x: Value 0x%x", myaddr, *((unsigned *)myaddr));
    }
  else if (substring("fstest", cur_heap) == 0)
    {
      if (debug_fs)
	{
	  debug_fs = NULL;
	  printf(" Filesystem tracing is now off\n");
	}
      else
	{
	  debug_fs = debug_fs_print_func;
	  printf(" Filesystem tracing is now on\n");
	}
    }
  else if (substring("impsprobe", cur_heap) == 0)
    {
      if (!imps_probe())
	printf(" No MPS information found or probe failed\n");
    }
  else if (substring("displaymem", cur_heap) == 0)
    {
      if (get_eisamemsize() != -1)
	printf(" EISA Memory BIOS Interface is present\n");
      if (get_mmap_entry ((void *) SCRATCHADDR, 0) != 0 ||
	  *((int *) SCRATCHADDR) != 0)
	printf(" Address Map BIOS Interface is present\n");

      printf(" Lower memory: %uK, Upper memory (to first chipset hole): %uK\n",
	     mbi.mem_lower, mbi.mem_upper);

      if (mbi.flags & MB_INFO_MEM_MAP)
	{
	  struct AddrRangeDesc *map = (struct AddrRangeDesc *) mbi.mmap_addr;
	  int end_addr = mbi.mmap_addr + mbi.mmap_length;

	  printf(" [Address Range Descriptor entries immediately follow (values are 64-bit)]\n");
	  while (end_addr > (int)map)
	    {
	      char *str;

	      if (map->Type == MB_ARD_MEMORY)
		str = "Usable RAM";
	      else
		str = "Reserved";
	      printf("   %s:  Base Address:  0x%x X 4GB + 0x%x,
      Length:   %u X 4GB + %u bytes\n",
		     str, map->BaseAddrHigh, map->BaseAddrLow,
		     map->LengthHigh, map->LengthLow);

	      map = ((struct AddrRangeDesc *) (((int)map) + 4 + map->size));
	    }
	}
    }
#endif /* DEBUG */
  else if (substring("makeactive", cur_heap) == 0)
    make_saved_active();
  else if (*cur_heap && *cur_heap != ' ')
    errnum = ERR_UNRECOGNIZED;

  goto restart;
}
