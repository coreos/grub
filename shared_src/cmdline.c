/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
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

#define _CMDLINE_C

#include "shared.h"

#ifdef DEBUG

unsigned long apic_addr;

int start_cpu(int cpu_num, int start_addr, unsigned long my_apic_addr);

extern char patch_code[];
extern char patch_code_end[];

unsigned long reg_table[] =
  { 0x20, 0x30, 0x80, 0x90, 0xa0, 0xd0, 0xe0, 0xf0, 0x280, 0x300, 0x310,
    0x320, 0x350, 0x360, 0x370, 0x380, 0x390, 0x3e0, 0 };


int
get_remote_APIC_reg(int cpu_num, int reg, unsigned long *retval)
{
  int i, j = 1000;

  i = *((volatile unsigned long *) (apic_addr+0xc0));
  *((volatile unsigned long *) (apic_addr+0x310)) = (cpu_num << 24);
  i = *((volatile unsigned long *) (apic_addr+0xc0));
  *((volatile unsigned long *) (apic_addr+0x300)) = (0x300 | (reg>>4));

  while (((i = (*((volatile unsigned long *) (apic_addr+0x300)) & 0x30000))
	  == 1) && (--j));

  if (!i || i == 3 || j == 0)
    return 1;

  *retval = *((volatile unsigned long *) (apic_addr+0xc0));

  return 0;
}


void
list_regs(int cpu_num)
{
  int i = 0, j = 0;
  unsigned long tmpval;

  while (reg_table[i] != 0)
    {
      printf(" %x: ", reg_table[i]);

      if (cpu_num == -1)
        tmpval = *((volatile unsigned long *) (apic_addr+reg_table[i]));
      else if (get_remote_APIC_reg(cpu_num, reg_table[i], &tmpval))
        printf("error!");

      printf("%x", tmpval);

      i++;
      j++;

      if (j == 5 || reg_table[i] == 0)
	{
	  j = 0;
          putchar('\n');
	}
      else
	putchar(',');
    }
}


void
boot_cpu(int cpu_num)
{
  int i, start_addr = (128 * 1024);

  bcopy( patch_code, ((char *) start_addr),
	 ((int) patch_code_end) - ((int) patch_code) );

  outb(0x70, 0xf);
  *((volatile unsigned short *) 0x467) = 0;
  *((volatile unsigned short *) 0x469) = ((unsigned short)(start_addr >> 4));
  outb(0x71, 0xa);

  print_error();

  printf("Starting probe for CPU #%d...  value = (%x)\n",
	 cpu_num, *((int *) start_addr) );

  i = start_cpu(cpu_num, start_addr, apic_addr);

  printf("Return value = (%x), waiting for RET...", i);

  i = getc();

  outb(0x70, 0xf);
  printf("\nEnding value = (%x), Status code = (%x)\n",
	 *((int *) start_addr), (unsigned long)inb(0x71));
}


unsigned char
sum(unsigned char *addr, int len)
{
  int i;
  unsigned long retval = 0;

  for (i = 0; i < len; i++)
    {
      retval += addr[len];
    }

  return ((unsigned char)(retval & 0xFF));
}


int
get_mp_parameters(unsigned char *addr)
{
  int i, may_be_bad;
  unsigned long backup;

  if (((int)addr)&0xF || addr[0] != '_' || addr[1] != 'M' || addr[2] != 'P'
      || addr[3] != '_')
    return 0;

  may_be_bad = 0;

  if (sum(addr, addr[8] * 16))
    {
      printf("Found MP structure but checksum bad, use (y/n) ?");
      i = getc();
      putchar('\n');
      if (i != 'y')
        return 0;
      may_be_bad = 1;
    }

  backup = apic_addr;

  printf("MP Floating Pointer Structure (address, then 12 bytes starting at 4):
        %x, %x, %x, %x\n", (int)addr,
	 *((int *)(addr+4)), *((int *)(addr+8)), *((int *)(addr+12)));

  if (*((int *)(addr+4)) != 0)
    {
      addr = *((unsigned char **)(addr+4));
      apic_addr = *((unsigned long *)(addr+0x24));
      printf("MP Configuration Table, local APIC at (%x)\n", apic_addr);
    }

  if (may_be_bad)
    {
      printf("Use this entry (y/n) ?");
      i = getc();
      putchar('\n');
      if (i != 'y')
	{
	  apic_addr = backup;
          return 0;
	}
    }

  return 1;
}


