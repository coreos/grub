/* cmdline.c - the device-independent GRUB text command line */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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

#include "shared.h"

/* Find the next word from CMDLINE and return the pointer. If
   AFTER_EQUAL is non-zero, assume that the character `=' is treated as
   a space. Caution: this assumption is for backward compatibility.  */
char *
skip_to (int after_equal, char *cmdline)
{
  if (after_equal)
    {
      while (*cmdline && *cmdline != ' ' && *cmdline != '=')
	cmdline++;
      while (*cmdline == ' ' || *cmdline == '=')
	cmdline++;
    }
  else
    {
      while (*cmdline && *cmdline != ' ')
	cmdline++;
      while (*cmdline == ' ')
	cmdline++;
    }
  
  return cmdline;
}


void
print_cmdline_message (void)
{
  printf (" [ Minimal BASH-like line editing is supported.  For the first word, TAB
   lists possible command completions.  Anywhere else TAB lists the possible
   completions of a device/filename.  ESC at any time exits. ]\n");
}

/* Find the builtin whose command name is COMMAND and return the
   pointer. If not found, return 0.  */
struct builtin *
find_command (char *command)
{
  char *ptr;
  char c;
  struct builtin **builtin;
  
  /* Find the first space and terminate the command name.  */
  ptr = command;
  while (*ptr && *ptr != ' ' && *ptr != '=')
    ptr++;
  c = *ptr;
  *ptr = 0;

  /* Seek out the builtin whose command name is COMMAND.  */
  for (builtin = builtin_table; *builtin != 0; builtin++)
    {
      if (grub_strcmp (command, (*builtin)->name) == 0)
	{
	  *ptr = c;
	  return *builtin;
	}
    }
  
  /* Cannot find COMMAND.  */
  errnum = ERR_UNRECOGNIZED;
  *ptr = c;
  return 0;
}

static void
init_cmdline (void)
{
  /* Initialization.  */
  saved_drive = boot_drive;
  saved_partition = install_partition;
  current_drive = 0xFF;
  errnum = 0;
  
  /* Restore memory probe state.  */
  mbi.mem_upper = saved_mem_upper;
  if (mbi.mmap_length)
    mbi.flags |= MB_INFO_MEM_MAP;

  init_builtins ();
}

void
enter_cmdline (char *heap)
{
  /* Initialize the data and print a message.  */
  init_cmdline ();
  init_page ();
  print_cmdline_message ();

  while (1)
    {
      struct builtin *builtin;
      char *arg;
      
      *heap = 0;
      print_error ();

      /* Get the command-line with the minimal BASH-like interface.  */
      if (get_cmdline (PACKAGE "> ", heap, 2048, 0, 1))
	return;

      builtin = find_command (heap);
      if (! builtin)
	continue;

      if (! (builtin->flags & BUILTIN_CMDLINE))
	{
	  errnum = ERR_UNRECOGNIZED;
	  continue;
	}

      arg = skip_to (1, heap);
      (builtin->func) (arg, BUILTIN_CMDLINE);
    }
}

int
run_script (char *script, char *heap)
{
  char *old_entry;
  char *cur_entry = script;
  
  /* Initialize the data.  */
  init_cmdline ();

  while (1)
    {
      struct builtin *builtin;
      char *arg;
      
      print_error ();
      
      if (errnum)
	{
	  if (fallback_entry < 0)
	    return 1;
	  
	  grub_printf ("Press any key to continue...");
	  (void) getkey ();
	  return 1;
	}

      old_entry = cur_entry;
      while (*cur_entry++)
	;

      grub_memmove (heap, old_entry, (int) cur_entry - (int) old_entry);
      grub_printf ("%s\n", old_entry);

      if (! *heap)
	{
	  if (kernel_type == KERNEL_TYPE_NONE)
	    return 0;

	  grub_memmove (heap, "boot", 5);
	}
      
      builtin = find_command (heap);
      if (! builtin)
	continue;

      if (! (builtin->flags & BUILTIN_CMDLINE))
	{
	  errnum = ERR_UNRECOGNIZED;
	  continue;
	}

      arg = skip_to (1, heap);
      (builtin->func) (arg, BUILTIN_CMDLINE);
    }
}
