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

/* Simulator entry point. */
int grub_stage2 (void);

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

/* Emulation requirements. */
char *grub_scratch_mem = 0;

#define NUM_DISKS 256
static FILE **disks = 0;

/* The main entry point into this mess. */
int
grub_stage2 (void)
{
  /* These need to be static, because they survive our stack transitions. */
  static int status = 0;
  static char *realstack;
  int i;
  char *scratch, *simstack;

  assert (grub_scratch_mem == 0);
  scratch = malloc (0x100000 + EXTENDED_MEMSIZE + 15);
  assert (scratch);
  grub_scratch_mem = (char *) ((((int) scratch) >> 4) << 4);

  /* FIXME: simulate the memory holes using mprot, if available. */

  assert (disks == 0);
  disks = malloc (NUM_DISKS * sizeof (*disks));
  assert (disks);

  /* Check some invariants. */
  assert ((SCRATCHSEG << 4) == SCRATCHADDR);
  assert ((BUFFERSEG << 4) == BUFFERADDR);
  assert (BUFFERADDR + BUFFERLEN == SCRATCHADDR);
  assert (FSYS_BUF % 16 == 0);
  assert (FSYS_BUF + FSYS_BUFLEN == BUFFERADDR);

#ifdef HAVE_LIBCURSES
  /* Get into char-at-a-time mode. */
  initscr ();
  cbreak ();
  noecho ();
  nonl ();
  scrollok (stdscr, TRUE);
  keypad (stdscr, TRUE);
#endif

  /* Make sure our stack lives in the simulated memory area. */
  simstack = (char *) RAW_ADDR (PROTSTACKINIT);
#ifdef SIMULATE_STACK
  __asm __volatile ("movl %%esp, %0;" : "=d" (realstack));
  __asm __volatile ("movl %0, %%esp" :: "d" (simstack));
#endif

  {
    /* FIXME: Do a setjmp here for the stop command. */
    if (1)
      {
	/* Actually enter the generic stage2 code. */
	status = 0;
	init_bios_info ();
      }
    else
      {
	/* Somebody aborted. */
	status = 1;
      }
  }

#ifdef SIMULATE_STACK
  /* Replace our stack before we use any local variables. */
  __asm __volatile ("movl %0, %%esp" :: "d" (realstack));
#endif

#ifdef HAVE_LIBCURSES
  endwin ();
#endif

  /* Close off the file pointers we used. */
  for (i = 0; i < NUM_DISKS; i ++)
    if (disks[i])
      fclose (disks[i]);

  /* Release memory. */
  free (disks);
  disks = 0;
  free (scratch);
  grub_scratch_mem = 0;

  /* Ahh... at last we're ready to return to caller. */
  return status;
}


void
stop (void)
{
#ifdef HAVE_LIBCURSES
  endwin ();
#endif
  /* FIXME: If we don't exit, then we need to free our data areas. */
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


int
get_code_end (void)
{
  /* Just return a little area for simulation. */
  return BOOTSEC_LOCATION + (60 * 1024);
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

/* Fetch the next entry in the memory map and return the continuation
   value.  DESC is a pointer to the descriptor buffer, and CONT is the
   previous continuation value (0 to get the first entry in the
   map). */
int
get_mmap_entry (struct mmar_desc *desc, int cont)
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
#ifdef HAVE_LIBCURSES
  clear ();
#endif
}


/* returns packed values, LSB+1 is x, LSB is y */
int
getxy (void)
{
  int y, x;
#ifdef HAVE_LIBCURSES
  getyx (stdscr, y, x);
#else
  y = x = 0;
#endif
  return (x << 8) | (y & 0xff);
}


void
gotoxy (int x, int y)
{
#ifdef HAVE_LIBCURSES
  move (y, x);
#endif
}


/* displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones */
void
grub_putchar (int c)
{
#ifdef HAVE_LIBCURSES
  addch (c);
#else
  putchar (c);
#endif
}


/* returns packed BIOS/ASCII code */
int
getkey (void)
{
#ifdef HAVE_LIBCURSES
  int c;
  nodelay (stdscr, FALSE);
  c = getch ();
  nodelay (stdscr, TRUE);
  return c;
#else
  return getchar ();
#endif
}


/* like 'getkey', but doesn't wait, returns -1 if nothing available */
int
checkkey (void)
{
#ifdef HAVE_LIBCURSES
  return getch ();
#else
  return getchar ();
#endif
}


/* sets text mode character attribute at the cursor position */
void
set_attrib (int attr)
{
#ifdef HAVE_LIBCURSES
  chgat (1, attr, 0, NULL);
#endif
}


/* Low-level disk I/O.  Our stubbed version just returns a file
   descriptor, not the actual geometry. */
int
get_diskinfo (int drive)
{
  /* The unpartitioned device name: /dev/XdX */
  char devname[9];

  /* See if we have a cached device. */
  if (disks[drive])
    return (int) disks[drive];

  /* Try opening the drive device. */
  strcpy (devname, "/dev/");
  if (drive & 0x80)
    devname[5] = 'h';
  else
    devname[5] = 'f';
  devname[6] = 'd';

  /* Check to make sure we don't exceed /dev/hdz. */
  devname[7] = (drive & 0x7f) + 'a';
  if (devname[7] > 'z')
    return 0;
  devname[8] = '\0';

  /* Open read/write, or read-only if that failed. */
  disks[drive] = fopen (devname, "r+");
  if (! disks[drive])
    disks[drive] = fopen (devname, "r");

  return (int) disks[drive];
}

int
biosdisk (int subfunc, int drive, int geometry,
	  int sector, int nsec, int segment)
{
  char *buf;
  FILE *fp;

  /* Get the file pointer from the geometry, and make sure it matches. */
  fp = (FILE *) geometry;
  if (! fp || fp != disks[drive])
    return BIOSDISK_ERROR_GEOMETRY;

  /* Seek to the specified location. */
  if (fseek (fp, sector * SECTOR_SIZE, SEEK_SET))
    return -1;

  buf = (char *) (segment << 4);
  if (fread (buf, nsec * SECTOR_SIZE, 1, fp) != 1)
    return -1;
  return 0;
}


void
stop_floppy (void)
{
  /* NOTUSED */
}
