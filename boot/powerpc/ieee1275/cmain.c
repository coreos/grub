/* cmain.c - Startup code for the PowerPC.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

#include "pupa/machine/ieee1275.h"
#include "pupa/kernel.h"

struct module_info
{
  uint32_t start;
  uint32_t end;
};

#define roundup(a, s) (((a) + ((s) - 1)) & ~((s) - 1))

/* OpenFirmware entry point passed to us from the real bootloader.  */
intptr_t (*pupa_ieee1275_entry_fn) (void *);


/* Return a help text for this architecture.  */
const char *
help_arch (void)
{
  /* No special information.  */
  return "";
}


/* Setup the argument vector and pass control over to the main
   function.  */
void
cmain (uint32_t firmware_entry)
{
  char **argv, args[256];
  pupa_ieee1275_phandle_t chosen;
  int argc = 0, actual;
  long batl, batu;

  pupa_ieee1275_entry_fn = (intptr_t (*)(void *)) firmware_entry;

  /* Initialize BAT registers to unmapped to not generate overlapping
     mappings below.  */
  asm volatile ("mtibatu 0,%0" :: "r"(0));
  asm volatile ("mtibatu 1,%0" :: "r"(0));
  asm volatile ("mtibatu 2,%0" :: "r"(0));
  asm volatile ("mtibatu 3,%0" :: "r"(0));
  asm volatile ("mtdbatu 0,%0" :: "r"(0));
  asm volatile ("mtdbatu 1,%0" :: "r"(0));
  asm volatile ("mtdbatu 2,%0" :: "r"(0));
  asm volatile ("mtdbatu 3,%0" :: "r"(0));

  /* Set up initial BAT table to only map the lowest 256 MB area.  */
  batl = 0x00000000 | 0x10   | 0x02;
  batu = 0x00000000 | 0x1ffc | 0x02;

  /* IBAT0 used for initial 256 MB segment */
  asm volatile ("mtibatl 3,%0; mtibatu 3,%1" :: "r" (batl), "r" (batu));
  /* DBAT0 used similar */
  asm volatile ("mtdbatl 3,%0; mtdbatu 3,%1" :: "r" (batl), "r" (batu));
  asm ("isync");

  /* If any argument was passed to the kernel (us), they are
     put in the bootargs property of /chosen.  The string can
     be null (just the nul-character), so check that the size
     is actually greater than one.  */

  pupa_ieee1275_finddevice ("/chosen", &chosen);
  if (pupa_ieee1275_get_property (chosen, "bootargs", args,
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
      argv[0] = "pupa";

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
      argv[0] = "pupa";
      argv[1] = 0;
      argc = 1;
    }
  /* Now invoke the main function.  */
  /* XXX: pupa_main does not parse arguments yet.  */
  pupa_main ();

  /* Never reached.  */
}
