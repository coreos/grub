/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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

#ifndef PUPA_ENV_HEADER
#define PUPA_ENV_HEADER	1

#include <pupa/symbol.h>
#include <pupa/err.h>
#include <pupa/types.h>

struct pupa_env_var
{
  char *name;
  char *value;
  pupa_err_t (*read_hook) (struct pupa_env_var *var, char **val);
  pupa_err_t (*write_hook) (struct pupa_env_var *var);
  struct pupa_env_var *next;
  struct pupa_env_var **prevp;
  struct pupa_env_var *sort_next;
  struct pupa_env_var **sort_prevp;
};

pupa_err_t EXPORT_FUNC(pupa_env_set) (const char *var, const char *val);
char *EXPORT_FUNC(pupa_env_get) (const char *name);
void EXPORT_FUNC(pupa_env_unset) (const char *name);
void EXPORT_FUNC(pupa_env_iterate) (int (* func) (struct pupa_env_var *var));
pupa_err_t EXPORT_FUNC(pupa_register_variable_hook) (const char *var,
						     pupa_err_t (*read_hook) 
						     (struct pupa_env_var *var, char **val),
						     pupa_err_t (*write_hook)
						     (struct pupa_env_var *var));

#endif /* ! PUPA_ENV_HEADER */
