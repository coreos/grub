/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Free Software Foundation, Inc.
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
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

#ifndef PUPA_TERM_HEADER
#define PUPA_TERM_HEADER	1

#include <pupa/symbol.h>
#include <pupa/types.h>

/* These are used to represent the various color states we use.  */
typedef enum
  {
    /* The color used to display all text that does not use the
       user defined colors below.  */
    PUPA_TERM_COLOR_STANDARD,
    /* The user defined colors for normal text.  */
    PUPA_TERM_COLOR_NORMAL,
    /* The user defined colors for highlighted text.  */
    PUPA_TERM_COLOR_HIGHLIGHT
  }
pupa_term_color_state;

/* Flags for representing the capabilities of a terminal.  */
/* Some notes about the flags:
   - These flags are used by higher-level functions but not terminals
   themselves.
   - If a terminal is dumb, you may assume that only putchar, getkey and
   checkkey are called.
   - Some fancy features (setcolorstate, setcolor and setcursor) can be set
   to NULL.  */

/* Set when input characters shouldn't be echoed back.  */
#define PUPA_TERM_NO_ECHO	(1 << 0)
/* Set when the editing feature should be disabled.  */
#define PUPA_TERM_NO_EDIT	(1 << 1)
/* Set when the terminal cannot do fancy things.  */
#define PUPA_TERM_DUMB		(1 << 2)
/* Set when the terminal needs to be initialized.  */
#define PUPA_TERM_NEED_INIT	(1 << 16)

struct pupa_term
{
  /* The terminal name.  */
  const char *name;
  
  /* Put a character.  */
  void (*putchar) (int c);
  
  /* Check if any input character is available.  */
  int (*checkkey) (void);
  
  /* Get a character.  */
  int (*getkey) (void);
  
  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  pupa_uint16_t (*getxy) (void);
  
  /* Go to the position (X, Y).  */
  void (*gotoxy) (pupa_uint8_t x, pupa_uint8_t y);
  
  /* Clear the screen.  */
  void (*cls) (void);
  
  /* Set the current color to be used */
  void (*setcolorstate) (pupa_term_color_state state);
  
  /* Set the normal color and the highlight color. The format of each
     color is VGA's.  */
  void (*setcolor) (pupa_uint8_t normal_color, pupa_uint8_t highlight_color);
  
  /* Turn on/off the cursor.  */
  void (*setcursor) (int on);

  /* The feature flags defined above.  */
  pupa_uint32_t flags;
  
  /* The next terminal.  */
  struct pupa_term *next;
};
typedef struct pupa_term *pupa_term_t;

void EXPORT_FUNC(pupa_term_register) (pupa_term_t term);
void EXPORT_FUNC(pupa_term_unregister) (pupa_term_t term);
void EXPORT_FUNC(pupa_term_iterate) (int (*hook) (pupa_term_t term));

void EXPORT_FUNC(pupa_term_set_current) (pupa_term_t term);
pupa_term_t EXPORT_FUNC(pupa_term_get_current) (void);

void EXPORT_FUNC(pupa_putchar) (int c);
int EXPORT_FUNC(pupa_getkey) (void);
int EXPORT_FUNC(pupa_checkkey) (void);
pupa_uint16_t EXPORT_FUNC(pupa_getxy) (void);
void EXPORT_FUNC(pupa_gotoxy) (pupa_uint8_t x, pupa_uint8_t y);
void EXPORT_FUNC(pupa_cls) (void);
void EXPORT_FUNC(pupa_setcolorstate) (pupa_term_color_state state);
void EXPORT_FUNC(pupa_setcolor) (pupa_uint8_t normal_color,
				 pupa_uint8_t highlight_color);
int EXPORT_FUNC(pupa_setcursor) (int on);

/* For convenience.  */
#define PUPA_TERM_ASCII_CHAR(c)	((c) & 0xff)

#endif /* ! PUPA_TERM_HEADER */
