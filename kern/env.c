/* env.c - Environment variables */
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

#include <pupa/env.h>
#include <pupa/misc.h>
#include <pupa/mm.h>

/* XXX: What would be a good size for the hashtable?  */
#define	HASHSZ	123

/* A hashtable for quick lookup of variables.  */
static struct pupa_env_var *pupa_env[HASHSZ];

/* The variables in a sorted list.  */
static struct pupa_env_var *pupa_env_sorted;

/* Return the hash representation of the string S.  */
static unsigned int pupa_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more effecient.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
}

static struct pupa_env_var *
pupa_env_find (const char *name)
{
  struct pupa_env_var *var;
  int idx = pupa_env_hashval (name);

  for (var = pupa_env[idx]; var; var = var->next)
    if (! pupa_strcmp (var->name, name))
      return var;
  return 0;
}

pupa_err_t
pupa_env_set (const char *var, const char *val)
{
  int idx = pupa_env_hashval (var);
  struct pupa_env_var *env;
  struct pupa_env_var *sort;
  struct pupa_env_var **sortp;
  
  /* If the variable does already exist, just update the variable.  */
  env = pupa_env_find (var);
  if (env)
    {
      char *old = env->value;
      env->value = pupa_strdup (val);
      if (! env->name)
	{
	  env->value = old;
	  return pupa_errno;
	}

      if (env->write_hook)
	(env->write_hook) (env);

      pupa_free (old);
      return 0;
    }

  /* The variable does not exist, create it.  */
  env = pupa_malloc (sizeof (struct pupa_env_var));
  if (! env)
    return pupa_errno;
  
  pupa_memset (env, 0, sizeof (struct pupa_env_var));
  
  env->name = pupa_strdup (var);
  if (! env->name)
    goto fail;
  
  env->value = pupa_strdup (val);
  if (! env->name)
    goto fail;
  
  /* Insert it in the hashtable.  */
  env->prevp = &pupa_env[idx];
  env->next = pupa_env[idx];
  if (pupa_env[idx])
    pupa_env[idx]->prevp = &env->next;
  pupa_env[idx] = env;
  
  /* Insert it in the sorted list.  */
  sortp = &pupa_env_sorted;
  sort = pupa_env_sorted;
  while (sort)
    {
      if (pupa_strcmp (sort->name, var) > 0)
	break;
      
      sortp = &sort->sort_next;
      sort = sort->sort_next;
    }
  env->sort_prevp = sortp;
  env->sort_next = sort;
  if (sort)
    sort->sort_prevp = &env->sort_next;
  *sortp = env;

 fail:
  if (pupa_errno)
    {
      pupa_free (env->name);
      pupa_free (env->value);
      pupa_free (env);
    }
  
  return 0;
}

char *
pupa_env_get (const char *name)
{
  struct pupa_env_var *env;
  env = pupa_env_find (name);
  if (! env)
    return 0;

  if (env->read_hook)
    {
      char *val;
      env->read_hook (env, &val);

      return val;
    }

  return env->value;
}

void
pupa_env_unset (const char *name)
{
  struct pupa_env_var *env;
  env = pupa_env_find (name);
  if (! env)
    return;

  /* XXX: It is not possible to unset variables with a read or write
     hook.  */
  if (env->read_hook || env->write_hook)
    return;

  *env->prevp = env->next;
  if (env->next)
    env->next->prevp = env->prevp;

  *env->sort_prevp = env->sort_next;
  if (env->sort_next)
    env->sort_next->sort_prevp = env->sort_prevp;

  pupa_free (env->name);
  pupa_free (env->value);
  pupa_free (env);
  return;
}

void
pupa_env_iterate (int (* func) (struct pupa_env_var *var))
{
  struct pupa_env_var *env;
  
  for (env = pupa_env_sorted; env; env = env->sort_next)
    if (func (env))
      return;
}

pupa_err_t
pupa_register_variable_hook (const char *var,
			     pupa_err_t (*read_hook) (struct pupa_env_var *var, char **),
			     pupa_err_t (*write_hook) (struct pupa_env_var *var))
{
  struct pupa_env_var *env = pupa_env_find (var);

  if (! env)
    if (pupa_env_set (var, "") != PUPA_ERR_NONE)
      return pupa_errno;
  
  env = pupa_env_find (var);
  /* XXX Insert an assertion?  */
  
  env->read_hook = read_hook;
  env->write_hook = write_hook;

  return PUPA_ERR_NONE;
}
