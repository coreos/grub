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

#ifndef GRUB_UTIL
# include "apic.h"
# include "smp-imps.h"
#endif

/*
 *  These are used for determining if the command-line should ask the user
 *  to correct errors during scripts.
 */
int fallback;
char *password;

/* True when the debug mode is turned on, and false when it is turned off.  */
int debug = 0;

/* Color settings */
int normal_color;
int highlight_color;

/* Find the next word from CMDLINE and return the pointer. If
   AFTER_EQUAL is non-zero, assume that the character `=' is treated as
   a space. Caution: this assumption is for backward compatibility.  */
char *
skip_to (int after_equal, char *cmdline)
{
  if (after_equal)
    {
      while (*cmdline && *cmdline != ' ' && *cmdline != '=')
	cmdline++;
      while (*cmdline == ' ' || *cmdline == '=')
	cmdline++;
    }
  else
    {
      while (*cmdline && *cmdline != ' ')
	cmdline++;
      while (*cmdline == ' ')
	cmdline++;
    }
  
  return cmdline;
}


void
init_cmdline (void)
{
  printf (" [ Minimal BASH-like line editing is supported.  For the first word, TAB
   lists possible command completions.  Anywhere else TAB lists the possible
   completions of a device/filename.  ESC at any time exits. ]\n");
}

char commands[] =
" Possible commands are: \"pause ...\", \"uppermem <kbytes>\", \"root <device>\",
  \"rootnoverify <device>\", \"chainloader <file>\", \"kernel <file> ...\",
  \"testload <file>\", \"read <addr>\", \"displaymem\", \"impsprobe\",
  \"geometry <drive>\", \"hide <device>\", \"unhide <device>\",
  \"fstest\", \"debug\", \"module <file> ...\", \"modulenounzip <file> ...\",
  \"color <normal> [<highlight>]\", \"makeactive\", \"boot\", \"quit\" and
  \"install <stage1_file> [d] <dest_dev> <file> <addr> [p] [<config_file>]\"\n";

static void
debug_fs_print_func (int sector)
{
  printf ("[%d]", sector);
}

static int installaddr, installlist, installsect;

