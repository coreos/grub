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

#include <grub/dl.h>
#include <grub/at_keyboard.h>
#include <grub/cpu/at_keyboard.h>
#include <grub/cpu/io.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/keyboard_layouts.h>

static short at_keyboard_status = 0;
static int e0_received = 0;
static int f0_received = 0;
static int pending_key = -1;

static grub_uint8_t led_status;

#define KEYBOARD_LED_SCROLL		(1 << 0)
#define KEYBOARD_LED_NUM		(1 << 1)
#define KEYBOARD_LED_CAPS		(1 << 2)

static grub_uint8_t grub_keyboard_controller_orig;
static grub_uint8_t grub_keyboard_orig_set;
static grub_uint8_t current_set; 

static const grub_uint8_t set1_mapping[128] =
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
    /* 0x1c */ 0x28 /* Enter */,       0xe0 /* Left CTRL */, 
    /* 0x1e */ 0x04 /* a */,           0x16 /* s */, 
    /* 0x20 */ 0x07 /* d */,           0x09 /* f */, 
    /* 0x22 */ 0x0a /* g */,           0x0b /* h */, 
    /* 0x24 */ 0x0d /* j */,           0x0e /* k */, 
    /* 0x26 */ 0x0f /* l */,           0x33 /* ; */, 
    /* 0x28 */ 0x34 /* " */,           0x35 /* ` */, 
    /* 0x2a */ 0xe1 /* Left Shift */,  0x32 /* \ */, 
    /* 0x2c */ 0x1d /* z */,           0x1b /* x */, 
    /* 0x2e */ 0x06 /* c */,           0x19 /* v */, 
    /* 0x30 */ 0x05 /* b */,           0x11 /* n */, 
    /* 0x32 */ 0x10 /* m */,           0x36 /* , */, 
    /* 0x34 */ 0x37 /* . */,           0x38 /* / */, 
    /* 0x36 */ 0xe5 /* Right Shift */, 0x55 /* Num * */, 
    /* 0x38 */ 0xe2 /* Left ALT  */,   0x2c /* Space */, 
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
    /* 0x58 */ 0x45 /* F12 */,         0x00,
    /* 0x5a */ 0x00,                   0x00,
    /* 0x5c */ 0x00,                   0x00,
    /* 0x5e */ 0x00,                   0x00,
    /* 0x60 */ 0x00,                   0x00,
    /* 0x62 */ 0x00,                   0x00,
    /* OLPC keys. Just mapped to normal keys.  */
    /* 0x64 */ 0x00,                   0x52 /* Up */,
    /* 0x66 */ 0x51 /* Down */,        0x50 /* Left */,
    /* 0x68 */ 0x4f /* Right */
  };

static const struct
{
  grub_uint8_t from, to;
} set1_e0_mapping[] = 
  {
    {0x1c, 0x58 /* Num \n */},
    {0x1d, 0xe4 /* Right CTRL */},
    {0x35, 0x54 /* Num / */ }, 
    {0x38, 0xe6 /* Right ALT */},
    {0x47, 0x4a /* Home */  }, 
    {0x48, 0x52 /* Up */    },
    {0x49, 0x4e /* NPage */ },
    {0x4b, 0x50 /* Left */  },
    {0x4d, 0x4f /* Right */ },
    {0x4f, 0x4d /* End */   }, 
    {0x50, 0x51 /* Down */  },
    {0x51, 0x4b /* PPage */ }, 
    {0x52, 0x49 /* Insert */},
    {0x53, 0x4c /* DC */    }, 
  };