int
probe_mp_table(void)
{
  int i, probe_addr = *((unsigned short *)0x40E);

  probe_addr <<= 4;

  if (probe_addr > 0 && probe_addr <= 639*1024
      && *((unsigned char *) probe_addr) > 0
      && probe_addr + *((unsigned char *) probe_addr) * 0x400 <= 640*1024)
    {
      for (i = 0; i < 1024; i += 16)
	{
	  if (get_mp_parameters((unsigned char *)(probe_addr+i)))
	    return 1;
	}
    }

  /*
   *  Technically, if there is an EBDA, we shouldn't search the last
   *  KB of memory, but I don't think it will be a problem.
   */

  if (mbi.mem_lower > 512)
    probe_addr = 639 * 1024;
  else
    probe_addr = 511 * 1024;

  for (i = 0; i < 1024; i += 16)
    {
      if (get_mp_parameters((unsigned char *)(probe_addr+i)))
	return 1;
    }

  for (probe_addr = 0xF0000; probe_addr < 0x100000; probe_addr += 16)
    {
      if (get_mp_parameters((unsigned char *)probe_addr))
	return 1;
    }

  return 0;
}


void
copy_patch_code(void)
{
  int start_addr = (128 * 1024);

  bcopy( patch_code, ((char *) start_addr),
	 ((int) patch_code_end) - ((int) patch_code) );

  outb(0x70, 0xf);
  *((volatile unsigned short *) 0x467) = 0;
  *((volatile unsigned short *) 0x469) = ((unsigned short)(start_addr >> 4));
  outb(0x71, 0xa);

  print_error();
}


void
send_init(int cpu_num)
{
  int i, start_addr = (128 * 1024);

  *((volatile unsigned long *) (apic_addr+0x280)) = 0;
  i = *((volatile unsigned long *) (apic_addr+0x280));

  *((volatile unsigned long *) (apic_addr+0x310)) = (cpu_num << 24);
  i = *((volatile unsigned long *) (apic_addr+0x280));
  *((volatile unsigned long *) (apic_addr+0x300)) = 0xc500;

  for (i = 0; i < 100000; i++);

  *((volatile unsigned long *) (apic_addr+0x300)) = 0x8500;

  for (i = 0; i < 1000000; i++);

  i = *((volatile unsigned long *) (apic_addr+0x280));

  i &= 0xEF;

  if (i)
    printf("APIC error (%x)\n", i);
}


void
send_startup(int cpu_num)
{
  int i, start_addr = (128 * 1024);

  printf("Starting value = (%x)\n", *((int *) start_addr) );

  *((volatile unsigned long *) (apic_addr+0x280)) = 0;
  i = *((volatile unsigned long *) (apic_addr+0x280));

  *((volatile unsigned long *) (apic_addr+0x310)) = (cpu_num << 24);
  i = *((volatile unsigned long *) (apic_addr+0x280));
  *((volatile unsigned long *) (apic_addr+0x300)) = (0x600 | (start_addr>>12));

  for (i = 0; i < 100000; i++);

  i = *((volatile unsigned long *) (apic_addr+0x280));

  i &= 0xEF;

  if (i)
    printf("APIC error (%x)\n", i);
  else
    {
      printf("Waiting for RET...");

      i = getc();

      outb(0x70, 0xf);
      printf("\nEnding value = (%x), Status code = (%x)\n",
             *((int *) start_addr), (unsigned long)inb(0x71));
    }
}

#endif /* DEBUG */

/*
 *  This is used for determining of the command-line should ask the user
 *  to correct errors.
 */
