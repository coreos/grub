/* builtins.c - the GRUB builtin commands */
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

#include <shared.h>

#ifndef GRUB_UTIL
# include "apic.h"
# include "smp-imps.h"
#endif

/* The type of kernel loaded.  */
kernel_t kernel_type;
/* The boot device.  */
static int bootdev;
/* True when the debug mode is turned on, and false when it is turned off.  */
int debug = 0;
/* The default entry.  */
int default_entry = 0;
/* The fallback entry.  */
int fallback_entry = -1;
/* The address for Multiboot command-line buffer.  */
static char *mb_cmdline;
/* The password.  */
char *password;
/* Color settings.  */
int normal_color;
int highlight_color;
/* The timeout.  */
int grub_timeout = -1;

/* Initialize the data for builtins.  */
void
init_builtins (void)
{
  kernel_type = KERNEL_TYPE_NONE;
  /* BSD and chainloading evil hacks!  */
  bootdev = set_bootdev (0);
  mb_cmdline = (char *) MB_CMDLINE_BUF;
}

/* Initialize the data for the configuration file.  */
void
init_config (void)
{
  default_entry = 0;
  normal_color = A_NORMAL;
  highlight_color = A_REVERSE;
  password = 0;
  fallback_entry = -1;
  grub_timeout = -1;
}

/* Print which sector is read when loading a file.  */
static void
debug_fs_print_func (int sector)
{
  grub_printf ("[%d]", sector);
}


/* boot */
static int
boot_func (char *arg, int flags)
{
  switch (kernel_type)
    {
    case KERNEL_TYPE_FREEBSD:
    case KERNEL_TYPE_NETBSD:
      /* *BSD */
      bsd_boot (kernel_type, bootdev, arg);
      break;

    case KERNEL_TYPE_LINUX:
      /* Linux */
      linux_boot ();
      break;

    case KERNEL_TYPE_BIG_LINUX:
      /* Big Linux */
      big_linux_boot ();
      break;

    case KERNEL_TYPE_CHAINLOADER:
      /* Chainloader */
      gateA20 (0);
      boot_drive = saved_drive;
      chain_stage1 (0, BOOTSEC_LOCATION, BOOTSEC_LOCATION - 16);
      break;

    case KERNEL_TYPE_MULTIBOOT:
      /* Multiboot */
      multi_boot ((int) entry_addr, (int) &mbi);
      break;

    default:
      errnum = ERR_BOOT_COMMAND;
      return 1;
    }

  return 0;
}

static struct builtin builtin_boot =
{
  "boot",
  boot_func,
  BUILTIN_CMDLINE,
  "boot",
  "Boot the OS/chain-loader which has been loaded."
};


/* chainloader */
static int
chainloader_func (char *arg, int flags)
{
  if (grub_open (arg)
      && grub_read ((char *) BOOTSEC_LOCATION, SECTOR_SIZE) == SECTOR_SIZE
      && (*((unsigned short *) (BOOTSEC_LOCATION + BOOTSEC_SIG_OFFSET))
	  == BOOTSEC_SIGNATURE))
    kernel_type = KERNEL_TYPE_CHAINLOADER;
  else if (! errnum)
    {
      errnum = ERR_EXEC_FORMAT;
      kernel_type = KERNEL_TYPE_NONE;
      return 1;
    }

  return 0;
}

static struct builtin builtin_chainloader =
{
  "chainloader",
  chainloader_func,
  BUILTIN_CMDLINE,
  "chainloader FILE",
  "Load the chain-loader FILE."
};


/* color */
static int
color_func (char *arg, int flags)
{
  char *normal;
  char *highlight;

  normal = arg;
  highlight = skip_to (0, arg);

  if (safe_parse_maxint (&normal, &normal_color))
    {
      /* The second argument is optional, so set highlight_color
	 to inverted NORMAL_COLOR.  */
      if (*highlight == 0
	  || ! safe_parse_maxint (&highlight, &highlight_color))
	highlight_color = ((normal_color >> 4) | ((normal_color & 0xf) << 4));
    }
  else
    return 1;

  return 0;
}

static struct builtin builtin_color =
{
  "color",
  color_func,
  BUILTIN_CMDLINE | BUILTIN_MENU,
  "color NORMAL [HIGHLIGHT]",
  "Change the menu colors. The color NORMAL is used for most"
  " lines in the menu, and the color HIGHLIGHT is used to highlight the"
  " line where the cursor points. If you omit HIGHLIGHT, then the"
  " inverted color of NORMAL is used for the highlighted line. You"
  " must specify an integer for a color value, where bits 0-3"
  " represent the foreground color, bits 4-6 represents the"
  " background color, and bit 7 indicates that the foreground"
  " blinks."
};


