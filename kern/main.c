/* main.c - the kernel main routine */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
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
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/symbol.h>
#include <pupa/dl.h>
#include <pupa/term.h>
#include <pupa/rescue.h>
#include <pupa/file.h>
#include <pupa/device.h>

/* Return the end of the core image.  */
pupa_addr_t
pupa_get_end_addr (void)
{
  return pupa_total_module_size + pupa_end_addr;
}

/* Load all modules in core.  */
static void
pupa_load_modules (void)
{
  struct pupa_module_header *header;

  for (header = (struct pupa_module_header *) pupa_end_addr;
       header < (struct pupa_module_header *) pupa_get_end_addr ();
       header = (struct pupa_module_header *) ((char *) header + header->size))
    {
      if (! pupa_dl_load_core ((char *) header + header->offset,
			       (header->size - header->offset)))
	pupa_fatal ("%s", pupa_errmsg);
    }
}

/* Add the region where modules reside into dynamic memory.  */
static void
pupa_add_unused_region (void)
{
  if (pupa_total_module_size)
    pupa_mm_init_region ((void *) pupa_end_addr, pupa_total_module_size);
}

/* Set the root device according to the dl prefix.  */
static void
pupa_set_root_dev (void)
{
  const char *prefix;

  prefix = pupa_dl_get_prefix ();
  
  if (prefix)
    {
      char *dev;

      dev = pupa_file_get_device_name (prefix);
      if (dev)
	{
	  pupa_device_set_root (dev);
	  pupa_free (dev);
	}
    }
}

/* Load the normal mode module and execute the normal mode if possible.  */
static void
pupa_load_normal_mode (void)
{
  /* Load the module.  */
  pupa_dl_load ("normal");
  
  /* Ignore any error, because we have the rescue mode anyway.  */
  pupa_errno = PUPA_ERR_NONE;
}

/* The main routine.  */
void
pupa_main (void)
{
  /* First of all, initialize the machine.  */
  pupa_machine_init ();

  /* Hello.  */
  pupa_setcolorstate (PUPA_TERM_COLOR_HIGHLIGHT);
  pupa_printf ("Welcome to PUPA!\n\n");
  pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  pupa_set_root_dev ();

  /* Load pre-loaded modules and free the space.  */
  pupa_register_exported_symbols ();
  pupa_load_modules ();
  pupa_add_unused_region ();

  /* Load the normal mode module.  */
  pupa_load_normal_mode ();
  
  /* Enter the rescue mode.  */
  pupa_enter_rescue_mode ();
}
