/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/loader.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>

static pupa_err_t (*pupa_loader_load_module_func) (int argc, char *argv[]);
static pupa_err_t (*pupa_loader_boot_func) (void);
static pupa_err_t (*pupa_loader_unload_func) (void);

static int pupa_loader_loaded;

void
pupa_loader_set (pupa_err_t (*load_module) (int argc, char *argv[]),
		 pupa_err_t (*boot) (void),
		 pupa_err_t (*unload) (void))
{
  if (pupa_loader_loaded && pupa_loader_unload_func)
    if (pupa_loader_unload_func () != PUPA_ERR_NONE)
      return;
  
  pupa_loader_load_module_func = load_module;
  pupa_loader_boot_func = boot;
  pupa_loader_unload_func = unload;

  pupa_loader_loaded = 1;
}

pupa_err_t
pupa_loader_load_module (int argc, char *argv[])
{
  if (! pupa_loader_loaded)
    return pupa_error (PUPA_ERR_NO_KERNEL, "no loaded kernel");

  if (! pupa_loader_load_module_func)
    return pupa_error (PUPA_ERR_BAD_OS, "module not supported");

  return pupa_loader_load_module_func (argc, argv);
}

pupa_err_t
pupa_loader_boot (void)
{
  if (! pupa_loader_loaded)
    return pupa_error (PUPA_ERR_NO_KERNEL, "no loaded kernel");

  return (pupa_loader_boot_func) ();
}