int fallback = -1;

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
  \"testload= <file>\", \"syscmd= <cmd>\", \"displaymem\", \"probemps\",
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
enter_cmdline(char *script, char *heap)
{
  int bootdev, cmd_len, type = 0, run_cmdline = 1, have_run_cmdline = 0;
#ifdef DEBUG
  int mptest = 0;
#endif /* DEBUG */
  char *cur_heap = heap, *cur_entry = script, *old_entry;

  /* initialization */
  saved_drive = boot_drive;
  saved_partition = install_partition;
  current_drive = 0xFF;
  errnum = 0;

  /* restore memory probe state */
  mbi.mem_upper = saved_mem_upper;
  if (mem_map)
    mbi.flags |= MB_INFO_MEM_MAP;

  /* XXX evil hack !! */
  bootdev = bsd_bootdev();

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
	  if (fallback != -1)
	    return 0;

	  print_error();
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

  if (run_cmdline && get_cmdline("command> ", commands, cur_heap, 2048))
    return 1;

  if (strcmp("boot", cur_heap) == 0 || (script && !*cur_heap))
    {
      if ((type == 'f') | (type == 'n'))
	bsd_boot(type, bootdev);
      if (type == 'l')
	linux_boot();

      if (type == 'c')
	{
	  gateA20(0);
	  boot_drive = saved_drive;
	  chain_stage1(0, BOOTSEC_LOCATION, BOOTSEC_LOCATION-16);
	}

      if (!type)
	{
	  printf(" Error, cannot boot unless kernel loaded.\n");

	  if (fallback != -1)
	    return 0;

	  if (script)
	    {
	      printf("Press any key to continue...");
	      getc();
	      return 1;
	    }
	  else
	    goto restart;
	}

      /* this is the final possibility */
      multi_boot((int)entry_addr, (int)(&mbi));
    }

  /* get clipped command-line */
  cur_cmdline = skip_to(1, cur_heap);
  cmd_len = 0;
  while (cur_cmdline[cmd_len++]);

  if (strcmp("chainloader", cur_heap) < 1)
    {
      if (open(cur_cmdline) && (read(BOOTSEC_LOCATION, SECTOR_SIZE) 
				== SECTOR_SIZE)
	  && (*((unsigned short *) (BOOTSEC_LOCATION+BOOTSEC_SIG_OFFSET))
	      == BOOTSEC_SIGNATURE))
	type = 'c';
      else if (!errnum)
	{
	  errnum = ERR_EXEC_FORMAT;
	  type = 0;
	}
    }
  else if (strcmp("pause", cur_heap) < 1)
    {
      if (getc() == 27)
	return 1;
    }
  else if (strcmp("uppermem", cur_heap) < 1)
    {
      if (safe_parse_maxint(&cur_cmdline, (int *)&(mbi.mem_upper)))
	mbi.flags &= ~MB_INFO_MEM_MAP;
    }
  else if (strcmp("root", cur_heap) < 1)
    {
      set_device(cur_cmdline);

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
	      /* XXX evil hack !! */
	      bootdev = bsd_bootdev();

	      print_fsys_type();
	    }
	  else
	    current_drive = -1;
	}
    }
  else if (strcmp("kernel", cur_heap) < 1)
    {
      /* make sure it's at the beginning of the boot heap area */
      bcopy(cur_heap, heap, cmd_len + (((int)cur_cmdline) - ((int)cur_heap)));
      cur_cmdline = heap + (((int)cur_cmdline) - ((int)cur_heap));
      cur_heap = heap;
      if (type = load_image())
	cur_heap = cur_cmdline + cmd_len;
    }
  else if (strcmp("module", cur_heap) < 1)
    {
      if (type == 'm')
	{
#ifndef NO_DECOMPRESSION
	  /* this will respond to any "modulen<XXX>" command,
	     but that's OK */
	  if (cur_heap[6] = 'n')
	    no_decompression = 1;
#endif  /* NO_DECOMPRESSION */

	  if (load_module())
	    cur_heap = cur_cmdline + cmd_len;

#ifndef NO_DECOMPRESSION
	  no_decompression = 0;
#endif  /* NO_DECOMPRESSION */
	}
      else
	errnum = ERR_NEED_KERNEL;
    }
  else if (strcmp("install", cur_heap) < 1)
    {
      char *stage1_file = cur_cmdline, *dest_dev, *file, *addr, *config_file;
      char buffer[SECTOR_SIZE], old_sect[SECTOR_SIZE];
      int i = BOOTSEC_LOCATION+STAGE1_FIRSTLIST-4, new_drive = 0xFF;

      dest_dev = skip_to(0, stage1_file);
      if (*dest_dev == 'd')
	{
	  new_drive = 0;
	  dest_dev = skip_to(0, dest_dev);
	}
      file = skip_to(0, dest_dev);
      addr = skip_to(0, file);

      if (safe_parse_maxint(&addr, &installaddr) && open(stage1_file)
	  && read((int)buffer, SECTOR_SIZE) == SECTOR_SIZE
	  && set_device(dest_dev) && open_partition()
	  && devread(0, 0, SECTOR_SIZE, (int)old_sect))
	{
	  int dest_drive = current_drive, dest_geom = buf_geom;
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
	  else if (open(file))
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

	      if (!errnum && read(SCRATCHADDR, SECTOR_SIZE) == SECTOR_SIZE)
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
			  while (*(str++) = *(ptr++));    /* do copy */
			}

		      read(0x100000, -1);

		      buf_track = -1;

		      if (!errnum
			  && (biosdisk(BIOSDISK_SUBFUNC_WRITE,
				       dest_drive, dest_geom,
				       dest_sector, 1, (BOOTSEC_LOCATION>>4))
			      || (write_stage2_sect
				  && biosdisk(BIOSDISK_SUBFUNC_WRITE,
					      current_drive, buf_geom,
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
    }
#ifdef DEBUG
  else if (strcmp("testload", cur_heap) < 1)
    {
      if (open(cur_cmdline))
	{
	  int i;

	  debug_fs = debug_fs_print_func;

	  /*
	   *  Perform filesystem test on the specified file.
	   */

	  /* read whole file first */
	  printf("Whole file: ");

	  read(0x100000, -1);

	  /* now compare two sections of the file read differently */

	  for (i = 0; i < 0x10ac0; i++)
	    {
	      *((unsigned char *)(0x200000+i)) = 0;
	      *((unsigned char *)(0x300000+i)) = 1;
	    }

	  /* first partial read */
	  printf("\nPartial read 1: ");

	  filepos = 0;
	  read(0x200000, 0x7);
	  read(0x200007, 0x100);
	  read(0x200107, 0x10);
	  read(0x200117, 0x999);
	  read(0x200ab0, 0x10);
	  read(0x200ac0, 0x10000);

	  /* second partial read */
	  printf("\nPartial read 2: ");

	  filepos = 0;
	  read(0x300000, 0x10000);
	  read(0x310000, 0x10);
	  read(0x310010, 0x7);
	  read(0x310017, 0x10);
	  read(0x310027, 0x999);
	  read(0x3109c0, 0x100);

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
  else if (strcmp("syscmd", cur_heap) < 1)
    {
      switch(cur_cmdline[0])
	{
	case 'F':
	  if (debug_fs)
	    {
	      debug_fs = NULL;
	      printf(" Filesystem tracing is now off\n");
	    }
	  else
	    {
	      debug_fs = debug_fs_print_func;
	      printf(" Filesystem tracing if now on\n");
	    }
	  break;
	case 'R':
	  {
	    char *ptr = cur_cmdline+1;
	    int myaddr;
	    if (safe_parse_maxint(&ptr, &myaddr))
	      printf("0x%x: 0x%x", myaddr, *((unsigned *)myaddr));
	  }
	  break;
	case 'r':
	  if (mptest)
	    {
	      list_regs(-1);
	      break;
	    }
	case 'p':
	  if (mptest)
	    {
	      copy_patch_code();
	      break;
	    }
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  if (mptest)
	    {
	      int j = cur_cmdline[0] - '0';
	      switch (cur_cmdline[1])
		{
		case 'i':
		  send_init(j);
		  break;
		case 's':
		  send_startup(j);
		  break;
		case 'r':
		  list_regs(j);
		  break;
		case 'b':
		  boot_cpu(j);
		}
	      break;
	    }
	default:
	  printf("Bad subcommand, try again please\n");
	}
    }
  else if (strcmp("probemps", cur_heap) == 0)
    {
      apic_addr = 0xFEE00000;

      if (mptest = probe_mp_table())
	printf("APIC test (%x), SPIV test(%x)\n",
	       *((volatile unsigned long *) (apic_addr+0x30)),
	       *((volatile unsigned long *) (apic_addr+0xf0)));
      else
	printf("No MPS information found\n");
    }
  else if (strcmp("displaymem", cur_heap) == 0)
    {
      if (get_eisamemsize() != -1)
	printf(" EISA Memory BIOS Interface is present\n");
      if (get_mem_map(SCRATCHADDR, 0) != 0 || *((int *) SCRATCHADDR) != 0)
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
  else if (strcmp("makeactive", cur_heap) == 0)
    make_saved_active();
  else if (*cur_heap && *cur_heap != ' ')
    errnum = ERR_UNRECOGNIZED;

  goto restart;
}