static void
debug_fs_blocklist_func (int sector)
{
  if (debug)
    printf("[%d]", sector);

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


/* Run the command-line interface or execute a command from SCRIPT if
   SCRIPT is not NULL. Return CMDLINE_OK if successful, CMDLINE_ABORT
   if ``quit'' command is executed, and CMDLINE_ERROR if an error
   occures or ESC is pushed.  */
cmdline_t
enter_cmdline (char *script, char *heap)
{
  int bootdev, cmd_len, type = 0, run_cmdline = 1, have_run_cmdline = 0;
  char *cur_heap = heap, *cur_entry = script, *old_entry;
  char *mb_cmdline = (char *) MB_CMDLINE_BUF;
  
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
	      (void) getkey ();
returnit:
	      return CMDLINE_OK;
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
	  memmove (cur_heap, old_entry, ((int)cur_entry) - ((int)old_entry));

	  printf("%s\n", old_entry);
	}
    }
  else
    {
      cur_heap[0] = 0;
      print_error();
    }

  if (run_cmdline && get_cmdline (PACKAGE "> ", commands, cur_heap, 2048, 0))
    return CMDLINE_ERROR;

  if (substring ("boot", cur_heap) == 0 || (script && !*cur_heap))
    {
      if ((type == 'f') | (type == 'n'))
	bsd_boot (type, bootdev, (char *) MB_CMDLINE_BUF);
      if (type == 'l')
	linux_boot ();
      if (type == 'L')
	big_linux_boot ();

      if (type == 'c')
	{
	  gateA20 (0);
	  boot_drive = saved_drive;
	  chain_stage1 (0, BOOTSEC_LOCATION, BOOTSEC_LOCATION-16);
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
      if (ASCII_CHAR (getkey ()) == 27)
	return CMDLINE_ERROR;
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
  else if (substring ("kernel", cur_heap) < 1)
    {
      /* Reset MB_CMDLINE.  */
      mb_cmdline = (char *) MB_CMDLINE_BUF;
      if (cmd_len + 1 > MB_CMDLINE_BUFLEN)
	errnum = ERR_WONT_FIT;
      else
	{
	  /* Copy the command-line to MB_CMDLINE.  */
	  grub_memmove (mb_cmdline, cur_cmdline, cmd_len + 1);
	  if ((type = load_image (cur_cmdline, mb_cmdline)) != 0)
	    mb_cmdline += cmd_len + 1;
	}
    }
  else if (substring ("module", cur_heap) < 1)
    {
      if (type == 'm')
	{
#ifndef NO_DECOMPRESSION
	  /* this will respond to any "modulen<XXX>" command,
	     but that's OK */
	  if (cur_heap[6] == 'n')
	    no_decompression = 1;
#endif  /* NO_DECOMPRESSION */
	  
	  if (mb_cmdline + cmd_len + 1
	      > (char *) MB_CMDLINE_BUF + MB_CMDLINE_BUFLEN)
	    errnum = ERR_WONT_FIT;
	  else
	    {
	      /* Copy the command-line to MB_CMDLINE.  */
	      grub_memmove (mb_cmdline, cur_cmdline, cmd_len + 1);
	      if (load_module (cur_cmdline, mb_cmdline))
		mb_cmdline += cmd_len + 1;
	    }
	  
#ifndef NO_DECOMPRESSION
	  no_decompression = 0;
#endif  /* NO_DECOMPRESSION */
	}
      else if (type == 'L' || type == 'l')
	{
	  load_initrd (cur_cmdline);
	}
      else
	errnum = ERR_NEED_MB_KERNEL;
    }
  else if (substring("initrd", cur_heap) < 1)
    {
      if (type == 'L' || type == 'l')
	{
	  load_initrd (cur_cmdline);
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
	  memmove (buffer+BOOTSEC_BPB_OFFSET, old_sect+BOOTSEC_BPB_OFFSET,
		   BOOTSEC_BPB_LENGTH);

	  /* if for a hard disk, copy possible MBR/extended part table */
	  if ((dest_drive & 0x80) && current_partition == 0xFFFFFF)
	    memmove (buffer+BOOTSEC_PART_OFFSET, old_sect+BOOTSEC_PART_OFFSET,
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
	      /* If STAGE1_FILE is the LBA version, do a sanity check.  */
	      if (buffer[STAGE1_ID_OFFSET] == STAGE1_ID_LBA)
		{
		  /* The geometry of the drive in which FILE is located.  */
		  struct geometry load_geom;

		  /* Check if CURRENT_DRIVE is a floppy disk.  */
		  if (! (current_drive & 0x80))
		    {
		      errnum = ERR_DEV_VALUES;
		      goto install_failure;
		    }
		  
		  /* Get the geometry of CURRENT_DRIVE.  */
		  if (get_diskinfo (current_drive, &load_geom))
		    {
		      errnum = ERR_NO_DISK;
		      goto install_failure;
		    }

#ifdef GRUB_UTIL
		  /* XXX Can we determine if LBA is supported in
		     /sbin/grub as well?  */
		  grub_printf ("Warning: make sure that the access mode for"
			       "(hd%d) is LBA.\n", current_drive - 0x80);
#else
		  /* Check if LBA is supported.  */
		  if (! (load_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION))
		    {
		      errnum = ERR_DEV_VALUES;
		      goto install_failure;
		    }
#endif
		}
	      
	      if (!new_drive)
		new_drive = current_drive;

	      memmove ((char*)BOOTSEC_LOCATION, buffer, SECTOR_SIZE);

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
		  if (*((short *)(SCRATCHADDR+STAGE2_VER_MAJ_OFFS))
		      != COMPAT_VERSION)
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

		      grub_read ((char *) RAW_ADDR (0x100000), -1);

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
	  
    install_failure:
      
      /* Error running the install script, so drop to command line. */
      if (script)
	{
	  fallback = -1;
	  return CMDLINE_ERROR;
	}
    }
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

	  grub_read ((char *) RAW_ADDR (0x100000), -1);

	  /* now compare two sections of the file read differently */

	  for (i = 0; i < 0x10ac0; i++)
	    {
	      *((unsigned char *) RAW_ADDR (0x200000+i)) = 0;
	      *((unsigned char *) RAW_ADDR (0x300000+i)) = 1;
	    }

	  /* first partial read */
	  printf("\nPartial read 1: ");

	  filepos = 0;
	  grub_read ((char *) RAW_ADDR (0x200000), 0x7);
	  grub_read ((char *) RAW_ADDR (0x200007), 0x100);
	  grub_read ((char *) RAW_ADDR (0x200107), 0x10);
	  grub_read ((char *) RAW_ADDR (0x200117), 0x999);
	  grub_read ((char *) RAW_ADDR (0x200ab0), 0x10);
	  grub_read ((char *) RAW_ADDR (0x200ac0), 0x10000);

	  /* second partial read */
	  printf("\nPartial read 2: ");

	  filepos = 0;
	  grub_read ((char *) RAW_ADDR (0x300000), 0x10000);
	  grub_read ((char *) RAW_ADDR (0x310000), 0x10);
	  grub_read ((char *) RAW_ADDR (0x310010), 0x7);
	  grub_read ((char *) RAW_ADDR (0x310017), 0x10);
	  grub_read ((char *) RAW_ADDR (0x310027), 0x999);
	  grub_read ((char *) RAW_ADDR (0x3109c0), 0x100);

	  printf ("\nHeader1 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
		  *((int *) RAW_ADDR (0x200000)),
		  *((int *) RAW_ADDR (0x200004)),
		  *((int *) RAW_ADDR (0x200008)),
		  *((int *) RAW_ADDR (0x20000c)));

	  printf ("Header2 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
		  *((int *) RAW_ADDR (0x300000)),
		  *((int *) RAW_ADDR (0x300004)),
		  *((int *) RAW_ADDR (0x300008)),
		  *((int *) RAW_ADDR (0x30000c)));
	  
	  for (i = 0; i < 0x10ac0 && *((unsigned char *) RAW_ADDR (0x200000+i))
		 == *((unsigned char *) RAW_ADDR (0x300000+i)); i++);

	  printf("Max is 0x10ac0: i=0x%x, filepos=0x%x\n", i, filepos);

	  debug_fs = NULL;
	}
    }
  else if (substring ("read", cur_heap) < 1)
    {
      int myaddr;
      if (safe_parse_maxint (&cur_cmdline, &myaddr))
	printf ("Address 0x%x: Value 0x%x",
		myaddr, *((unsigned *) RAW_ADDR (myaddr)));
    }
  else if (substring ("fstest", cur_heap) == 0)
    {
      if (debug_fs)
	{
	  debug_fs = NULL;
	  printf (" Filesystem tracing is now off\n");
	}
      else
	{
	  debug_fs = debug_fs_print_func;
	  printf (" Filesystem tracing is now on\n");
	}
    }
  else if (substring ("impsprobe", cur_heap) == 0)
    {
#ifndef GRUB_UTIL
      if (!imps_probe ())
#endif
	printf (" No MPS information found or probe failed\n");
    }
  else if (substring ("displaymem", cur_heap) == 0)
    {
      if (get_eisamemsize () != -1)
	printf (" EISA Memory BIOS Interface is present\n");
      if (get_mmap_entry ((void *) SCRATCHADDR, 0) != 0 ||
	  *((int *) SCRATCHADDR) != 0)
	printf (" Address Map BIOS Interface is present\n");

      printf (" Lower memory: %uK, Upper memory (to first chipset hole): %uK\n",
	     mbi.mem_lower, mbi.mem_upper);

      if (mbi.flags & MB_INFO_MEM_MAP)
	{
	  struct AddrRangeDesc *map = (struct AddrRangeDesc *) mbi.mmap_addr;
	  int end_addr = mbi.mmap_addr + mbi.mmap_length;

	  printf (" [Address Range Descriptor entries immediately follow (values are 64-bit)]\n");
	  while (end_addr > (int) map)
	    {
	      char *str;

	      if (map->Type == MB_ARD_MEMORY)
		str = "Usable RAM";
	      else
		str = "Reserved";
	      printf ("   %s:  Base Address:  0x%x X 4GB + 0x%x,
      Length:   %u X 4GB + %u bytes\n",
		     str, map->BaseAddrHigh, map->BaseAddrLow,
		     map->LengthHigh, map->LengthLow);

	      map = ((struct AddrRangeDesc *) (((int) map) + 4 + map->size));
	    }
	}
    }
  else if (substring ("makeactive", cur_heap) == 0)
    make_saved_active();
  else if (substring ("debug", cur_heap) == 0)
    {
      if (debug)
	{
	  debug = 0;
	  grub_printf (" Debug mode is turned off\n");
	}
      else
	{
	  debug = 1;
	  grub_printf (" Debug mode is turned on\n");
	}
    }
  else if (substring ("color", cur_heap) < 1)
    {
      char *normal;
      char *highlight;

      normal = cur_cmdline;
      highlight = skip_to (0, normal);
      
      if (safe_parse_maxint (&normal, &normal_color))
	{
	  /* The second argument is optional, so set highlight_color
	     to inverted NORMAL_COLOR.  */
	  if (*highlight == 0
	      || ! safe_parse_maxint (&highlight, &highlight_color))
	    highlight_color = ((normal_color >> 4)
			       | ((normal_color & 0xf) << 4));
	}
    }
  else if (substring ("quit", cur_heap) < 1)
    {
#ifdef GRUB_UTIL
      return CMDLINE_ABORT;
#else
      grub_printf (" The quit command is ignored in Stage2\n");
#endif
    }
  else if (substring ("geometry", cur_heap) < 1)
    {
      set_device (cur_cmdline);
      
      if (! errnum)
	{
	  struct geometry geom;
	  
	  if (get_diskinfo (current_drive, &geom))
	    errnum = ERR_NO_DISK;
	  else
	    {
	      char *msg;

#ifdef GRUB_UTIL
	      msg = device_map[current_drive];
#else
	      if (geom.flags & BIOSDISK_FLAG_LBA_EXTENSION)
		msg = "LBA";
	      else
		msg = "CHS";
#endif
	      
	      grub_printf ("drive 0x%x: C/H/S = %d/%d/%d, "
			   "The number of sectors = %d, %s\n",
			   current_drive,
			   geom.cylinders, geom.heads, geom.sectors,
			   geom.total_sectors, msg);
	    }
	}
    }
  else if (substring ("hide", cur_heap) < 1)
    {
      set_device (cur_cmdline);
      
      if (! errnum)
	{
	  saved_partition = current_partition;
	  saved_drive = current_drive;
	  hide_partition ();
        }
    }
  else if (substring ("unhide", cur_heap) < 1)
    {
      set_device (cur_cmdline);
      
      if (! errnum)
	{
	  saved_partition = current_partition;
	  saved_drive = current_drive;
	  unhide_partition ();
        }
    }
  else if (*cur_heap && *cur_heap != ' ')
    errnum = ERR_UNRECOGNIZED;
  
  goto restart;
}
