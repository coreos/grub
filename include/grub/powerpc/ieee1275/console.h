/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002, 2004  Free Software Foundation, Inc.
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

#ifndef PUPA_CONSOLE_MACHINE_HEADER
#define PUPA_CONSOLE_MACHINE_HEADER	1

/* Define scan codes.  */
#define PUPA_CONSOLE_KEY_LEFT		0x4B00
#define PUPA_CONSOLE_KEY_RIGHT		0x4D00
#define PUPA_CONSOLE_KEY_UP		0x4800
#define PUPA_CONSOLE_KEY_DOWN		0x5000
#define PUPA_CONSOLE_KEY_IC		0x5200
#define PUPA_CONSOLE_KEY_DC		0x5300
#define PUPA_CONSOLE_KEY_BACKSPACE	0x0008
#define PUPA_CONSOLE_KEY_HOME		0x4700
#define PUPA_CONSOLE_KEY_END		0x4F00
#define PUPA_CONSOLE_KEY_NPAGE		0x4900
#define PUPA_CONSOLE_KEY_PPAGE		0x5100

#ifndef ASM_FILE

#include <pupa/types.h>
#include <pupa/symbol.h>

/* These are global to share code between C and asm.  */
extern pupa_uint8_t pupa_console_cur_color;
void pupa_console_real_putchar (int c);
int EXPORT_FUNC(pupa_console_checkkey) (void);
int EXPORT_FUNC(pupa_console_getkey) (void);
pupa_uint16_t pupa_console_getxy (void);
void pupa_console_gotoxy (pupa_uint8_t x, pupa_uint8_t y);
void pupa_console_cls (void);
void pupa_console_setcursor (int on);

/* Initialize the console system.  */
void pupa_console_init (void);

#endif

#endif /* ! PUPA_CONSOLE_MACHINE_HEADER */