/* configfile */
static int
configfile_func (char *arg, int flags)
{
  char *new_config = config_file;

  /* Check if the file ARG is present.  */
  if (! grub_open (arg))
    return 1;

  /* Copy ARG to CONFIG_FILE.  */
  while ((*new_config++ = *arg++) != 0)
    ;

#ifdef GRUB_UTIL
  /* Force to load the configuration file.  */
  use_config_file = 1;
#endif

  /* Restart cmain.  */
  cmain ();

  /* Never reach here.  */
  return 0;
}

static struct builtin builtin_configfile =
{
  "configfile",
  configfile_func,
  BUILTIN_CMDLINE,
  "configfile FILE",
  "Load FILE as the configuration file."
};


/* debug */
static int
debug_func (char *arg, int flags)
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

  return 0;
}

static struct builtin builtin_debug =
{
  "debug",
  debug_func,
  BUILTIN_CMDLINE,
  "debug",
  "Turn on/off the debug mode."
};


/* default */
static int
default_func (char *arg, int flags)
{
  if (! safe_parse_maxint (&arg, &default_entry))
    return 1;

  return 0;
}

static struct builtin builtin_default =
{
  "default",
  default_func,
  BUILTIN_MENU,
#if 0
  "default NUM",
  "Set the default entry to entry number NUM (if not specified, it is"
  " 0, the first entry)."
#endif
};


/* device */
static int
device_func (char *arg, int flags)
{
#ifdef GRUB_UTIL
  char *drive = arg;
  char *device;
  char *ptr;

  /* Get the drive number from DRIVE.  */
  if (! set_device (drive))
    return 1;

  /* Get the device argument.  */
  device = skip_to (0, drive);
  if (! *device || ! check_device (device))
    {
      errnum = ERR_FILE_NOT_FOUND;
      return 1;
    }

  /* Terminate DEVICE.  */
  ptr = device;
  while (*ptr && *ptr != ' ')
    ptr++;
  *ptr = 0;

  assign_device_name (current_drive, device);
#endif /* GRUB_UTIL */

  return 0;
}

static struct builtin builtin_device =
{
  "device",
  device_func,
  BUILTIN_MENU | BUILTIN_CMDLINE,
  "device DRIVE DEVICE",
  "Specify DEVICE as the actual drive for a BIOS drive DRIVE. This command"
  " is just ignored in the native Stage 2."
};


/* displaymem */
static int
displaymem_func (char *arg, int flags)
{
  if (get_eisamemsize () != -1)
    grub_printf (" EISA Memory BIOS Interface is present\n");
  if (get_mmap_entry ((void *) SCRATCHADDR, 0) != 0
      || *((int *) SCRATCHADDR) != 0)
    grub_printf (" Address Map BIOS Interface is present\n");

  grub_printf (" Lower memory: %uK, "
	       "Upper memory (to first chipset hole): %uK\n",
	       mbi.mem_lower, mbi.mem_upper);

  if (mbi.flags & MB_INFO_MEM_MAP)
    {
      struct AddrRangeDesc *map = (struct AddrRangeDesc *) mbi.mmap_addr;
      int end_addr = mbi.mmap_addr + mbi.mmap_length;

      grub_printf (" [Address Range Descriptor entries "
		   "immediately follow (values are 64-bit)]\n");
      while (end_addr > (int) map)
	{
	  char *str;

	  if (map->Type == MB_ARD_MEMORY)
	    str = "Usable RAM";
	  else
	    str = "Reserved";
	  grub_printf ("   %s:  Base Address:  0x%x X 4GB + 0x%x,\n"
		       "      Length:   %u X 4GB + %u bytes\n",
		       str, map->BaseAddrHigh, map->BaseAddrLow,
		       map->LengthHigh, map->LengthLow);

	  map = ((struct AddrRangeDesc *) (((int) map) + 4 + map->size));
	}
    }

  return 0;
}

static struct builtin builtin_displaymem =
{
  "displaymem",
  displaymem_func,
  BUILTIN_CMDLINE,
  "displaymem",
  "Display what GRUB thinks the system address space map of the"
  " machine is, including all regions of physical RAM installed."
};


/* fallback */
static int
fallback_func (char *arg, int flags)
{
  if (! safe_parse_maxint (&arg, &fallback_entry))
    return 1;

  return 0;
}

static struct builtin builtin_fallback =
{
  "fallback",
  fallback_func,
  BUILTIN_MENU,
#if 0
  "fallback NUM",
  "Go into unattended boot mode: if the default boot entry has any"
  " errors, instead of waiting for the user to do anything, it"
  " immediately starts over using the NUM entry (same numbering as the"
  " `default' command). This obviously won't help if the machine"
  " was rebooted by a kernel that GRUB loaded."
#endif
};


/* fstest */
static int
fstest_func (char *arg, int flags)
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

  return 0;
}

static struct builtin builtin_fstest =
{
  "fstest",
  fstest_func,
  BUILTIN_CMDLINE,
  "fstest",
  "Toggle filesystem test mode."
};


