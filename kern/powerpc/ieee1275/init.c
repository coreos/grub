/*  init.c -- Initialize GRUB on the newworld mac (PPC).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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

/* XXX: Modules are not yet supported.  */
grub_addr_t grub_end_addr = -1;
grub_addr_t grub_total_module_size = 0;

void
abort (void)
{
  /* Trap to Open Firmware.  */
  asm ("trap");
  
  for (;;);
}

void
grub_machine_init (void)
{
  extern char _start;
  grub_addr_t heap_start;
  grub_addr_t heap_len;

  grub_console_init ();

  /* Apple OF 1.0.5 reserves 0x4000 bytes for the exception handlers.  */
  heap_start = 0x4000;
  /* Apple OF 3.1.1 reserves an extra 0x1000 bytes below the load address
     of an ELF file.  */
  heap_len = (grub_addr_t) &_start - 0x1000 - heap_start;

  if (grub_ieee1275_claim (heap_start, heap_len, 0, 0))
    {
      grub_printf ("Failed to claim heap at 0x%x, len 0x%x\n", heap_start,
		   heap_len);
      abort ();
    }
  grub_mm_init_region ((void *) heap_start, heap_len);

  grub_env_set ("prefix", "");

  grub_ofdisk_init ();
}

void
grub_stop (void)
{
  for (;;);
}

grub_uint32_t
grub_get_rtc (void)
{
  return 0;
}
