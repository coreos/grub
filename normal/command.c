/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Yoshinori K. Okuji <okuji@enbug.org>
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

#include <pupa/normal.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>

static pupa_command_t pupa_command_list;

void
pupa_register_command (const char *name,
		       int (*func) (int argc, char *argv[]),
		       unsigned flags,
		       const char *summary,
		       const char *description)
{
  pupa_command_t cmd, *p;

  cmd = (pupa_command_t) pupa_malloc (sizeof (*cmd));
  if (! cmd)
    return;

  cmd->name = name;
  cmd->func = func;
  cmd->flags = flags;
  cmd->summary = summary;
  cmd->description = description;

  /* Keep the list sorted for simplicity.  */
  p = &pupa_command_list;
  while (*p)
    {
      if (pupa_strcmp ((*p)->name, name) > 0)
	break;

      p = &((*p)->next);
    }

  cmd->next = *p;
  *p = cmd;
}

void
pupa_unregister_command (const char *name)
{
  pupa_command_t *p, q;

  for (p = &pupa_command_list, q = *p; q; p = &(q->next), q = q->next)
    if (pupa_strcmp (name, q->name) == 0)
      {
        *p = q->next;
        pupa_free (q);
        break;
      }
}

pupa_command_t
pupa_command_find (char *cmdline)
{
  char *first_space;
  pupa_command_t cmd;

  first_space = pupa_strchr (cmdline, ' ');
  if (first_space)
    *first_space = '\0';

  for (cmd = pupa_command_list; cmd; cmd = cmd->next)
    if (pupa_strcmp (cmdline, cmd->name) == 0)
      break;

  if (! cmd)
    pupa_error (PUPA_ERR_UNKNOWN_COMMAND, "unknown command `%s'", cmdline);
  
  if (first_space)
    *first_space = ' ';

  return cmd;
}

int
pupa_command_execute (char *cmdline)
{
  pupa_command_t cmd;
  char *p;
  char **args;
  int num = 0;
  int i;
  int ret;

  cmd = pupa_command_find (cmdline);
  if (! cmd)
    return -1;

  /* Count arguments.  */
  p = cmdline;
  while (1)
    {
      while (*p && *p != ' ')
	p++;

      if (! *p)
	break;
      
      while (*p == ' ')
	p++;

      num++;
    }

  args = (char **) pupa_malloc (sizeof (char *) * (num + 1));
  if (! args)
    return -1;

  /* Fill arguments.  */
  for (i = 0, p = pupa_strchr (cmdline, ' '); i < num && p; i++)
    {
      if (! p)
	break;
      
      while (*p == ' ')
	p++;

      args[i] = p;

      while (*p && *p != ' ')
	p++;

      *p++ = '\0';
    }

  /* Terminate the array with NULL.  */
  args[i] = 0;

  ret = (cmd->func) (num, args);

  pupa_free (args);
  return ret;
}

static int
rescue_command (int argc __attribute__ ((unused)),
		char *argv[] __attribute__ ((unused)))
{
  pupa_longjmp (pupa_exit_env, 0);

  /* Never reach here.  */
  return 0;
}

void
pupa_command_init (void)
{
  /* This is a special command, because this never be called actually.  */
  pupa_register_command ("title", 0, PUPA_COMMAND_FLAG_TITLE, 0, 0);

  pupa_register_command ("rescue", rescue_command, PUPA_COMMAND_FLAG_BOTH,
			 "rescue",
			 "Enter into the rescue mode.");
}