/* geometry */
static int
geometry_func (char *arg, int flags)
{
  struct geometry geom;
  char *msg;
  char *device = arg;
#ifdef GRUB_UTIL
  char *ptr;
#endif
  
  set_device (device);
  if (errnum)
    return 1;

  if (get_diskinfo (current_drive, &geom))
    {
      errnum = ERR_NO_DISK;
      return 1;
    }

#ifdef GRUB_UTIL
  ptr = skip_to (0, device);
  if (*ptr)
    {
      char *cylinder, *head, *sector, *total_sector;
      int num_cylinder, num_head, num_sector, num_total_sector;

      cylinder = ptr;
      head = skip_to (0, cylinder);
      sector = skip_to (0, head);
      total_sector = skip_to (0, sector);
      if (! safe_parse_maxint (&cylinder, &num_cylinder)
	  || ! safe_parse_maxint (&head, &num_head)
	  || ! safe_parse_maxint (&sector, &num_sector))
	return 1;

      disks[current_drive].cylinders = num_cylinder;
      disks[current_drive].heads = num_head;
      disks[current_drive].sectors = num_sector;

      if (safe_parse_maxint (&total_sector, &num_total_sector))
	disks[current_drive].total_sectors = num_total_sector;
      else
	disks[current_drive].total_sectors
	  = num_cylinder * num_head * num_sector;
      errnum = 0;

      geom = disks[current_drive];
      buf_drive = -1;
    }
#endif /* GRUB_UTIL */
  
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
  
  return 0;
}

static struct builtin builtin_geometry =
{
  "geometry",
  geometry_func,
  BUILTIN_CMDLINE,
  "geometry DRIVE [CYLINDER HEAD SECTOR [TOTAL_SECTOR]]",
  "Print the information for a drive DRIVE. In the grub shell, you can"
  "set the geometry of the drive arbitrarily. The number of the cylinders,"
  " the one of the heads, the one of the sectors and the one of the total"
  " sectors are set to CYLINDER, HEAD, SECTOR and TOTAL_SECTOR,"
  "respectively. If you omit TOTAL_SECTOR, then it will be calculated based"
  " on the C/H/S values automatically."
};


/* help */
#define MAX_SHORT_DOC_LEN	39
#define MAX_LONG_DOC_LEN	66

static int
help_func (char *arg, int flags)
{
  if (! *arg)
    {
      /* Invoked with no argument. Print the list of the short docs.  */
      struct builtin **builtin;
      int left = 1;

      for (builtin = builtin_table; *builtin != 0; builtin++)
	{
	  int len;
	  int i;

	  /* If this cannot be run in the command-line interface,
	     skip this.  */
	  if (! ((*builtin)->flags & BUILTIN_CMDLINE))
	    continue;

	  len = grub_strlen ((*builtin)->short_doc);
	  /* If the length of SHORT_DOC is too long, truncate it.  */
	  if (len > MAX_SHORT_DOC_LEN - 1)
	    len = MAX_SHORT_DOC_LEN - 1;

	  for (i = 0; i < len; i++)
	    grub_putchar ((*builtin)->short_doc[i]);

	  for (; i < MAX_SHORT_DOC_LEN; i++)
	    grub_putchar (' ');

	  if (! left)
	    grub_putchar ('\n');

	  left = ! left;
	}
    }
  else
    {
      /* Invoked with one or more patterns.  */
      do
	{
	  struct builtin **builtin;
	  char *ptr = arg;
	  char *next_arg;

	  /* Get the next argument.  */
	  next_arg = skip_to (0, arg);

	  /* Terminate ARG.  */
	  while (*ptr && *ptr != ' ')
	    ptr++;
	  *ptr = 0;

	  for (builtin = builtin_table; *builtin; builtin++)
	    {
	      /* Skip this if this is only for the configuration file.  */
	      if (! ((*builtin)->flags & BUILTIN_CMDLINE))
		continue;

	      if (substring (arg, (*builtin)->name) < 1)
		{
		  char *doc = (*builtin)->long_doc;

		  /* At first, print the name and the short doc.  */
		  grub_printf ("%s: %s\n",
			       (*builtin)->name, (*builtin)->short_doc);

		  /* Print the long doc.  */
		  while (*doc)
		    {
		      int len = grub_strlen (doc);
		      int i;

		      /* If LEN is too long, fold DOC.  */
		      if (len > MAX_LONG_DOC_LEN)
			{
			  /* Fold this line at the position of a space.  */
			  for (len = MAX_LONG_DOC_LEN; len > 0; len--)
			    if (doc[len - 1] == ' ')
			      break;
			}

		      grub_printf ("    ");
		      for (i = 0; i < len; i++)
			grub_putchar (*doc++);
		      grub_putchar ('\n');
		    }
		}
	    }

	  arg = next_arg;
	}
      while (*arg);
    }

  return 0;
}

static struct builtin builtin_help =
{
  "help",
  help_func,
  BUILTIN_CMDLINE,
  "help [PATTERN ...]",
  "Display helpful information about builtin commands."
};


