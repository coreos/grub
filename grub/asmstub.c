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
#define _LARGEFILE_SOURCE	1
/* lseek becomes synonymous with lseek64.  */
#define _FILE_OFFSET_BITS	64

/* Simulator entry point. */
int grub_stage2 (void);

/* We want to prevent any circularararity in our stubs, as well as
   libc name clashes. */
#define WITHOUT_LIBC_STUBS 1
#include "shared.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#ifdef __linux__
# include <sys/ioctl.h>		/* ioctl */
# include <linux/hdreg.h>	/* HDIO_GETGEO */
# if __GLIBC__ < 2
/* Maybe libc doesn't have large file support.  */
#  include <linux/unistd.h>	/* _llseek */
#  include <linux/fs.h>		/* BLKFLSBUF */
# endif /* GLIBC < 2 */
# ifndef BLKFLSBUF
#  define BLKFLSBUF	_IO (0x12,97)
# endif /* ! BLKFLSBUF */
#endif /* __linux__ */

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/ioctl.h>		/* ioctl */
# include <sys/disklabel.h>
#endif /* __FreeBSD__ || __NetBSD__ || __OpenBSD__ */

#ifdef HAVE_OPENDISK
# include <util.h>
#endif /* HAVE_OPENDISK */

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
char version_string[] = VERSION;
char config_file[128] = "/boot/grub/menu.lst"; /* FIXME: arbitrary */

/* Emulation requirements. */
char *grub_scratch_mem = 0;

#define NUM_DISKS 256
struct geometry *disks = 0;

/* The map between BIOS drives and UNIX device file names.  */
char **device_map = 0;

/* The jump buffer for exiting correctly.  */
static jmp_buf env_for_exit;

/* Forward declarations.  */
static void get_floppy_disk_name (char *name, int unit);
static void get_ide_disk_name (char *name, int unit);
static void get_scsi_disk_name (char *name, int unit);
static void init_device_map (void);

