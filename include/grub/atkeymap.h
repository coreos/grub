/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_AT_KEYMAP_HEADER
#define GRUB_AT_KEYMAP_HEADER	1

static inline int
grub_at_map_to_usb (grub_uint8_t keycode)
{
  /* Modifier keys (ctrl, alt, shift, capslock, numlock and scrolllock
     are handled by driver and hence here are mapped to 0)*/

  static const grub_uint8_t at_to_usb_map[] =
    {
      /* 0x00 */ 0x00 /* Unused  */,     0x29 /* Escape */, 
      /* 0x02 */ 0x1e /* 1 */,           0x1f /* 2 */, 
      /* 0x04 */ 0x20 /* 3 */,           0x21 /* 4 */, 
      /* 0x06 */ 0x22 /* 5 */,           0x23 /* 6 */, 
      /* 0x08 */ 0x24 /* 7 */,           0x25 /* 8 */, 
      /* 0x0a */ 0x26 /* 9 */,           0x27 /* 0 */, 
      /* 0x0c */ 0x2d /* - */,           0x2e /* = */, 
      /* 0x0e */ 0x2a /* \b */,          0x2b /* \t */, 
      /* 0x10 */ 0x14 /* q */,           0x1a /* w */, 
      /* 0x12 */ 0x08 /* e */,           0x15 /* r */, 
      /* 0x14 */ 0x17 /* t */,           0x1c /* y */, 
      /* 0x16 */ 0x18 /* u */,           0x0c /* i */, 
      /* 0x18 */ 0x12 /* o */,           0x13 /* p */, 
      /* 0x1a */ 0x2f /* [ */,           0x30 /* ] */, 
      /* 0x1c */ 0x28 /* Enter */,       0x00 /* Left CTRL */, 
      /* 0x1e */ 0x04 /* a */,           0x16 /* s */, 
      /* 0x20 */ 0x07 /* d */,           0x09 /* f */, 
      /* 0x22 */ 0x0a /* g */,           0x0b /* h */, 
      /* 0x24 */ 0x0d /* j */,           0x0e /* k */, 
      /* 0x26 */ 0x0f /* l */,           0x33 /* ; */, 
      /* 0x28 */ 0x34 /* " */,           0x35 /* ` */, 
      /* 0x2a */ 0x00 /* Left Shift */,  0x32 /* \ */, 
      /* 0x2c */ 0x1d /* z */,           0x1b /* x */, 
      /* 0x2e */ 0x06 /* c */,           0x19 /* v */, 
      /* 0x30 */ 0x05 /* b */,           0x11 /* n */, 
      /* 0x32 */ 0x10 /* m */,           0x36 /* , */, 
      /* 0x34 */ 0x37 /* . */,           0x38 /* / */, 
      /* 0x36 */ 0x00 /* Right Shift */, 0x55 /* Num * */, 
      /* 0x38 */ 0x00 /* Left ALT  */,   0x2c /* Space */, 
      /* 0x3a */ 0x39 /* Caps Lock */,   0x3a /* F1 */, 
      /* 0x3c */ 0x3b /* F2 */,          0x3c /* F3 */, 
      /* 0x3e */ 0x3d /* F4 */,          0x3e /* F5 */, 
      /* 0x40 */ 0x3f /* F6 */,          0x40 /* F7 */, 
      /* 0x42 */ 0x41 /* F8 */,          0x42 /* F9 */, 
      /* 0x44 */ 0x43 /* F10 */,         0x53 /* NumLock */, 
      /* 0x46 */ 0x47 /* Scroll Lock */, 0x5f /* Num 7 */, 
      /* 0x48 */ 0x60 /* Num 8 */,       0x61 /* Num 9 */, 
      /* 0x4a */ 0x56 /* Num - */,       0x5c /* Num 4 */, 
      /* 0x4c */ 0x5d /* Num 5 */,       0x5e /* Num 6 */, 
      /* 0x4e */ 0x57 /* Num + */,       0x59 /* Num 1 */, 
      /* 0x50 */ 0x5a /* Num 2 */,       0x5b /* Num 3 */, 
      /* 0x52 */ 0x62 /* Num 0 */,       0x63 /* Num . */, 
      /* 0x54 */ 0x00,                   0x00, 
      /* 0x56 */ 0x64 /* 102nd key. */,  0x44 /* F11 */, 
      /* 0x58 */ 0x45 /* F12 */,         0x00
    };

  static const struct
  {
    grub_uint8_t from, to;
  } at_to_usb_extended[] = 
      {
	/* OLPC keys. Just mapped to normal keys.  */
	{0x65, 0x52 /* Up */    },
	{0x66, 0x51 /* Down */  },
	{0x67, 0x50 /* Left */  },
	{0x68, 0x4f /* Right */ },

	{0x9c, 0x58 /* Num \n */},
	{0xb5, 0x54 /* Num / */ }, 
	{0xc7, 0x4a /* Home */  }, 
	{0xc8, 0x52 /* Up */    },
	{0xc9, 0x4e /* NPage */ },
	{0xcb, 0x50 /* Left */  },
	{0xcd, 0x4f /* Right */ },
	{0xcf, 0x4d /* End */   }, 
	{0xd0, 0x51 /* Down */  },
	{0xd1, 0x4b /* PPage */ }, 
	{0xd2, 0x49 /* Insert */},
	{0xd3, 0x4c /* DC */    }, 
      };
  if (keycode >= ARRAY_SIZE (at_to_usb_map))
    {
      unsigned i;
      for (i = 0; i < ARRAY_SIZE (at_to_usb_extended); i++)
	if (at_to_usb_extended[i].from == keycode)
	  return at_to_usb_extended[i].to;
      return 0;
    }
  return at_to_usb_map[keycode];
}
#endif