/* hide */
static int
hide_func (char *arg, int flags)
{
  if (! set_device (arg))
    return 1;

  saved_partition = current_partition;
  saved_drive = current_drive;
  if (! set_partition_hidden_flag (1))
    return 1;

  return 0;
}

static struct builtin builtin_hide =
{
  "hide",
  hide_func,
  BUILTIN_CMDLINE | BUILTIN_MENU,
  "hide PARTITION",
  "Hide PARTITION by setting the \"hidden\" bit in"
  " its partition type code."
};


/* impsprobe */
static int
impsprobe_func (char *arg, int flags)
{
#ifndef GRUB_UTIL
  if (!imps_probe ())
#endif
    printf (" No MPS information found or probe failed\n");

  return 0;
}

static struct builtin builtin_impsprobe =
{
  "impsprobe",
  impsprobe_func,
  BUILTIN_CMDLINE,
  "impsprobe",
  "Probe the Intel Multiprocessor Specification 1.1 or 1.4"
  " configuration table and boot the various CPUs which are found into"
  " a tight loop."
};


/* initrd */
static int
initrd_func (char *arg, int flags)
{
  switch (kernel_type)
    {
    case KERNEL_TYPE_LINUX:
    case KERNEL_TYPE_BIG_LINUX:
      if (! load_initrd (arg))
	return 1;
      break;

    default:
      errnum = ERR_NEED_LX_KERNEL;
      return 1;
    }

  return 0;
}

static struct builtin builtin_initrd =
{
  "initrd",
  initrd_func,
  BUILTIN_CMDLINE,
  "initrd FILE [ARG ...]",
  "Load an initial ramdisk FILE for a Linux format boot image and set the"
  " appropriate parameters in the Linux setup area in memory."
};


