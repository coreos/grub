/*  init.c -- Initialize GRUB on the newworld mac (PPC).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/machine/ieee1275.h>
#include <grub/normal.h>
#include <grub/fs.h>
#include <grub/setjmp.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/machine/init.h>
#include <grub/machine/time.h>
#include <grub/machine/kernel.h>

/* Apple OF 1.0.5 reserves 0x0 to 0x4000 for the exception handlers.  */
static const grub_addr_t grub_heap_start = 0x4000;
static grub_addr_t grub_heap_len;

void
abort (void)
{
  /* Trap to Open Firmware.  */
  asm ("trap");
  
  for (;;);
}

/* Translate an OF filesystem path (separated by backslashes), into a GRUB
   path (separated by forward slashes).  */
static void
grub_translate_ieee1275_path (char *filepath)
{
  char *backslash;

  backslash = grub_strchr (filepath, '\\');
  while (backslash != 0)
    {
      *backslash = '/';
      backslash = grub_strchr (filepath, '\\');
    }
}

static void
grub_set_prefix (void)
{
  char bootpath[64]; /* XXX check length */
  char *filename;
  char *prefix;
  grub_ieee1275_phandle_t chosen;

  grub_ieee1275_finddevice ("/chosen", &chosen);
  if (grub_ieee1275_get_property (chosen, "bootpath", &bootpath,
				  sizeof (bootpath), 0))
    { 
      /* Should never happen.  */
      grub_printf ("/chosen/bootpath property missing!\n");
      grub_env_set ("prefix", "");
      return;
    }

  /* Transform an OF device path to a GRUB path.  */

  prefix = grub_ieee1275_encode_devname (bootpath);

  filename = grub_ieee1275_get_filename (bootpath);
  if (filename)
    {
      char *newprefix;
      char *lastslash = grub_strrchr (filename, '\\');

      /* Truncate at last directory.  */
      if (lastslash)
        {
	  *lastslash = '\0';
	  grub_translate_ieee1275_path (filename);

	  newprefix = grub_malloc (grub_strlen (prefix)
				   + grub_strlen (filename));
	  grub_sprintf (newprefix, "%s%s", prefix, filename);
	  grub_free (prefix);
	  prefix = newprefix;
	}
    }

  grub_env_set ("prefix", prefix);

  grub_free (filename);
  grub_free (prefix);
}

void
grub_machine_init (void)
{
  extern char _start;

  grub_console_init ();

  /* Apple OF 3.1.1 reserves an extra 0x1000 bytes below the load address
     of an ELF file.  */
  grub_heap_len = (grub_addr_t) &_start - 0x1000 - grub_heap_start;

  if (grub_ieee1275_claim (grub_heap_start, grub_heap_len, 0, 0))
    {
      grub_printf ("Failed to claim heap at 0x%x, len 0x%x\n", grub_heap_start,
		   grub_heap_len);
      abort ();
    }
  grub_mm_init_region ((void *) grub_heap_start, grub_heap_len);

  grub_set_prefix ();

  grub_ofdisk_init ();
}

void
grub_machine_fini (void)
{
  grub_ofdisk_fini ();
  grub_console_fini ();

  grub_ieee1275_release (grub_heap_start, grub_heap_len);
  /* XXX Release memory claimed for Old World firmware.  */
}

void
grub_stop (void)
{
  for (;;);
}

grub_uint32_t
grub_get_rtc (void)
{
  grub_uint32_t msecs;

  if (grub_ieee1275_milliseconds (&msecs))
    return 0;

  return msecs;
}

grub_addr_t
grub_arch_modules_addr (void)
{
  return GRUB_IEEE1275_MODULE_BASE;
}
