/* cmain.c - Startup code for the PowerPC.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.
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

#include <alloca.h>
#include <stdint.h>

#include <grub/machine/ieee1275.h>
#include <grub/kernel.h>
#include <grub/machine/init.h>

struct module_info
{
  uint32_t start;
  uint32_t end;
};

#define roundup(a, s) (((a) + ((s) - 1)) & ~((s) - 1))

/* OpenFirmware entry point passed to us from the real bootloader.  */
intptr_t (*grub_ieee1275_entry_fn) (void *);

grub_uint32_t grub_ieee1275_flags;
int grub_ieee1275_realmode;



static void
find_options (void)
{
  grub_ieee1275_phandle_t options;

  grub_ieee1275_finddevice ("/options", &options);
  grub_ieee1275_get_property (options, "real-mode?", &grub_ieee1275_realmode,
			      sizeof (grub_ieee1275_realmode), 0);
}

/* Setup the argument vector and pass control over to the main
   function.  */
void
cmain (uint32_t r3, uint32_t r4 __attribute__((unused)), uint32_t r5)
{
  char **argv, args[256];
  grub_ieee1275_phandle_t chosen;
  int argc = 0, actual;

  if (r5 == 0xdeadbeef)
    {
      /* Entered from Old World stage1.  */
      extern char _start;
      extern char _end;

      grub_ieee1275_entry_fn = (intptr_t (*)(void *)) r3;

      grub_ieee1275_flags = GRUB_IEEE1275_NO_PARTITION_0;

      /* Old World Open Firmware may use 4M-5M without claiming it.  */
      grub_ieee1275_claim (0x00400000, 0x00100000, 0, 0);

      /* Need to claim ourselves so we don't cannibalize our memory later.  */
      if (grub_ieee1275_claim ((grub_addr_t) &_start, (grub_addr_t) &_end
          - (grub_addr_t) &_start, 0, 0))
	abort();
    }
  else
    {
      /* Assume we were entered from Open Firmware.  */
      grub_ieee1275_entry_fn = (intptr_t (*)(void *)) r5;
    }

  find_options ();

  /* If any argument was passed to the kernel (us), they are
     put in the bootargs property of /chosen.  The string can
     be null (just the nul-character), so check that the size
     is actually greater than one.  */

  grub_ieee1275_finddevice ("/chosen", &chosen);
  if (grub_ieee1275_get_property (chosen, "bootargs", args,
				  sizeof args, &actual) == 0
      && actual > 1)
    {
      /* A command line was passed.  */
      char *str = args;
      int nr = 1;

      /* First time around we count the number of arguments.  */
      argc = 2;
      while (*str && *str == ' ')
	str++;

      while (*str)
	if (*(str++) == ' ')
	  {
	    while (*str && *str == ' ')
	      str++;
	    if (*str)
	      argc++;
	  }
      argv = alloca (sizeof (char *) * (argc + 2));

      /* The bootargs property does not contain the program
	 name, just the arguments.  */
      argv[0] = "grub";

      /* Second time around we fill in the argv.  */
      str = args;

      while (*str && *str == ' ')
	str++;
      argv[nr++] = str;

      while (*str)
	{
	  if (*str == ' ')
	    {
	      *(str++) = '\0';
	      while (*str && *str == ' ')
		str++;
	      if (*str)
		argv[nr++] = str;
	    }
	  else
	    str++;
	}
      argv[nr] = 0;
    }
  else
    {
      argv = alloca (sizeof (char *) * 2);
      argv[0] = "grub";
      argv[1] = 0;
      argc = 1;
    }
  /* Now invoke the main function.  */
  /* XXX: grub_main does not parse arguments yet.  */
  grub_main ();

  /* Never reached.  */
}