/* install */
static int
install_func (char *arg, int flags)
{
  char *stage1_file, *dest_dev, *file, *addr;
  char buffer[SECTOR_SIZE], old_sect[SECTOR_SIZE];
  int new_drive = 0xFF;
  int dest_drive, dest_sector;
  int i;
  struct geometry dest_geom;
  int write_stage2_sect = 0;
  int stage2_sect;
  char *ptr;
  int installaddr, installlist, installsect;

  /* Write SECTOR to INSTALLLIST, and update INSTALLADDR and
     INSTALLSECT.  */
  static void debug_fs_blocklist_func (int sector)
    {
      if (debug)
	printf("[%d]", sector);

      if (*((unsigned long *) (installlist - 4))
	  + *((unsigned short *) installlist) != sector
	  || installlist == BOOTSEC_LOCATION + STAGE1_FIRSTLIST + 4)
	{
	  installlist -= 8;

	  if (*((unsigned long *) (installlist - 8)))
	    errnum = ERR_WONT_FIT;
	  else
	    {
	      *((unsigned short *) (installlist + 2)) = (installaddr >> 4);
	      *((unsigned long *) (installlist - 4)) = sector;
	    }
	}

      *((unsigned short *) installlist) += 1;
      installsect = sector;
      installaddr += 512;
    }

  stage1_file = arg;
  dest_dev = skip_to (0, stage1_file);
  if (*dest_dev == 'd')
    {
      new_drive = 0;
      dest_dev = skip_to (0, dest_dev);
    }
  file = skip_to (0, dest_dev);
  addr = skip_to (0, file);

  /* Get the installation address.  */
  if (! safe_parse_maxint (&addr, &installaddr))
    return 1;

  /* Read Stage 1.  */
  if (! grub_open (stage1_file)
      || ! grub_read (buffer, SECTOR_SIZE) == SECTOR_SIZE)
    return 1;

  /* Read the old sector from DEST_DEV.  */
  if (! set_device (dest_dev)
      || ! open_partition ()
      || ! devread (0, 0, SECTOR_SIZE, old_sect))
    return 1;

  /* Store the information for the destination device.  */
  dest_drive = current_drive;
  dest_geom = buf_geom;
  dest_sector = part_start;

#ifndef NO_DECOMPRESSION
  /* Do not decompress Stage 2.  */
  no_decompression = 1;
#endif

  /* copy possible DOS BPB, 59 bytes at byte offset 3 */
  grub_memmove (buffer + BOOTSEC_BPB_OFFSET,
		old_sect + BOOTSEC_BPB_OFFSET,
		BOOTSEC_BPB_LENGTH);

  /* if for a hard disk, copy possible MBR/extended part table */
  if ((dest_drive & 0x80) && current_partition == 0xFFFFFF)
    grub_memmove (buffer + BOOTSEC_PART_OFFSET,
		  old_sect + BOOTSEC_PART_OFFSET,
		  BOOTSEC_PART_LENGTH);

  /* Check for the version and the signature of Stage 1.  */
  if (*((short *)(buffer + STAGE1_VER_MAJ_OFFS)) != COMPAT_VERSION
      || (*((unsigned short *) (buffer + BOOTSEC_SIG_OFFSET))
	  != BOOTSEC_SIGNATURE)
      || (! (dest_drive & 0x80)
	  && (*((unsigned char *) (buffer + BOOTSEC_PART_OFFSET)) == 0x80
	      || buffer[BOOTSEC_PART_OFFSET] == 0))
      || (buffer[STAGE1_ID_OFFSET] != STAGE1_ID_LBA
	  && buffer[STAGE1_ID_OFFSET] != STAGE1_ID_CHS))
    {
      errnum = ERR_BAD_VERSION;
      return 1;
    }

  /* Open Stage 2.  */
  if (! grub_open (file))
    return 1;

  /* If STAGE1_FILE is the LBA version, do a sanity check.  */
  if (buffer[STAGE1_ID_OFFSET] == STAGE1_ID_LBA)
    {
      /* The geometry of the drive in which FILE is located.  */
      struct geometry load_geom;

      /* Check if CURRENT_DRIVE is a floppy disk.  */
      if (! (current_drive & 0x80))
	{
	  errnum = ERR_DEV_VALUES;
	  return 1;
	}

      /* Get the geometry of CURRENT_DRIVE.  */
      if (get_diskinfo (current_drive, &load_geom))
	{
	  errnum = ERR_NO_DISK;
	  return 1;
	}

#ifdef GRUB_UTIL
      /* XXX Can we determine if LBA is supported in
	 the grub shell as well?  */
      grub_printf ("Warning: make sure that the access mode for "
		   "(hd%d) is LBA.\n",
		   current_drive - 0x80);
#else
      /* Check if LBA is supported.  */
      if (! (load_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION))
	{
	  errnum = ERR_DEV_VALUES;
	  return 1;
	}
#endif
    }

  if (! new_drive)
    new_drive = current_drive;
  else if (current_drive != dest_drive)
    grub_printf ("Warning: the option `d' was not used, but the Stage 1 will"
		 "be installed on a\ndifferent drive than the drive where"
		 " the Stage 2 resides.\n");

  memmove ((char*) BOOTSEC_LOCATION, buffer, SECTOR_SIZE);

  *((unsigned char *) (BOOTSEC_LOCATION + STAGE1_FIRSTLIST))
    = new_drive;
  *((unsigned short *) (BOOTSEC_LOCATION + STAGE1_INSTALLADDR))
    = installaddr;

  i = BOOTSEC_LOCATION+STAGE1_FIRSTLIST - 4;
  while (*((unsigned long *) i))
    {
      if (i < BOOTSEC_LOCATION + STAGE1_FIRSTLIST - 256
	  || (*((int *) (i - 4)) & 0x80000000)
	  || *((unsigned short *) i) >= 0xA00
	  || *((short *) (i + 2)) == 0)
	{
	  errnum = ERR_BAD_VERSION;
	  return 1;
	}

      *((int *) i) = 0;
      *((int *) (i - 4)) = 0;
      i -= 8;
    }

  installlist = BOOTSEC_LOCATION + STAGE1_FIRSTLIST + 4;
  debug_fs = debug_fs_blocklist_func;

  /* Read the first sector of Stage 2.  */
  if (! grub_read ((char *) SCRATCHADDR, SECTOR_SIZE) == SECTOR_SIZE)
    {
      debug_fs = 0;
      return 1;
    }

  /* Check for the version of Stage 2.  */
  if (*((short *) (SCRATCHADDR + STAGE2_VER_MAJ_OFFS)) != COMPAT_VERSION)
    {
      errnum = ERR_BAD_VERSION;
      debug_fs = 0;
      return 1;
    }

  stage2_sect = installsect;
  ptr = skip_to (0, addr);

  if (*ptr == 'p')
    {
      write_stage2_sect = 1;
      *((long *) (SCRATCHADDR + STAGE2_INSTALLPART)) = current_partition;
      ptr = skip_to (0, ptr);
    }

  if (*ptr)
    {
      char *str	= ((char *) (SCRATCHADDR + STAGE2_VER_STR_OFFS));

      write_stage2_sect = 1;
      /* Find a string for the configuration filename.  */
      while (*(str++))
	;
      /* Copy the filename to Stage 2.  */
      while ((*(str++) = *(ptr++)) != 0)
	;
    }

  /* Read the whole of Stage 2.  */
  if (! grub_read ((char *) RAW_ADDR (0x100000), -1))
    {
      debug_fs = 0;
      return 1;
    }

  /* Clear the cache.  */
  buf_track = -1;

  if (biosdisk (BIOSDISK_WRITE,
		dest_drive, &dest_geom,
		dest_sector, 1, (BOOTSEC_LOCATION >> 4))
      || (write_stage2_sect
	  && biosdisk (BIOSDISK_WRITE,
		       current_drive, &buf_geom,
		       stage2_sect, 1, SCRATCHSEG)))
    {
      errnum = ERR_WRITE;
      debug_fs = 0;
      return 1;
    }

  debug_fs = 0;

#ifndef NO_DECOMPRESSION
  no_decompression = 0;
#endif

  return 0;
}