static const grub_uint8_t set2_mapping[256] =
  {
    /* 0x00 */ 0x00,          0x42 /* F9 */,   0x00,          0x3e /* F5 */,
    /* 0x04 */ 0x3c /* F3 */, 0x3a /* F1 */,    0x3b /* F2 */, 0x45 /* F12 */,
    /* 0x08 */ 0x00,          0x43 /* F10 */,   0x41 /* F8 */, 0x3f /* F6 */,
    /* 0x0c */ 0x3d /* F4 */, 0x2b /* \t */,    0x35 /* ` */,  0x00,
    /* 0x10 */ 0x00,          GRUB_KEYBOARD_KEY_LEFT_ALT,             GRUB_KEYBOARD_KEY_LEFT_SHIFT,          0x00,
    /* 0x14 */ GRUB_KEYBOARD_KEY_LEFT_CTRL,          0x14 /* q */,     0x1e /* 1 */,  0x00,
    /* 0x18 */ 0x00,          0x00,             0x1d /* s */,  0x16 /* s */,
    /* 0x1c */ 0x04 /* a */,  0x1a /* w */,     0x1f /* 2 */,  0x00,
    /* 0x20 */ 0x00,          0x06 /* c */,     0x1b /* x */,  0x07 /* d */,
    /* 0x24 */ 0x08 /* e */,  0x21 /* 4 */,     0x20 /* 3 */,  0x00,
    /* 0x28 */ 0x00,          0x2c /* Space */, 0x19 /* v */,  0x09 /* f */,
    /* 0x2c */ 0x17 /* t */,  0x15 /* r */,     0x22 /* 5 */,  0x00,
    /* 0x30 */ 0x00,          0x11 /* n */,     0x05 /* b */,  0x0b /* h */,
    /* 0x34 */ 0x0a /* g */,  0x1c /* y */,     0x23 /* 6 */,  0x00,
    /* 0x38 */ 0x00,          0x00,             0x10 /* m */,  0x0d /* j */,
    /* 0x3c */ 0x18 /* u */,  0x24 /* 7 */,     0x25 /* 8 */,  0x00,
    /* 0x40 */ 0x00,          0x37 /* . */,     0x0e /* k */,  0x0c /* i */,
    /* 0x44 */ 0x12 /* o */,  0x27 /* 0 */,     0x26 /* 9 */,  0x00,
    /* 0x48 */ 0x00,          0x36 /* , */,     0x38 /* / */,  0x0f /* l */,
    /* 0x4c */ 0x33 /* ; */,  0x13 /* p */,     0x2d /* - */,  0x00,
    /* 0x50 */ 0x00,          0x00,             0x34 /* ' */,  0x00,
    /* 0x54 */ 0x2f /* [ */,  0x2e /* = */,     0x00,          0x00,
    /* 0x58 */ GRUB_KEYBOARD_KEY_CAPS_LOCK, GRUB_KEYBOARD_KEY_RIGHT_SHIFT, 0x28 /* \n */,0x30 /* ] */,
    /* 0x5c */ 0x00,          0x32 /* \ */,             0x00,          0x00,
    /* 0x60 */ 0x00,          0x64 /* 102nd key. */,         0x00,          0x00,
    /* 0x64 */ 0x00,          0x00,             0x2a /* \b */, 0x00,
    /* 0x68 */ 0x00,          0x59 /* Num 1 */, 0x00,          0x5c /* Num 4 */,
    /* 0x6c */ 0x5f /* Num 7 */, 0x00,          0x00,          0x00,
    /* 0x70 */ 0x62 /* Num 0 */, 0x63 /* Num 0 */, 0x5a /* Num 2 */, 0x5d /* Num 5 */,
    /* 0x74 */ 0x5e /* Num 6 */, 0x60 /* Num 8 */, 0x29 /* \e */, 0x53 /* NumLock */,
    /* 0x78 */ 0x44 /* F11 */, 0x57 /* Num + */, 0x5b /* Num 3 */, 0x56 /* Num - */,
    /* 0x7c */ 0x55 /* Num * */, 0x61 /* Num 9 */, 0x47 /* ScrollLock */, 0x00,
    /* 0x80 */ 0x00, 0x00, 0x00, 0x40 /* F7 */,
  };

static const struct
{
  grub_uint8_t from, to;
} set2_e0_mapping[] = 
  {
    {0x11, GRUB_KEYBOARD_KEY_RIGHT_ALT},
    {0x14, GRUB_KEYBOARD_KEY_RIGHT_CTRL},
    {0x4a, 0x54}, /* Num / */
    {0x5a, 0x58}, /* Num enter */
    {0x69, 0x4d}, /* End */
    {0x6b, 0x50}, /* Left */
    {0x6c, 0x4a}, /* Home */
    {0x70, 0x49}, /* Insert */
    {0x71, 0x4c}, /* Delete */
    {0x72, 0x51}, /* Down */
    {0x74, 0x4f}, /* Right */
    {0x75, 0x52}, /* Up */
    {0x7a, 0x4e}, /* PageDown */
    {0x7d, 0x4b}, /* PageUp */
  };

static void
keyboard_controller_wait_until_ready (void)
{
  while (! KEYBOARD_COMMAND_ISREADY (grub_inb (KEYBOARD_REG_STATUS)));
}

static void
grub_keyboard_controller_write (grub_uint8_t c)
{
  keyboard_controller_wait_until_ready ();
  grub_outb (KEYBOARD_COMMAND_WRITE, KEYBOARD_REG_STATUS);
  grub_outb (c, KEYBOARD_REG_DATA);
}

