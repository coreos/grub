/* err.h - error numbers and prototypes */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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

#ifndef PUPA_ERR_HEADER
#define PUPA_ERR_HEADER	1

#include <pupa/symbol.h>

typedef enum
  {
    PUPA_ERR_NONE = 0,
    PUPA_ERR_BAD_MODULE,
    PUPA_ERR_OUT_OF_MEMORY,
    PUPA_ERR_BAD_FILE_TYPE,
    PUPA_ERR_FILE_NOT_FOUND,
    PUPA_ERR_FILE_READ_ERROR,
    PUPA_ERR_BAD_FILENAME,
    PUPA_ERR_UNKNOWN_FS,
    PUPA_ERR_BAD_FS,
    PUPA_ERR_BAD_NUMBER,
    PUPA_ERR_OUT_OF_RANGE,
    PUPA_ERR_UNKNOWN_DEVICE,
    PUPA_ERR_BAD_DEVICE,
    PUPA_ERR_READ_ERROR,
    PUPA_ERR_WRITE_ERROR,
    PUPA_ERR_UNKNOWN_COMMAND,
    PUPA_ERR_BAD_ARGUMENT,
    PUPA_ERR_BAD_PART_TABLE,
    PUPA_ERR_UNKNOWN_OS,
    PUPA_ERR_BAD_OS,
    PUPA_ERR_NO_KERNEL,
    PUPA_ERR_BAD_FONT,
    PUPA_ERR_NOT_IMPLEMENTED_YET,
    PUPA_ERR_SYMLINK_LOOP
  }
pupa_err_t;

extern pupa_err_t EXPORT_VAR(pupa_errno);
extern char EXPORT_VAR(pupa_errmsg)[];

pupa_err_t EXPORT_FUNC(pupa_error) (pupa_err_t n, const char *fmt, ...);
void EXPORT_FUNC(pupa_fatal) (const char *fmt, ...) __attribute__ ((noreturn));
void EXPORT_FUNC(pupa_print_error) (void);

#endif /* ! PUPA_ERR_HEADER */