static struct builtin builtin_install =
{
  "install",
  install_func,
  BUILTIN_CMDLINE,
  "install STAGE1 [d] DEVICE STAGE2 ADDR [p] [CONFIG_FILE]",
  "Install STAGE1 on DEVICE, and install a blocklist for loading STAGE2"
  " as a Stage 2. If the option `d' is present, the Stage 1 will always"
  " look for the disk where STAGE2 was installed, rather than using"
  " the booting drive. The Stage 2 will be loaded at address ADDR, which"
  " must be 0x8000 for a true Stage 2, and 0x2000 for a Stage 1.5. If"
  " the option `p' or CONFIG_FILE is present, then the first block"
  " of Stage 2 is patched with new values of the partition and name"
  " of the configuration file used by the true Stage 2 (for a Stage 1.5,"
  " this is the name of the true Stage 2) at boot time."
};


/* kernel */
static int
kernel_func (char *arg, int flags)
{
  int len = grub_strlen (arg);

  /* Reset MB_CMDLINE.  */
  mb_cmdline = (char *) MB_CMDLINE_BUF;
  if (len + 1 > MB_CMDLINE_BUFLEN)
    {
      errnum = ERR_WONT_FIT;
      return 1;
    }

  /* Copy the command-line to MB_CMDLINE.  */
  grub_memmove (mb_cmdline, arg, len + 1);
  kernel_type = load_image (arg, mb_cmdline);
  if (kernel_type == KERNEL_TYPE_NONE)
    return 1;

  mb_cmdline += len + 1;
  return 0;
}

static struct builtin builtin_kernel =
{
  "kernel",
  kernel_func,
  BUILTIN_CMDLINE,
  "kernel FILE [ARG ...]",
  "Attempt to load the primary boot image (Multiboot a.out or ELF,"
  " Linux zImage or bzImage, FreeBSD a.out, or NetBSD a.out) from"
  " FILE. The rest of the line is passed verbatim as the"
  " \"kernel command line\".  Any modules must be reloaded after"
  " using this command."
};


/* makeactive */
static int
makeactive_func (char *arg, int flags)
{
  if (! make_saved_active ())
    return 1;

  return 0;
}

static struct builtin builtin_makeactive =
{
  "makeactive",
  makeactive_func,
  BUILTIN_CMDLINE,
  "makeactive",
  "Set the active partition on the root disk to GRUB's root partition."
  " This command is limited to _primary_ PC partitions on a hard disk."
};


/* module */
static int
module_func (char *arg, int flags)
{
  int len = grub_strlen (arg);

  switch (kernel_type)
    {
    case KERNEL_TYPE_MULTIBOOT:
      if (mb_cmdline + len + 1 > (char *) MB_CMDLINE_BUF + MB_CMDLINE_BUFLEN)
	{
	  errnum = ERR_WONT_FIT;
	  return 1;
	}
      grub_memmove (mb_cmdline, arg, len + 1);
      if (! load_module (arg, mb_cmdline))
	return 1;
      mb_cmdline += len + 1;
      break;

    case KERNEL_TYPE_LINUX:
    case KERNEL_TYPE_BIG_LINUX:
      if (! load_initrd (arg))
	return 1;
      break;

    default:
      errnum = ERR_NEED_MB_KERNEL;
      return 1;
    }

  return 0;
}

static struct builtin builtin_module =
{
  "module",
  module_func,
  BUILTIN_CMDLINE,
  "module FILE [ARG ...]",
  "Load a boot module FILE for a Multiboot format boot image (no"
  " interpretation of the file contents is made, so users of this"
  " command must know what the kernel in question expects). The"
  " rest of the line is passed as the \"module command line\", like"
  " the `kernel' command."
};


/* modulenounzip */
static int
modulenounzip_func (char *arg, int flags)
{
  int ret;

#ifndef NO_DECOMPRESSION
  no_decompression = 1;
#endif

  ret = module_func (arg, flags);

#ifndef NO_DECOMPRESSION
  no_decompression = 0;
#endif

  return ret;
}

static struct builtin builtin_modulenounzip =
{
  "modulenounzip",
  modulenounzip_func,
  BUILTIN_CMDLINE,
  "modulenounzip FILE [ARG ...]",
  "The same as `module', except that automatic decompression is"
  " disabled."
};


/* password */
static int
password_func (char *arg, int flags)
{
  int len = grub_strlen (arg);

  if (len > PASSWORD_BUFLEN)
    {
      errnum = ERR_WONT_FIT;
      return 1;
    }

  password = (char *) PASSWORD_BUF;
  grub_memmove (password, arg, len + 1);
  return 0;
}

