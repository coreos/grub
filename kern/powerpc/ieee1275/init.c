/*  init.c -- Initialize PUPA on the newworld mac (PPC).  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
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

#include <pupa/kernel.h>
#include <pupa/dl.h>
#include <pupa/disk.h>
#include <pupa/mm.h>
#include <pupa/machine/partition.h>
#include <pupa/machine/ieee1275.h>
#include <pupa/normal.h>
#include <pupa/fs.h>
#include <pupa/setjmp.h>
#include <env.h>

void pupa_ofdisk_init (void);
void pupa_console_init (void);


/* XXX: Modules are not yet supported.  */
pupa_addr_t pupa_end_addr = -1;
pupa_addr_t pupa_total_module_size = 0;

void
abort (void)
{
  for (;;);
}

void
pupa_machine_init (void)
{
  void *mem;

  if (pupa_ieee1275_claim ((void *) 0x300000, 0x150000, 0, &mem) == -1)
    abort (); /* Damn, we are in trouble!  */
  
  /* The memory allocations were copied from yaboot.  */
  pupa_mm_init_region ((void *) 0x300000, 0x150000);

  /* XXX: Loadable modules are not supported.  */
  pupa_env_set ("prefix", "");

  pupa_ext2_init ();
  pupa_ofdisk_init ();
  pupa_console_init ();
}

int
pupa_arch_dl_check_header (void *ehdr __attribute ((unused)),
			   pupa_size_t size __attribute ((unused)))
{
  return 0;
}

pupa_err_t
pupa_arch_dl_relocate_symbols (pupa_dl_t mod __attribute ((unused)),
			       void *ehdr __attribute ((unused)))
{
  return 0;
}

void
pupa_stop (void)
{
  for (;;);
}

void
pupa_register_exported_symbols (void)
{
}

pupa_uint32_t
pupa_get_rtc (void)
{
  return 0;
}

int
pupa_setjmp (pupa_jmp_buf env __attribute ((unused)))
{
  return 0;
}

void
pupa_longjmp (pupa_jmp_buf env __attribute ((unused)),
	      int val __attribute ((unused)))
{
}
