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

/* Try to use glibc's transparant LFS support. */
#define _LARGEFILE_SOURCE 1

/* Simulator entry point. */
int grub_stage2 (void);

/* We want to prevent any circularararity in our stubs, as well as
   libc name clashes. */
#define WITHOUT_LIBC_STUBS 1
#include "shared.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifdef __linux__
# include <sys/ioctl.h>		/* ioctl */
# include <linux/hdreg.h>	/* HDIO_GETGEO */
/* FIXME: only include if libc doesn't have large file support. */
# include <unistd.h>
# include <linux/unistd.h>	/* _llseek */
#endif /* __linux__ */

/* Simulated memory sizes. */
#define EXTENDED_MEMSIZE (4 * 1024 * 1024)	/* 4MB */
#define CONVENTIONAL_MEMSIZE (640) /* 640kB */

/* Simulated disk sizes. */
#define DEFAULT_FD_CYLINDERS	80
#define DEFAULT_FD_HEADS	2
#define DEFAULT_FD_SECTORS	18
#define DEFAULT_HD_CYLINDERS	620
#define DEFAULT_HD_HEADS	128
#define DEFAULT_HD_SECTORS	63

unsigned long install_partition = 0x20000;
unsigned long boot_drive = 0;
char version_string[] = "0.5";
char *config_file = "/boot/grub/menu.lst";

/* Emulation requirements. */
char *grub_scratch_mem = 0;

#define NUM_DISKS 256
static struct geometry *disks = 0;

/* The main entry point into this mess. */
int
grub_stage2 (void)
{
  /* These need to be static, because they survive our stack transitions. */
  static int status = 0;
  static char *realstack;
  int i;
  char *scratch, *simstack;

  /* We need a nested function so that we get a clean stack frame,
     regardless of how the code is optimized. */
  static volatile void doit ()
  {
    /* Make sure our stack lives in the simulated memory area. */
    asm volatile ("movl %%esp, %0\n\tmovl %1, %%esp\n"
		  : "&=r" (realstack) : "r" (simstack));

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

    /* Replace our stack before we use any local variables. */
    asm volatile ("movl %0, %%esp\n" : : "r" (realstack));
  }

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
  nodelay (stdscr, TRUE);
#endif

  /* Set our stack, and go for it. */
  simstack = (char *) PROTSTACKINIT;
  doit ();

#ifdef HAVE_LIBCURSES
  endwin ();
#endif

  /* Close off the file descriptors we used. */
  for (i = 0; i < NUM_DISKS; i ++)
    if (disks[i].flags)
      close (disks[i].flags);

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
  /* FIXME: exact value is not important, so just return time_t for now. */
  return time (0);
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
  int c;
  c = getch ();
  /* If C is not ERR, then put it back in the input queue.  */
  if (c != ERR)
    ungetch (c);	/* FIXME: ncurses-1.9.9g ungetch is buggy.  */
  return c;
#else
  /* Just pretend they hit the space bar. */
  return ' ';
#endif
}


/* sets text mode character attribute at the cursor position */
void
set_attrib (int attr)
{
#ifdef HAVE_LIBCURSES
  /* FIXME: I don't know why, but chgat doesn't work as expected, so
     use this dirty way... - okuji  */
  chtype ch = inch ();
  addch ((ch & A_CHARTEXT) | attr);
# if 0
  chgat (1, attr, 0, NULL);
# endif
#endif
}


/* Low-level disk I/O.  Our stubbed version just returns a file
   descriptor, not the actual geometry. */
