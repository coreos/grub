/* asmstub.c - a version of shared_src/asm.S that works under Unix */
/*
 *  GRUB  --  GRand Unified Bootloader
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

#define end _end
#include "shared.h"
#undef end

char *end = _end;

unsigned long install_partition = 0x20000;
unsigned long boot_drive = 0;
char version_string[] = "0.5";
char config_file[] = "/boot/grub/menu.lst";


/* The main entry point into this mess. */
void
start_stage2 (void)
{
  init_bios_info ();
}

/* calls for direct boot-loader chaining */
void
chain_stage1(int segment, int offset, int part_table_addr)
{

}


void
chain_stage2(int segment, int offset)
{

}


/* do some funky stuff, then boot linux */
void
linux_boot(void)
{

}


/* For bzImage kernels. */
void
big_linux_boot(void)
{

}


/* booting a multiboot executable */
void
multi_boot(int start, int mbi)
{

}

/* sets it to linear or wired A20 operation */
void
gateA20 (int linear)
{
  /* Nothing to do in the simulator. */
}


/* memory probe routines */
int
get_memsize (int type)
{
  if (! type)
    return 640 * 1024;		/* 640kB conventional */
  else
    return 4 * 1024 * 1024;	/* 4MB extended */
}


int
get_eisamemsize (void)
{
}


int
get_mmap_entry (int buf, int cont)
{
}


/* Address and old continuation value for the Query System Address Map
   BIOS call. */
int
get_mem_map (int addr, int cont)
{
  return 0;
}


/* low-level timing info */
int
getrtsecs (void)
{
  return 0xff;
}


/* low-level character I/O */
void
cls(void)
{
}


/* returns packed values, LSB+1 is x, LSB is y */
int
getxy(void)
{
}


void
gotoxy(int x, int y)
{
}


/* displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones */
void
putchar(int c)
{
}


/* returns packed BIOS/ASCII code */
int
asm_getkey(void)
{
}


/* like 'getkey', but doesn't wait, returns -1 if nothing available */
int
checkkey(void)
{
}


/* sets text mode character attribute at the cursor position */
void
set_attrib(int attr)
{
}


/* low-level disk I/O */
int
get_diskinfo(int drive)
{
}


int
biosdisk(int subfunc, int drive, int geometry,
	 int sector, int nsec, int segment)
{
}


void
stop_floppy(void)
{
}