static struct builtin builtin_password =
{
  "password",
  password_func,
  BUILTIN_MENU,
#if 0
  "password PASSWD FILE",
  "Disable all interactive editing control (menu entry editor and"
  " command line). If the password PASSWD is entered, it loads the"
  " FILE as a new config file and restarts the GRUB Stage 2."
#endif
};


/* pause */
static int
pause_func (char *arg, int flags)
{
  /* If ESC is returned, then abort this entry.  */
  if (ASCII_CHAR (getkey ()) == 27)
    return 1;

  return 0;
}

static struct builtin builtin_pause =
{
  "pause",
  pause_func,
  BUILTIN_CMDLINE,
  "pause [MESSAGE ...]",
  "Print MESSAGE, then wait until a key is pressed."
};


/* quit */
static int
quit_func (char *arg, int flags)
{
#ifdef GRUB_UTIL
  stop ();
#endif

  /* In Stage 2, just ignore this command.  */
  return 0;
}

static struct builtin builtin_quit =
{
  "quit",
  quit_func,
  BUILTIN_CMDLINE,
  "quit",
  "Exit from the GRUB shell."
};


static int
read_func (char *arg, int flags)
{
  int addr;

  if (! safe_parse_maxint (&arg, &addr))
    return 1;

  grub_printf ("Address 0x%x: Value 0x%x\n",
	       addr, *((unsigned *) RAW_ADDR (addr)));
  return 0;
}

static struct builtin builtin_read =
{
  "read",
  read_func,
  BUILTIN_CMDLINE,
  "read ADDR",
  "Read a 32-bit value from memory at address ADDR and"
  " display it in hex format."
};


static int
root_func (char *arg, int flags)
{
  int hdbias = 0;
  char *biasptr;
  char *next;

  /* Call set_device to get the drive and the partition in ARG.  */
  next = set_device (arg);
  if (! next)
    return 1;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (! open_device () && errnum != ERR_FSYS_MOUNT)
    return 1;

  /* Clear ERRNUM.  */
  errnum = 0;
  saved_partition = current_partition;
  saved_drive = current_drive;

  /* BSD and chainloading evil hacks !!  */
  biasptr = skip_to (0, next);
  safe_parse_maxint (&biasptr, &hdbias);
  errnum = 0;
  bootdev = set_bootdev (hdbias);

  /* Print the type of the filesystem.  */
  print_fsys_type ();

  return 0;
}

static struct builtin builtin_root =
{
  "root",
  root_func,
  BUILTIN_CMDLINE,
  "root DEVICE [HDBIAS]",
  "Set the current \"root partition\" to the device DEVICE, then"
  " attempt to mount it to get the partition size (for passing the"
  " partition descriptor in `ES:ESI', used by some chain-loaded"
  " bootloaders), the BSD drive-type (for booting BSD kernels using"
  " their native boot format), and correctly determine "
  " the PC partition where a BSD sub-partition is located. The"
  " optional HDBIAS parameter is a number to tell a BSD kernel"
  " how many BIOS drive numbers are on controllers before the current"
  " one. For example, if there is an IDE disk and a SCSI disk, and your"
  " FreeBSD root partition is on the SCSI disk, then use a `1' for HDBIAS."
};


/* rootnoverify */
static int
rootnoverify_func (char *arg, int flags)
{
  if (! set_device (arg))
    return 1;

  saved_partition = current_partition;
  saved_drive = current_drive;
  current_drive = -1;
  return 0;
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
  BUILTIN_CMDLINE,
  "rootnoverify DEVICE [HDBIAS]",
  "Similar to `root', but don't attempt to mount the partition. This"
  " is useful for when an OS is outside of the area of the disk that"
  " GRUB can read, but setting the correct root partition is still"
  " desired. Note that the items mentioned in `root' which"
  " derived from attempting the mount will NOT work correctly."
};