/* The main entry point into this mess. */
int
grub_stage2 (void)
{
  /* These need to be static, because they survive our stack transitions. */
  static int status = 0;
  static char *realstack;
  char *scratch, *simstack;
  int i;

  /* We need a nested function so that we get a clean stack frame,
     regardless of how the code is optimized. */
  static volatile void doit ()
  {
    /* Make sure our stack lives in the simulated memory area. */
    asm volatile ("movl %%esp, %0\n\tmovl %1, %%esp\n"
		  : "=&r" (realstack) : "r" (simstack));

    /* Do a setjmp here for the stop command.  */
    if (! setjmp (env_for_exit))
      {
	/* Actually enter the generic stage2 code.  */
	status = 0;
	init_bios_info ();
      }
    else
      {
	/* If ERRNUM is non-zero, then set STATUS to non-zero.  */
	if (errnum)
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
  /* Initialize DISKS.  */
  for (i = 0; i < NUM_DISKS; i++)
    disks[i].flags = -1;

  init_device_map ();

  /* Check some invariants. */
  assert ((SCRATCHSEG << 4) == SCRATCHADDR);
  assert ((BUFFERSEG << 4) == BUFFERADDR);
  assert (BUFFERADDR + BUFFERLEN == SCRATCHADDR);
  assert (FSYS_BUF % 16 == 0);
  assert (FSYS_BUF + FSYS_BUFLEN == BUFFERADDR);

#ifdef HAVE_LIBCURSES
  /* Get into char-at-a-time mode. */
  if (use_curses)
    {
      initscr ();
      cbreak ();
      noecho ();
      nonl ();
      scrollok (stdscr, TRUE);
      keypad (stdscr, TRUE);
      wtimeout (stdscr, 100);
    }
#endif

  /* Make sure that actual writing is done.  */
  sync ();

  /* Set our stack, and go for it. */
  simstack = (char *) PROTSTACKINIT;
  doit ();

  /* I don't know if this is necessary really.  */
  sync ();

#ifdef HAVE_LIBCURSES
  if (use_curses)
    endwin ();
#endif

  /* Close off the file descriptors we used. */
  for (i = 0; i < NUM_DISKS; i ++)
    if (disks[i].flags != -1)
      {
#ifdef __linux__
	/* In Linux, invalidate the buffer cache. In other OSes, reboot
	   is one of the solutions...  */
	ioctl (disks[i].flags, BLKFLSBUF, 0);
#else
# warning "In your operating system, the buffer cache will not be flushed."
#endif
	close (disks[i].flags);
      }

  /* Release memory. */
  for (i = 0; i < NUM_DISKS; i++)
    free (device_map[i]);
  free (device_map[i]);
  device_map = 0;
  free (disks);
  disks = 0;
  free (scratch);
  grub_scratch_mem = 0;

  /* Ahh... at last we're ready to return to caller. */
  return status;
}

static void
init_device_map (void)
{
  int i;
  int num_hd = 0;
  FILE *fp = 0;

  static void print_error (int no, const char *msg)
    {
      fprintf (stderr, "%s:%d: error: %s\n", device_map_file, no, msg);
    }

  assert (device_map == 0);
  device_map = malloc (NUM_DISKS * sizeof (char *));
  assert (device_map);

  /* Probe devices for creating the device map.  */

  /* Initialize DEVICE_MAP.  */
  for (i = 0; i < NUM_DISKS; i++)
    device_map[i] = 0;

  if (device_map_file)
    {
      /* Open the device map file.  */
      fp = fopen (device_map_file, "r");
      if (fp)
	{
	  /* If there is the device map file, use the data in it instead of
	     probing devices.  */
	  char buf[1024];		/* XXX */
	  int line_number = 0;

	  while (fgets (buf, sizeof (buf), fp))
	    {
	      char *ptr, *eptr;
	      int drive;
	      int is_floppy = 0;

	      /* Increase the number of lines.  */
	      line_number++;

	      /* If the first character is '#', skip it.  */
	      if (buf[0] == '#')
		continue;

	      ptr = buf;
	      /* Skip leading spaces.  */
	      while (*ptr && isspace (*ptr))
		ptr++;

	      if (*ptr != '(')
		{
		  print_error (line_number, "No open parenthesis found");
		  stop ();
		}

	      ptr++;
	      if ((*ptr != 'f' && *ptr != 'h') || *(ptr + 1) != 'd')
		{
		  print_error (line_number, "Bad drive name");
		  stop ();
		}

	      if (*ptr == 'f')
		is_floppy = 1;

	      ptr += 2;
	      drive = strtoul (ptr, &ptr, 10);
	      if (drive < 0 || drive > 8)
		{
		  print_error (line_number, "Bad device number");
		  stop ();
		}

	      if (! is_floppy)
		drive += 0x80;

	      if (*ptr != ')')
		{
		  print_error (line_number, "No close parenthesis found");
		  stop ();
		}

	      ptr++;
	      /* Skip spaces.  */
	      while (*ptr && isspace (*ptr))
		ptr++;

	      if (! *ptr)
		{
		  print_error (line_number, "No filename found");
		  stop ();
		}

	      /* Terminate the filename.  */
	      eptr = ptr;
	      while (*eptr && ! isspace (*eptr))
		eptr++;
	      *eptr = 0;

	      assign_device_name (drive, ptr);
	    }

	  fclose (fp);
	  return;
	}
    }

  /* Print something as the user does not think GRUB has been crashed.  */
  fprintf (stderr,
	   "Probe devices to guess BIOS drives. This may take a long time.\n");

  if (device_map_file)
    /* Try to open the device map file to write the probed data.  */
    fp = fopen (device_map_file, "w");

  /* Floppies.  */
  if (! no_floppy)
    for (i = 0; i < 2; i++)
      {
	char name[16];

	if (i == 1 && ! probe_second_floppy)
	  break;

	get_floppy_disk_name (name, i);
	/* In floppies, write the map, whether check_device succeeds
	   or not, because the user just does not insert floppies.  */
	if (fp)
	  fprintf (fp, "(fd%d)\t%s\n", i, name);

	if (check_device (name))
	  assign_device_name (i, name);
      }

  /* IDE disks.  */
  for (i = 0; i < 4; i++)
    {
      char name[16];

      get_ide_disk_name (name, i);
      if (check_device (name))
	{
	  assign_device_name (num_hd + 0x80, name);

	  /* If the device map file is opened, write the map.  */
	  if (fp)
	    fprintf (fp, "(hd%d)\t%s\n", num_hd, name);

	  num_hd++;
	}
    }

  /* The rest is SCSI disks.  */
  for (i = 0; i < 8; i++)
    {
      char name[16];

      get_scsi_disk_name (name, i);
      if (check_device (name))
	{
	  assign_device_name (num_hd + 0x80, name);

	  /* If the device map file is opened, write the map.  */
	  if (fp)
	    fprintf (fp, "(hd%d)\t%s\n", num_hd, name);

	  num_hd++;
	}
    }

  /* OK, close the device map file if opened.  */
  if (fp)
    fclose (fp);
}

/* These three functions are quite different among OSes.  */
static void
get_floppy_disk_name (char *name, int unit)
{
#if defined(__linux__) || defined(__GNU__)
  /* GNU/Linux and GNU/Hurd */
  sprintf (name, "/dev/fd%d", unit);
#elif defined(__FreeBSD__)
  /* FreeBSD */
  sprintf (name, "/dev/rfd%d", unit);
#elif defined(__NetBSD__)
  /* NetBSD */
  /* opendisk() doesn't work for floppies.  */
  sprintf (name, "/dev/rfd%da", unit);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rfd%dc", unit);
#else
# warning "BIOS floppy drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

static void
get_ide_disk_name (char *name, int unit)
{
#if defined(__linux__)
  /* GNU/Linux */
  sprintf (name, "/dev/hd%c", unit + 'a');
#elif defined(__GNU__)
  /* GNU/Hurd */
  sprintf (name, "/dev/hd%d", unit);
#elif defined(__FreeBSD__)
  /* FreeBSD */
  sprintf (name, "/dev/rwd%d", unit);
#elif defined(__NetBSD__) && defined(HAVE_OPENDISK)
  /* NetBSD */
  char shortname[16];
  int fd;

  sprintf (shortname, "wd%d", unit);
  fd = opendisk (shortname, O_RDONLY, name,
		 16,	/* length of NAME */
		 0	/* char device */
		 );
  close (fd);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rwd%dc", unit);
#else
# warning "BIOS IDE drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

static void
get_scsi_disk_name (char *name, int unit)
{
#if defined(__linux__)
  /* GNU/Linux */
  sprintf (name, "/dev/sd%c", unit + 'a');
#elif defined(__GNU__)
  /* GNU/Hurd */
  sprintf (name, "/dev/sd%d", unit);
#elif defined(__FreeBSD__)
  /* FreeBSD */
  sprintf (name, "/dev/rda%d", unit);
#elif defined(__NetBSD__) && defined(HAVE_OPENDISK)
  /* NetBSD */
  char shortname[16];
  int fd;

  sprintf (shortname, "sd%d", unit);
  fd = opendisk (shortname, O_RDONLY, name,
		 16,	/* length of NAME */
		 0	/* char device */
		 );
  close (fd);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rsd%dc", unit);
#else
# warning "BIOS SCSI drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

/* Check if DEVICE can be read. If an error occurs, return zero,
   otherwise return non-zero.  */
int
check_device (const char *device)
{
  char buf[512];
  FILE *fp;

  fp = fopen (device, "r");
  if (! fp)
    {
      switch (errno)
	{
#ifdef ENOMEDIUM
	case ENOMEDIUM:
# if 0
	  /* At the moment, this finds only CDROMs, which can't be
	     read anyway, so leave it out. Code should be
	     reactivated if `removable disks' and CDROMs are
	     supported.  */
	  /* Accept it, it may be inserted.  */
	  return 1;
# endif
	  break;
#endif /* ENOMEDIUM */
	default:
	  /* Break case and leave.  */
	  break;
	}
      /* Error opening the device.  */
      return 0;
    }

  /* Attempt to read the first sector.  */
  if (fread (buf, 1, 512, fp) != 512)
    {
      fclose (fp);
      return 0;
    }

  fclose (fp);
  return 1;
}

/* Assign DRIVE to a device name DEVICE.  */
void
assign_device_name (int drive, const char *device)
{
  /* If DRIVE is already assigned, free it.  */
  if (device_map[drive])
    free (device_map[drive]);

  /* If the old one is already opened, close it.  */
  if (disks[drive].flags != -1)
    {
      close (disks[drive].flags);
      disks[drive].flags = -1;
    }

  /* Assign DRIVE to DEVICE.  */
  if (! device)
    device_map[drive] = 0;
  else
    device_map[drive] = strdup (device);
}

void
stop (void)
{
#ifdef HAVE_LIBCURSES
  if (use_curses)
    endwin ();
#endif

  /* Jump to doit.  */
  longjmp (env_for_exit, 1);
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

/* Set up the int15 handler.  */
void
set_int15_handler (void)
{
  /* Nothing to do in the simulator.  */
}

/* Restore the original int15 handler.  */
void
unset_int15_handler (void)
{
  /* Nothing to do in the simulator.  */
}

/* The key map.  */
unsigned short bios_key_map[KEY_MAP_SIZE + 1];
unsigned short ascii_key_map[KEY_MAP_SIZE + 1];

/* Copy MAP to the drive map and set up the int13 handler.  */
void
set_int13_handler (unsigned short *map)
{
  /* Nothing to do in the simulator.  */
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
  if (use_curses)
    clear ();
#endif
}


/* returns packed values, LSB+1 is x, LSB is y */
int
getxy (void)
{
  int y, x;
#ifdef HAVE_LIBCURSES
  if (use_curses)
    getyx (stdscr, y, x);
  else
#endif
  y = x = 0;
  return (x << 8) | (y & 0xff);
}


void
gotoxy (int x, int y)
{
#ifdef HAVE_LIBCURSES
  if (use_curses)
    move (y, x);
#endif
}


/* displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones */
void
grub_putchar (int c)
{
#ifdef HAVE_LIBCURSES
  if (use_curses)
    addch (c);
  else
#endif
  putchar (c);
}


/* The store for ungetch simulation. This is necessary, because
   ncurses-1.9.9g is still used in the world and its ungetch is
   completely broken.  */
#ifdef HAVE_LIBCURSES
static int save_char = ERR;
#endif

/* returns packed BIOS/ASCII code */
int
getkey (void)
{
  int c;

#ifdef HAVE_LIBCURSES
  if (use_curses)
    {
      /* If checkkey has already got a character, then return it.  */
      if (save_char != ERR)
	{
	  c = save_char;
	  save_char = ERR;
	  return c;
	}

      wtimeout (stdscr, -1);
      c = getch ();
      wtimeout (stdscr, 100);
    }
  else
#endif
    c = getchar ();

  /* Quit if we get EOF. */
  if (c == -1)
    stop ();
  return c;
}


/* like 'getkey', but doesn't wait, returns -1 if nothing available */
int
checkkey (void)
{
#ifdef HAVE_LIBCURSES
  if (use_curses)
    {
      int c;

      /* Check for SAVE_CHAR. This should not be true, because this
	 means checkkey is called twice continuously.  */
      if (save_char != ERR)
	return save_char;

      c = getch ();
      /* If C is not ERR, then put it back in the input queue.  */
      if (c != ERR)
	save_char = c;
      return c;
    }
#endif

  /* Just pretend they hit the space bar, then read the real key when
     they call getkey. */
  return ' ';
}


/* sets text mode character attribute at the cursor position */
void
set_attrib (int attr)
{
#ifdef HAVE_LIBCURSES
  if (use_curses)
    {
      /* FIXME: I don't know why, but chgat doesn't work as expected, so
	 use this dirty way... - okuji  */
      chtype ch = inch ();
      addch ((ch & A_CHARTEXT) | attr);
# if 0
      chgat (1, attr, 0, NULL);
# endif
    }
#endif
}


/* Get the geometry of a drive DRIVE. If an error occurs, return zero,
   otherwise non-zero.  */
static int
get_drive_geometry (int drive)
{
  struct geometry *geom = &disks[drive];

#if defined(__linux__)
  /* Linux */
  struct hd_geometry hdg;
  if (ioctl (geom->flags, HDIO_GETGEO, &hdg))
    return 0;

  /* Got the geometry, so save it. */
  geom->cylinders = hdg.cylinders;
  geom->heads = hdg.heads;
  geom->sectors = hdg.sectors;
  /* FIXME: Should get the real number of sectors.  */
  geom->total_sectors = hdg.cylinders * hdg.heads * hdg.sectors;
  return 1;

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
  /* FreeBSD */
  struct disklabel hdg;
  if (ioctl (disks[drive].flags, DIOCGDINFO, &hdg))
    return 0;

  disks[drive].cylinders = hdg.d_ncylinders;
  disks[drive].heads = hdg.d_ntracks;
  disks[drive].sectors = hdg.d_nsectors;
  disks[drive].total_sectors = hdg.d_secperunit;
  return 1;

#else
# warning "In your operating system, automatic detection of geometries \
will not be performed."
  return 0;
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
  if (disks[drive].flags == -1)
    {
      /* The unpartitioned device name: /dev/XdX */
      char *devname = device_map[drive];
      char buf[512];

      if (! devname)
	return -1;

      if (verbose)
	grub_printf ("Attempt to open drive 0x%x (%s)\n",
		     drive, devname);

      /* Open read/write, or read-only if that failed. */
      if (! read_only)
	disks[drive].flags = open (devname, O_RDWR);

      if (disks[drive].flags == -1)
	{
	  if (read_only || errno == EACCES || errno == EROFS)
	    {
	      disks[drive].flags = open (devname, O_RDONLY);
	      if (disks[drive].flags == -1)
		{
		  assign_device_name (drive, 0);
		  return -1;
		}
	    }
	  else
	    {
	      assign_device_name (drive, 0);
	      return -1;
	    }
	}

      /* Attempt to read the first sector.  */
      if (read (disks[drive].flags, buf, 512) != 512)
	{
	  close (disks[drive].flags);
	  disks[drive].flags = -1;
	  assign_device_name (drive, 0);
	  return -1;
	}

      if (disks[drive].flags != -1)
	{
	  if (! get_drive_geometry (drive))
	    {
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
    }

  if (disks[drive].flags == -1)
    return -1;

#ifdef __linux__
  /* In Linux, invalidate the buffer cache, so that left overs
     from other program in the cache are flushed and seen by us */
  ioctl (disks[drive].flags, BLKFLSBUF, 0);
#endif

  *geometry = disks[drive];
  return 0;
}

/* Read LEN bytes from FD in BUF. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
static int
nread (int fd, char *buf, size_t len)
{
  int size = len;

  while (len)
    {
      int ret = read (fd, buf, len);

      if (ret <= 0)
	{
	  if (errno == EINTR)
	    continue;
	  else
	    return ret;
	}

      len -= ret;
      buf += ret;
    }

  return size;
}

/* Write LEN bytes from BUF to FD. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
static int
nwrite (int fd, char *buf, size_t len)
{
  int size = len;

  while (len)
    {
      int ret = write (fd, buf, len);

      if (ret <= 0)
	{
	  if (errno == EINTR)
	    continue;
	  else
	    return ret;
	}

      len -= ret;
      buf += ret;
    }

  return size;
}

/* Dump BUF in the format of hexadecimal numbers.  */
static void
hex_dump (void *buf, size_t size)
{
  /* FIXME: How to determine which length is readable?  */
#define MAX_COLUMN	70

  /* use unsigned char for numerical computations */
  unsigned char *ptr = buf;
  /* count the width of the line */
  int column = 0;
  /* how many bytes written */
  int count = 0;

  while (size > 0)
    {
      /* high 4 bits */
      int hi = *ptr >> 4;
      /* low 4 bits */
      int low = *ptr & 0xf;

      /* grub_printf does not handle prefix number, such as %2x, so
	 format the number by hand...  */
      grub_printf ("%x%x", hi, low);
      column += 2;
      count++;
      ptr++;
      size--;

      /* Insert space or newline with the interval 4 bytes.  */
      if (size != 0 && (count % 4) == 0)
	{
	  if (column < MAX_COLUMN)
	    {
	      grub_printf (" ");
	      column++;
	    }
	  else
	    {
	      grub_printf ("\n");
	      column = 0;
	    }
	}
    }

  /* Add a newline at the end for readability.  */
  grub_printf ("\n");
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
#if defined(__linux) && (__GLIBC__ < 2)
  /* Maybe libc doesn't have large file support.  */
  {
    loff_t offset, result;
    static int _llseek (uint fd, ulong hi, ulong lo, loff_t *res, uint wh);
    _syscall5 (int, _llseek, uint, fd, ulong, hi, ulong, lo,
	       loff_t *, res, uint, wh);

    offset = (loff_t) sector * (loff_t) SECTOR_SIZE;
    if (_llseek (fd, offset >> 32, offset & 0xffffffff, &result, SEEK_SET))
      return -1;
  }
#else
  {
    off_t offset = (off_t) sector * (off_t) SECTOR_SIZE;

    if (lseek (fd, offset, SEEK_SET) != offset)
      return -1;
  }
#endif

  buf = (char *) (segment << 4);

  switch (subfunc)
    {
    case BIOSDISK_READ:
      if (nread (fd, buf, nsec * SECTOR_SIZE) != nsec * SECTOR_SIZE)
	return -1;
      break;

    case BIOSDISK_WRITE:
      if (verbose)
	{
	  grub_printf ("Write %d sectors starting from %d sector"
		       " to drive 0x%x (%s)\n",
		       nsec, sector, drive, device_map[drive]);
	  hex_dump (buf, nsec * SECTOR_SIZE);
	}
      if (! read_only)
	if (nwrite (fd, buf, nsec * SECTOR_SIZE) != nsec * SECTOR_SIZE)
	  return -1;
      break;

    default:
      grub_printf ("unknown subfunc %d\n", subfunc);
      break;
    }

  return 0;
}


void
stop_floppy (void)
{
  /* NOTUSED */
}
