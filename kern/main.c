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

/* The main routine.  */
void
pupa_main (void)
{
  void (*normal_func) (void);
  
  /* First of all, initialize the machine.  */
  pupa_machine_init ();

  /* Hello.  */
  pupa_setcolorstate (PUPA_TERM_COLOR_HIGHLIGHT);
  pupa_printf ("Welcome to PUPA!");
  pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);
  pupa_printf ("\n\n");

  pupa_register_exported_symbols ();
  pupa_load_modules ();
  pupa_add_unused_region ();

  /* If the function pupa_enter_normal_mode is present, call it.  */
  normal_func = pupa_dl_resolve_symbol ("pupa_enter_normal_mode");
  if (normal_func)
    (*normal_func) ();

  /* If pupa_enter_normal_mode fails or doesn't exist, enter rescue mode.  */
  pupa_printf ("Entering into rescue mode...\n");
  pupa_enter_rescue_mode ();
}
