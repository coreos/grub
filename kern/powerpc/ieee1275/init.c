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
#include <grub/machine/partition.h>
#include <grub/machine/ieee1275.h>
#include <grub/normal.h>
#include <grub/fs.h>
#include <grub/setjmp.h>
#include <env.h>

void grub_ofdisk_init (void);
void grub_console_init (void);


/* XXX: Modules are not yet supported.  */
grub_addr_t grub_end_addr = -1;
grub_addr_t grub_total_module_size = 0;

void
abort (void)
{
  for (;;);
}

void
grub_machine_init (void)
{
  void *mem;

  if (grub_ieee1275_claim ((void *) 0x300000, 0x150000, 0, &mem) == -1)
    abort (); /* Damn, we are in trouble!  */
  
  /* The memory allocations were copied from yaboot.  */
  grub_mm_init_region ((void *) 0x300000, 0x150000);

  /* XXX: Loadable modules are not supported.  */
  grub_env_set ("prefix", "");

  grub_ext2_init ();
  grub_ofdisk_init ();
  grub_console_init ();
}

int
grub_arch_dl_check_header (void *ehdr __attribute ((unused)),
			   grub_size_t size __attribute ((unused)))
{
  return 0;
}

grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod __attribute ((unused)),
			       void *ehdr __attribute ((unused)))
{
  return 0;
}

void
grub_stop (void)
{
  for (;;);
}

void
grub_register_exported_symbols (void)
{
}

grub_uint32_t
grub_get_rtc (void)
{
  return 0;
}

int
grub_setjmp (grub_jmp_buf env __attribute ((unused)))
{
  return 0;
}

void
grub_longjmp (grub_jmp_buf env __attribute ((unused)),
	      int val __attribute ((unused)))
{
}
