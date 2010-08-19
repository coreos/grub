/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_AT_KEYBOARD_HEADER
#define GRUB_AT_KEYBOARD_HEADER	1

#define SHIFT_L		0x2a
#define SHIFT_R		0x36
#define CTRL		0x1d
#define ALT		0x38
#define CAPS_LOCK	0x3a
#define NUM_LOCK	0x45
#define SCROLL_LOCK	0x46

/* Used for sending commands to the controller.  */
#define KEYBOARD_COMMAND_ISREADY(x)	!((x) & 0x02)
#define KEYBOARD_COMMAND_READ		0x20
#define KEYBOARD_COMMAND_WRITE		0x60
#define KEYBOARD_COMMAND_REBOOT		0xfe

#define KEYBOARD_SCANCODE_SET1		0x40

#define KEYBOARD_ISMAKE(x)	!((x) & 0x80)
#define KEYBOARD_ISREADY(x)	((x) & 0x01)
#define KEYBOARD_SCANCODE(x)	((x) & 0x7f)

#ifdef GRUB_MACHINE_IEEE1275
#define OLPC_UP		GRUB_TERM_UP
#define OLPC_DOWN	GRUB_TERM_DOWN
#define OLPC_LEFT	GRUB_TERM_LEFT
#define OLPC_RIGHT	GRUB_TERM_RIGHT
#else
#define OLPC_UP		'\0'
#define OLPC_DOWN	'\0'
#define OLPC_LEFT	'\0'
#define OLPC_RIGHT	'\0'
#endif

#define GRUB_AT_KEY_KEYBOARD_MAP(name)					\
static const unsigned name[128] =					\
{									\
  /* 0x00 */ '\0', GRUB_TERM_ESC, '1', '2', '3', '4', '5', '6',		\
  /* 0x08 */ '7', '8', '9', '0',					\
  /* 0x0c */ '-', '=', GRUB_TERM_BACKSPACE, GRUB_TERM_TAB,		\
  /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',			\
  /* 0x18 */ 'o', 'p', '[', ']', '\n', '\0', 'a', 's',			\
  /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',			\
  /* 0x28 */ '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',			\
  /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', '\0', '*' | GRUB_TERM_KEYPAD, \
  /* 0x38 */ '\0', ' ', '\0', GRUB_TERM_KEY_F1,				\
  /* 0x3c */ GRUB_TERM_KEY_F2, GRUB_TERM_KEY_F3,                        \
  /* 0x3e */ GRUB_TERM_KEY_F4, GRUB_TERM_KEY_F5,			\
  /* 0x40 */ GRUB_TERM_KEY_F6, GRUB_TERM_KEY_F7,			\
  /* 0x42 */ GRUB_TERM_KEY_F8, GRUB_TERM_KEY_F9,			\
  /* 0x44 */ GRUB_TERM_KEY_F10, '\0',					\
  /* 0x46 */ '\0', GRUB_TERM_KEY_HOME,					\
  /* 0x48 */ GRUB_TERM_KEY_UP,						\
  /* 0x49 */ GRUB_TERM_KEY_NPAGE,					\
  /* 0x4a */ '-' | GRUB_TERM_KEYPAD,					\
  /* 0x4b */ GRUB_TERM_KEY_LEFT,					\
  /* 0x4c */ GRUB_TERM_KEY_CENTER | GRUB_TERM_KEYPAD,			\
  /* 0x4d */ GRUB_TERM_KEY_RIGHT,					\
  /* 0x4e */ '+' | GRUB_TERM_KEYPAD,					\
  /* 0x4f */ GRUB_TERM_KEY_END,						\
  /* 0x50 */ GRUB_TERM_KEY_DOWN,					\
  /* 0x51 */ GRUB_TERM_KEY_PPAGE,					\
  /* 0x52 */ GRUB_TERM_KEY_INSERT,					\
  /* 0x53 */ GRUB_TERM_KEY_DC,						\
  /* 0x54 */ '\0', '\0', GRUB_TERM_KEY_102, GRUB_TERM_KEY_F11,		\
  /* 0x58 */ GRUB_TERM_KEY_F12, '\0', '\0', '\0', '\0', '\0', '\0', '\0', \
  /* 0x60 */ '\0', '\0', '\0', '\0', '\0', OLPC_UP, OLPC_DOWN, OLPC_LEFT, \
  /* 0x68 */ OLPC_RIGHT							\
}

#define GRUB_AT_KEY_KEYBOARD_MAP_SHIFT(name)  	\
static unsigned name[128] =                     \
{                                               \
  '\0', '\0', '!', '@', '#', '$', '%', '^',     \
  '&', '*', '(', ')', '_', '+', '\0', '\0',     \
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',       \
  'O', 'P', '{', '}', '\n', '\0', 'A', 'S',     \
  'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',       \
  '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',     \
  'B', 'N', 'M', '<', '>', '?',                 \
  [0x56] = GRUB_TERM_KEY_SHIFT_102              \
}

#endif