int
get_diskinfo (int drive, struct geometry *geometry)
{
  /* FIXME: this function is truly horrid.  We try opening the device,
     then severely abuse the GEOMETRY->flags field to pass a file
     descriptor to biosdisk.  Thank God nobody's looking at this comment,
     or my reputation would be ruined. --Gord */

  /* See if we have a cached device. */
  if (! disks[drive].flags)
    {
      /* The unpartitioned device name: /dev/XdX */
      char devname[9];

      /* Try opening the drive device. */
      strcpy (devname, "/dev/");

#ifdef __linux__
      /* Linux uses /dev/hda, and /dev/sda, but /dev/fd0.  Go figure. */
      if (drive & 0x80)
	{
	  /* Check to make sure we don't exceed /dev/hdz. */
	  devname[5] = 'h';
	  devname[7] = 'a' + (drive & 0x7f);
	  if (devname[7] > 'z')
	    return -1;
	}
      else
	{
	  devname[5] = 'f';
	  devname[7] = '0' + drive;
	  if (devname[7] > '9')
	    return -1;
	}
#else /* ! __linux__ */
      if (drive & 0x80)
	devname[5] = 'h';

      devname[7] = '0' + drive;
      if (devname[7] > '9')
	return -1;
#endif /* __linux__ */

      devname[6] = 'd';
      devname[8] = '\0';

      /* Open read/write, or read-only if that failed. */
      disks[drive].flags = open (devname, O_RDWR);
      if (! disks[drive].flags)
	disks[drive].flags = open (devname, O_RDONLY);

      if (disks[drive].flags)
	{
#ifdef __linux__
	  struct hd_geometry hdg;
	  if (! ioctl (disks[drive].flags, HDIO_GETGEO, &hdg))
	    {
	      /* Got the geometry, so save it. */
	      disks[drive].cylinders = hdg.cylinders;
	      disks[drive].heads = hdg.heads;
	      disks[drive].sectors = hdg.sectors;
	      /* FIXME: Should get the real number of sectors.  */
	      disks[drive].total_sectors = (hdg.cylinders
					    * hdg.heads
					    * hdg.sectors);
	    }
	  else
	    /* FIXME: should have some other alternatives before using
	       arbitrary defaults. */
#endif
	  /* Set some arbitrary defaults. */
	  if (drive & 0x80)
	    {
	      /* Hard drive. */
	      disks[drive].cylinders = DEFAULT_HD_CYLINDERS;
	      disks[drive].heads = DEFAULT_HD_HEADS;
	      disks[drive].sectors = DEFAULT_HD_SECTORS;
	      disks[drive].total_sectors = (DEFAULT_HD_CYLINDERS
					    * DEFAULT_HD_HEADS
					    * DEFAULT_HD_SECTORS);
	    }
	  else
	    {
	      /* Floppy. */
	      disks[drive].cylinders = DEFAULT_FD_CYLINDERS;
	      disks[drive].heads = DEFAULT_FD_HEADS;
	      disks[drive].sectors = DEFAULT_FD_SECTORS;
	      disks[drive].total_sectors = (DEFAULT_FD_CYLINDERS
					    * DEFAULT_FD_HEADS
					    * DEFAULT_FD_SECTORS);
	    }
	}
    }

  if (! disks[drive].flags)
    return -1;

  *geometry = disks[drive];
  return 0;
}

int
biosdisk (int subfunc, int drive, struct geometry *geometry,
	  int sector, int nsec, int segment)
{
  char *buf;
  int fd = geometry->flags;

  /* Get the file pointer from the geometry, and make sure it matches. */
  if (fd == -1 || fd != disks[drive].flags)
    return BIOSDISK_ERROR_GEOMETRY;

  /* Seek to the specified location. */
#ifdef __linux__
  /* FIXME: only use this section if libc doesn't have large file support */
  {
    loff_t offset, result;
    _syscall5 (int, _llseek, uint, fd, ulong, hi, ulong, lo,
	       loff_t *, res, uint, wh);

    offset = (loff_t) sector * (loff_t) SECTOR_SIZE;
    if (_llseek (fd, offset >> 32, offset & 0xffffffff, &result, SEEK_SET))
      return -1;
  }
#else
  if (lseek (fd, sector * SECTOR_SIZE, SEEK_SET))
     return -1;
#endif /* __linux__ */

  buf = (char *) (segment << 4);
  /* FIXME: handle EINTR */
  if (read (fd, buf, nsec * SECTOR_SIZE) != nsec * SECTOR_SIZE)
    return -1;
  return 0;
}


void
stop_floppy (void)
{
  /* NOTUSED */
}
