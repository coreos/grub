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

#include "shared.h"
/* We want to prevent any circularararity in our stubs, as well as
   libc name clashes. */
#undef NULL
#undef bcopy
#undef bzero
#undef getc
#undef isspace
#undef printf
#undef putchar
#undef strncat
#undef strstr
#undef tolower

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/* Simulated memory sizes. */
#define EXTENDED_MEMSIZE (4 * 1024 * 1024)	/* 4MB */
#define CONVENTIONAL_MEMSIZE (640) /* 640kB */


unsigned long install_partition = 0x20000;
unsigned long boot_drive = 0;
char version_string[] = "0.5";
char config_file[] = "/boot/grub/menu.lst";
char *grub_scratch_mem = 0;

/* The main entry point into this mess. */
void
start_stage2 (void)
{
  grub_scratch_mem = memalign (16, EXTENDED_MEMSIZE);

  /* Check some invariants. */
  assert (BUFFERADDR + BUFFERLEN == SCRATCHADDR);
  assert (FSYS_BUF % 16 == 0);
  assert (FSYS_BUF + FSYS_BUFLEN == BUFFERADDR);

#ifdef HAVE_LIBCURSES
  /* Get into char-at-a-time mode. */
  initscr ();
  cbreak ();
  noecho ();
  scrollok (stdscr, TRUE);
#endif

  init_bios_info ();
}


void
stop (void)
{
  fprintf (stderr, "grub: aborting...\n");
  exit (1);
}


/* calls for direct boot-loader chaining */
void
chain_stage1 (int segment, int offset, int part_table_addr)
{
  stop ();
}


void
chain_stage2 (int segment, int offset)
{
  stop ();
}


/* do some funky stuff, then boot linux */
void
linux_boot (void)
{
  stop ();
}


/* For bzImage kernels. */
void
big_linux_boot (void)
{
  stop ();
}


/* booting a multiboot executable */
void
multi_boot (int start, int mbi)
{
  stop ();
}

/* sets it to linear or wired A20 operation */
void
gateA20 (int linear)
{
  /* Nothing to do in the simulator. */
}


void *
get_code_end (void)
{
  /* Just return a little area for simulation. */
  return malloc (1024);
}


/* memory probe routines */
int
get_memsize (int type)
{
  if (!type)
    return CONVENTIONAL_MEMSIZE;
  else
    return EXTENDED_MEMSIZE;
}


/* get_eisamemsize() :  return packed EISA memory map, lower 16 bits is
 *		memory between 1M and 16M in 1K parts, upper 16 bits is
 *		memory above 16M in 64K parts.  If error, return -1.
 */
int
get_eisamemsize (void)
{
  return (EXTENDED_MEMSIZE / 1024);
}


#define MMAR_DESC_TYPE_AVAILABLE 1 /* available to OS */
#define MMAR_DESC_TYPE_RESERVED 2 /* not available */
#define MMAR_DESC_TYPE_ACPI_RECLAIM 3 /* usable by OS after reading ACPI */
#define MMAR_DESC_TYPE_ACPI_NVS 4 /* required to save between NVS sessions */

/* Memory map address range descriptor. */
struct mmar_desc
{
  unsigned long desc_len;	/* Size of this descriptor. */
  unsigned long long addr;	/* Base address. */
  unsigned long long length;	/* Length in bytes. */
  unsigned long type;		/* Type of address range. */
};

/* Fetch the next entry in the memory map and return the continuation
   value.  DESC is a pointer to the descriptor buffer, and CONT is the
   previous continuation value (0 to get the first entry in the
   map). */
int
get_mem_map (struct mmar_desc *desc, int cont)
{
  if (! cont)
    {
      /* First entry, located at 1MB. */
      desc->desc_len = sizeof (*desc) - sizeof (desc->desc_len);
      desc->addr = 1024 * 1024;
      desc->length = EXTENDED_MEMSIZE;
      desc->type = MMAR_DESC_TYPE_AVAILABLE;
    }
  else
    {
      /* No more entries, so give an error. */
      desc->desc_len = 0;
    }
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
cls (void)
{
}


/* returns packed values, LSB+1 is x, LSB is y */
int
getxy (void)
{
}


void
gotoxy (int x, int y)
{
}


/* displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones */
void
grub_putchar (int c)
{
  addch (c);
}


/* returns packed BIOS/ASCII code */
int
getkey (void)
{
  int c;
  nodelay (stdscr, FALSE);
  c = getch ();
  nodelay (stdscr, TRUE);
  return c;
}


/* like 'getkey', but doesn't wait, returns -1 if nothing available */
int
checkkey (void)
{
  getch ();
}


/* sets text mode character attribute at the cursor position */
void
set_attrib (int attr)
{
}


/* low-level disk I/O */
int
get_diskinfo (int drive)
{
}


int
biosdisk (int subfunc, int drive, int geometry,
	  int sector, int nsec, int segment)
{
}


void
stop_floppy (void)
{
}