static grub_uint8_t
query_mode (int mode)
{
  keyboard_controller_wait_until_ready ();
  grub_outb (0xf0, KEYBOARD_REG_DATA);
  keyboard_controller_wait_until_ready ();
  grub_inb (KEYBOARD_REG_DATA);
  keyboard_controller_wait_until_ready ();
  grub_outb (mode, KEYBOARD_REG_DATA);

  keyboard_controller_wait_until_ready ();

  return grub_inb (KEYBOARD_REG_DATA);
}

/* QEMU translates the set even in no-translate mode.  */
static inline int
recover_mode (grub_uint8_t report)
{
  if (report == 0x43 || report == 1)
    return 1;
  if (report == 0x41 || report == 2)
    return 2;
  if (report == 0x3f || report == 3)
    return 3;
  return -1;
}

static void
set_scancodes (void)
{
  grub_keyboard_controller_write (grub_keyboard_controller_orig
				  & ~KEYBOARD_AT_TRANSLATE);
  grub_keyboard_orig_set = recover_mode (query_mode (0));

  query_mode (2);
  current_set = query_mode (0);
  current_set = recover_mode (current_set);
  if (current_set == 2)
    return;

  query_mode (1);
  current_set = query_mode (0);
  current_set = recover_mode (current_set);
  if (current_set == 1)
    return;
  grub_printf ("No supported scancode set found\n");
}

static grub_uint8_t
grub_keyboard_controller_read (void)
{
  keyboard_controller_wait_until_ready ();
  grub_outb (KEYBOARD_COMMAND_READ, KEYBOARD_REG_STATUS);
  return grub_inb (KEYBOARD_REG_DATA);
}

static void
keyboard_controller_led (grub_uint8_t leds)
{
  keyboard_controller_wait_until_ready ();
  grub_outb (0xed, KEYBOARD_REG_DATA);
  keyboard_controller_wait_until_ready ();
  grub_outb (leds & 0x7, KEYBOARD_REG_DATA);
}

static int
fetch_key (int *is_break)
{
  int was_ext = 0;
  grub_uint8_t at_key;
  int ret = 0;

  if (! KEYBOARD_ISREADY (grub_inb (KEYBOARD_REG_STATUS)))
    return -1;
  at_key = grub_inb (KEYBOARD_REG_DATA);
  if (at_key == 0xe0)
    {
      e0_received = 1;
      return -1;
    }

  was_ext = e0_received;
  e0_received = 0;

  switch (current_set)
    {
    case 1:
      *is_break = !!(at_key & 0x80);
      if (!was_ext)
	ret = set1_mapping[at_key & 0x7f];
      else
	{
	  unsigned i;
	  for (i = 0; i < ARRAY_SIZE (set1_e0_mapping); i++)
	    if (set1_e0_mapping[i].from == (at_key & 0x80))
	      {
		ret = set1_e0_mapping[i].to;
		break;
	      }
	}
      break;
    case 2:
      if (at_key == 0xf0)
	{
	  f0_received = 1;
	  return -1;
	}
      *is_break = f0_received;
      f0_received = 0;
      if (!was_ext)
	ret = set2_mapping[at_key];
      else
	{
	  unsigned i;
	  for (i = 0; i < ARRAY_SIZE (set1_e0_mapping); i++)
	    if (set1_e0_mapping[i].from == (at_key & 0x80))
	      {
		ret = set1_e0_mapping[i].to;
		break;
	      }
	}	
      break;
    default:
      return -1;
    }
  if (!ret)
    {
      grub_printf ("Unknown key 0x%02x from set %d\n\n", at_key, current_set);
      return -1;
    }
  return ret;
}

/* FIXME: This should become an interrupt service routine.  For now
   it's just used to catch events from control keys.  */
