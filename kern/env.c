/* env.c - Environment variables */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>

/* The size of the hash table.  */
#define	HASHSZ	13

/* A hashtable for quick lookup of variables.  */
struct grub_env_context
{
  struct grub_env_var *vars[HASHSZ];
  
  struct grub_env_var *sorted;

  /* One level deeper on the stack.  */
  struct grub_env_context *next;
};

/* The global context for environment variables.  */
static struct grub_env_context grub_env_context;

/* The nested contexts for regular variables.  */
static struct grub_env_context *grub_env_var_context = &grub_env_context;

/* Return the hash representation of the string S.  */
static unsigned int grub_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more effecient.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
}

static struct grub_env_var *
grub_env_find (const char *name)
{
  struct grub_env_var *var;
  int idx = grub_env_hashval (name);

  /* Look for the variable in the current context.  */
  for (var = grub_env_var_context->vars[idx]; var; var = var->next)
    if (! grub_strcmp (var->name, name))
      return var;

  /* Look for the variable in the environment context.  */
  for (var = grub_env_context.vars[idx]; var; var = var->next)
    if (! grub_strcmp (var->name, name))
      return var;

  return 0;
}

grub_err_t
grub_env_context_open (void)
{
  struct grub_env_context *context;
  int i;

  context = grub_malloc (sizeof (*context));
  if (! context)
    return grub_errno;

  for (i = 0; i < HASHSZ; i++)
    context->vars[i] = 0;
  context->next = grub_env_var_context;
  context->sorted = 0;
  
  grub_env_var_context = context;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_env_context_close (void)
{
  struct grub_env_context *context;
  struct grub_env_var *env;
  struct grub_env_var *prev = 0;

  context = grub_env_var_context->next;

  /* Free the variables associated with this context.  */
  for (env = grub_env_var_context->sorted; env; env = env->sort_next)
    {
      /* XXX: What if a hook is associated with this variable?  */
      grub_free (prev);
      prev = env;
    }
  grub_free (prev);

  /* Restore the previous context.  */
  grub_free (grub_env_var_context);
  grub_env_var_context = context;

  return GRUB_ERR_NONE;
}

static void
grub_env_insert (struct grub_env_context *context,
		 struct grub_env_var *env)
{
  struct grub_env_var **grub_env = context->vars;  
  struct grub_env_var *sort;
  struct grub_env_var **sortp;
  int idx = grub_env_hashval (env->name);

  /* Insert it in the hashtable.  */
  env->prevp = &grub_env[idx];
  env->next = grub_env[idx];
  if (grub_env[idx])
    grub_env[idx]->prevp = &env->next;
  grub_env[idx] = env;

  /* Insert it in the sorted list.  */
  sortp = &context->sorted;
  sort = context->sorted;
  while (sort)
    {
      if (grub_strcmp (sort->name, env->name) > 0)
	break;
      
      sortp = &sort->sort_next;
      sort = sort->sort_next;
    }
  env->sort_prevp = sortp;
  env->sort_next = sort;
  if (sort)
    sort->sort_prevp = &env->sort_next;
  *sortp = env;
}


static void
grub_env_remove (struct grub_env_var *env)
{
  /* Remove the entry from the variable table.  */
  *env->prevp = env->next;
  if (env->next)
    env->next->prevp = env->prevp;

  /* And from the sorted list.  */
  *env->sort_prevp = env->sort_next;
  if (env->sort_next)
    env->sort_next->sort_prevp = env->sort_prevp;
}

grub_err_t
grub_env_export (const char *var)
{
  struct grub_env_var *env;
  int idx = grub_env_hashval (var);

  /* Look for the variable in the current context only.  */
  for (env = grub_env_var_context->vars[idx]; env; env = env->next)
    if (! grub_strcmp (env->name, var))
      {
	/* Remove the variable from the old context and reinsert it
	   into the environment.  */
	grub_env_remove (env);
	grub_env_insert (&grub_env_context, env);

	return GRUB_ERR_NONE;
      }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_env_set (const char *var, const char *val)
{
  struct grub_env_var *env;

  /* If the variable does already exist, just update the variable.  */
  env = grub_env_find (var);
  if (env)
    {
      char *old = env->value;

      if (env->write_hook)
	env->value = env->write_hook (env, val);
      else
	env->value = grub_strdup (val);
      
      if (! env->value)
	{
	  env->value = old;
	  return grub_errno;
	}

      grub_free (old);
      return 0;
    }

  /* The variable does not exist, create it.  */
  env = grub_malloc (sizeof (struct grub_env_var));
  if (! env)
    return grub_errno;
  
  grub_memset (env, 0, sizeof (struct grub_env_var));
  
  env->name = grub_strdup (var);
  if (! env->name)
    goto fail;
  
  env->value = grub_strdup (val);
  if (! env->value)
    goto fail;

  grub_env_insert (grub_env_var_context, env);

  return 0;

 fail:
  grub_free (env->name);
  grub_free (env->value);
  grub_free (env);

  return grub_errno;
}

char *
grub_env_get (const char *name)
{
  struct grub_env_var *env;
  env = grub_env_find (name);
  if (! env)
    return 0;

  if (env->read_hook)
    return env->read_hook (env, env->value);

  return env->value;
}

void
grub_env_unset (const char *name)
{
  struct grub_env_var *env;
  env = grub_env_find (name);
  if (! env)
    return;

  /* XXX: It is not possible to unset variables with a read or write
     hook.  */
  if (env->read_hook || env->write_hook)
    return;

  grub_env_remove (env);

  grub_free (env->name);
  grub_free (env->value);
  grub_free (env);
  return;
}

void
grub_env_iterate (int (* func) (struct grub_env_var *var))
{
  struct grub_env_var *env = grub_env_context.sorted;
  struct grub_env_var *var = grub_env_var_context->sorted;

  /* Initially these are the same.  */
  if (env == var)
    var = 0;
  
  while (env || var)
    {
      struct grub_env_var **cur;

      /* Select the first name to be printed from the head of two
	 sorted lists.  */
      if (! env)
	cur = &var;
      else if (! var)
	cur = &env;
      else if (grub_strcmp (env->name, var->name) > 0)
	cur = &var;
      else
	cur = &env;

      if (func (*cur))
	return;
      *cur = (*cur)->sort_next;
    }
}

grub_err_t
grub_register_variable_hook (const char *var,
			     grub_env_read_hook_t read_hook,
			     grub_env_write_hook_t write_hook)
{
  struct grub_env_var *env = grub_env_find (var);

  if (! env)
    {
      char *val = grub_strdup ("");

      if (! val)
	return grub_errno;
      
      if (grub_env_set (var, val) != GRUB_ERR_NONE)
	return grub_errno;
    }
  
  env = grub_env_find (var);
  /* XXX Insert an assertion?  */
  
  env->read_hook = read_hook;
  env->write_hook = write_hook;

  return GRUB_ERR_NONE;
}
