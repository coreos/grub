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

#ifndef PUPA_ARG_HEADER
#define PUPA_ARG_HEADER	1

#include <pupa/symbol.h>
#include <pupa/err.h>
#include <pupa/types.h>

enum pupa_arg_type
  {
    ARG_TYPE_NONE,
    ARG_TYPE_STRING,
    ARG_TYPE_INT,
    ARG_TYPE_DEVICE,
    ARG_TYPE_FILE,
    ARG_TYPE_DIR,
    ARG_TYPE_PATHNAME
  };

typedef enum pupa_arg_type pupa_arg_type_t;

/* Flags for the option field op pupa_arg_option.  */
#define PUPA_ARG_OPTION_OPTIONAL	1 << 1

enum pupa_key_type
  {
    PUPA_KEY_ARG = -1,
    PUPA_KEY_END = -2
  };
typedef enum pupa_key_type pupa_arg_key_type_t;

struct pupa_arg_option
{
  char *longarg;
  char shortarg;
  int flags;
  char *doc;
  char *arg;
  pupa_arg_type_t type;
};

struct pupa_arg_list
{
  int set;
  char *arg;
};

#endif /* ! PUPA_ARG_HEADER */