static int
grub_keyboard_isr (grub_keyboard_key_t key, int is_break)
{
  if (!is_break)
    switch (key)
      {
      case GRUB_KEYBOARD_KEY_LEFT_SHIFT:
	at_keyboard_status |= GRUB_TERM_STATUS_LSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_SHIFT:
	at_keyboard_status |= GRUB_TERM_STATUS_RSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_CTRL:
	at_keyboard_status |= GRUB_TERM_STATUS_LCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_CTRL:
	at_keyboard_status |= GRUB_TERM_STATUS_RCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_ALT:
	at_keyboard_status |= GRUB_TERM_STATUS_RALT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_ALT:
	at_keyboard_status |= GRUB_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
  else
    switch (KEYBOARD_SCANCODE (key))
      {
      case GRUB_KEYBOARD_KEY_LEFT_SHIFT:
	at_keyboard_status &= ~GRUB_TERM_STATUS_LSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_SHIFT:
	at_keyboard_status &= ~GRUB_TERM_STATUS_RSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_CTRL:
	at_keyboard_status &= ~GRUB_TERM_STATUS_LCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_CTRL:
	at_keyboard_status &= ~GRUB_TERM_STATUS_RCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_ALT:
	at_keyboard_status &= ~GRUB_TERM_STATUS_RALT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_ALT:
	at_keyboard_status &= ~GRUB_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
}

/* If there is a raw key pending, return it; otherwise return -1.  */
static int
grub_keyboard_getkey (void)
{
  int key;
  int is_break;

  key = fetch_key (&is_break);
  if (key == -1)
    return -1;

  if (grub_keyboard_isr (key, is_break))
    return -1;
  if (is_break)
    return -1;
  return key;
}

/* If there is a character pending, return it; otherwise return -1.  */
static int
grub_at_keyboard_getkey_noblock (void)
{
  int code;
  code = grub_keyboard_getkey ();
  if (code == -1)
    return -1;
#ifdef DEBUG_AT_KEYBOARD
  grub_dprintf ("atkeyb", "Detected key 0x%x\n", key);
#endif
  switch (code)
    {
      case GRUB_KEYBOARD_KEY_CAPS_LOCK:
	at_keyboard_status ^= GRUB_TERM_STATUS_CAPS;
	led_status ^= KEYBOARD_LED_CAPS;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	grub_dprintf ("atkeyb", "caps_lock = %d\n", !!(at_keyboard_status & KEYBOARD_STATUS_CAPS_LOCK));
#endif
	return -1;
      case GRUB_KEYBOARD_KEY_NUM_LOCK:
	at_keyboard_status ^= GRUB_TERM_STATUS_NUM;
	led_status ^= KEYBOARD_LED_NUM;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	grub_dprintf ("atkeyb", "num_lock = %d\n", !!(at_keyboard_status & KEYBOARD_STATUS_NUM_LOCK));
#endif
	return -1;
      case GRUB_KEYBOARD_KEY_SCROLL_LOCK:
	at_keyboard_status ^= GRUB_TERM_STATUS_SCROLL;
	led_status ^= KEYBOARD_LED_SCROLL;
	keyboard_controller_led (led_status);
	return -1;
      default:
	return grub_term_map_key (code, at_keyboard_status);
    }
}

static int
grub_at_keyboard_checkkey (struct grub_term_input *term __attribute__ ((unused)))
{
  if (pending_key != -1)
    return 1;

  pending_key = grub_at_keyboard_getkey_noblock ();

  if (pending_key != -1)
    return 1;

  return -1;
}

static int
grub_at_keyboard_getkey (struct grub_term_input *term __attribute__ ((unused)))
{
  int key;
  if (pending_key != -1)
    {
      key = pending_key;
      pending_key = -1;
      return key;
    }
  do
    {
      key = grub_at_keyboard_getkey_noblock ();
    } while (key == -1);
  return key;
}

static grub_err_t
grub_keyboard_controller_init (struct grub_term_input *term __attribute__ ((unused)))
{
  pending_key = -1;
  at_keyboard_status = 0;
  grub_keyboard_controller_orig = grub_keyboard_controller_read ();
  set_scancodes ();
  keyboard_controller_led (led_status);
  /* Drain input buffer. */
  while (KEYBOARD_ISREADY (grub_inb (KEYBOARD_REG_STATUS)))
    grub_inb (KEYBOARD_REG_DATA);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_keyboard_controller_fini (struct grub_term_input *term __attribute__ ((unused)))
{
  query_mode (grub_keyboard_orig_set);
  grub_keyboard_controller_write (grub_keyboard_controller_orig);
  return GRUB_ERR_NONE;
}

static struct grub_term_input grub_at_keyboard_term =
  {
    .name = "at_keyboard",
    .init = grub_keyboard_controller_init,
    .fini = grub_keyboard_controller_fini,
    .checkkey = grub_at_keyboard_checkkey,
    .getkey = grub_at_keyboard_getkey
  };

GRUB_MOD_INIT(at_keyboard)
{
  grub_term_register_input ("at_keyboard", &grub_at_keyboard_term);
}

GRUB_MOD_FINI(at_keyboard)
{
  grub_term_unregister_input (&grub_at_keyboard_term);
}