/* testload */
static int
testload_func (char *arg, int flags)
{
  int i;

  kernel_type = KERNEL_TYPE_NONE;

  if (! grub_open (arg))
    return 1;

  debug_fs = debug_fs_print_func;

  /* Perform filesystem test on the specified file.  */
  /* Read whole file first. */
  grub_printf ("Whole file: ");

  grub_read ((char *) RAW_ADDR (0x100000), -1);

  /* Now compare two sections of the file read differently.  */

  for (i = 0; i < 0x10ac0; i++)
    {
      *((unsigned char *) RAW_ADDR (0x200000 + i)) = 0;
      *((unsigned char *) RAW_ADDR (0x300000 + i)) = 1;
    }

  /* First partial read.  */
  grub_printf ("\nPartial read 1: ");

  filepos = 0;
  grub_read ((char *) RAW_ADDR (0x200000), 0x7);
  grub_read ((char *) RAW_ADDR (0x200007), 0x100);
  grub_read ((char *) RAW_ADDR (0x200107), 0x10);
  grub_read ((char *) RAW_ADDR (0x200117), 0x999);
  grub_read ((char *) RAW_ADDR (0x200ab0), 0x10);
  grub_read ((char *) RAW_ADDR (0x200ac0), 0x10000);

  /* Second partial read.  */
  grub_printf ("\nPartial read 2: ");

  filepos = 0;
  grub_read ((char *) RAW_ADDR (0x300000), 0x10000);
  grub_read ((char *) RAW_ADDR (0x310000), 0x10);
  grub_read ((char *) RAW_ADDR (0x310010), 0x7);
  grub_read ((char *) RAW_ADDR (0x310017), 0x10);
  grub_read ((char *) RAW_ADDR (0x310027), 0x999);
  grub_read ((char *) RAW_ADDR (0x3109c0), 0x100);

  grub_printf ("\nHeader1 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
	       *((int *) RAW_ADDR (0x200000)),
	       *((int *) RAW_ADDR (0x200004)),
	       *((int *) RAW_ADDR (0x200008)),
	       *((int *) RAW_ADDR (0x20000c)));

  grub_printf ("Header2 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
	       *((int *) RAW_ADDR (0x300000)),
	       *((int *) RAW_ADDR (0x300004)),
	       *((int *) RAW_ADDR (0x300008)),
	       *((int *) RAW_ADDR (0x30000c)));

  for (i = 0; i < 0x10ac0; i++)
    if (*((unsigned char *) RAW_ADDR (0x200000 + i))
	!= *((unsigned char *) RAW_ADDR (0x300000 + i)))
      break;

  grub_printf ("Max is 0x10ac0: i=0x%x, filepos=0x%x\n", i, filepos);
  debug_fs = 0;
  return 0;
}

static struct builtin builtin_testload =
{
  "testload",
  testload_func,
  BUILTIN_CMDLINE,
  "testload FILE",
  "Read the entire contents of FILE in several different ways and"
  " compares them, to test the filesystem code. The output is somewhat"
  " cryptic, but if no errors are reported and the final `i=X,"
  " filepos=Y' reading has X and Y equal, then it is definitely"
  " consistent, and very likely works correctly subject to a"
  " consistent offset error. If this test succeeds, then a good next"
  " step is to try loading a kernel."
};


/* timeout */
static int
timeout_func (char *arg, int flags)
{
  if (! safe_parse_maxint (&arg, &grub_timeout))
    return 1;

  return 0;
}

static struct builtin builtin_timeout =
{
  "timeout",
  timeout_func,
  BUILTIN_MENU,
#if 0
  "timeout SEC",
  "Set a timeout, in SEC seconds, before automatically booting the"
  " default entry (normally the first entry defined)."
#endif
};


/* title */
static int
title_func (char *arg, int flags)
{
  /* This function is not actually used at least currently.  */
  return 0;
}

static struct builtin builtin_title =
{
  "title",
  title_func,
  BUILTIN_TITLE,
#if 0
  "title [NAME ...]",
  "Start a new boot entry, and set its name to the contents of the"
  " rest of the line, starting with the first non-space character."
#endif
};


/* unhide */
static int
unhide_func (char *arg, int flags)
{
  if (! set_device (arg))
    return 1;

  saved_partition = current_partition;
  saved_drive = current_drive;
  if (! set_partition_hidden_flag (0))
    return 1;

  return 0;
}

static struct builtin builtin_unhide =
{
  "unhide",
  unhide_func,
  BUILTIN_CMDLINE | BUILTIN_MENU,
  "unhide PARTITION",
  "Unhide PARTITION by clearing the \"hidden\" bit in its"
  " partition type code."
};


/* uppermem */
static int
uppermem_func (char *arg, int flags)
{
  if (! safe_parse_maxint (&arg, (int *) &mbi.mem_upper))
    return 1;

  mbi.flags &= ~MB_INFO_MEM_MAP;
  return 0;
}

static struct builtin builtin_uppermem =
{
  "uppermem",
  uppermem_func,
  BUILTIN_CMDLINE,
  "uppermem KBYTES",
  "Force GRUB to assume that only KBYTES kilobytes of upper memory are"
  " installed.  Any system address range maps are discarded."
};


/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
  &builtin_boot,
  &builtin_chainloader,
  &builtin_color,
  &builtin_configfile,
  &builtin_debug,
  &builtin_default,
  &builtin_device,
  &builtin_displaymem,
  &builtin_fallback,
  &builtin_fstest,
  &builtin_geometry,
  &builtin_help,
  &builtin_hide,
  &builtin_impsprobe,
  &builtin_initrd,
  &builtin_install,
  &builtin_kernel,
  &builtin_makeactive,
  &builtin_module,
  &builtin_modulenounzip,
  &builtin_password,
  &builtin_pause,
  &builtin_quit,
  &builtin_read,
  &builtin_root,
  &builtin_rootnoverify,
  &builtin_testload,
  &builtin_timeout,
  &builtin_title,
  &builtin_unhide,
  &builtin_uppermem,
  0
};
