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
static int extended_pending = 0;
static int pending_key = -1;

static grub_uint8_t led_status;

#define KEYBOARD_LED_SCROLL		(1 << 0)
#define KEYBOARD_LED_NUM		(1 << 1)
#define KEYBOARD_LED_CAPS		(1 << 2)

static grub_uint8_t grub_keyboard_controller_orig;

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

/* FIXME: This should become an interrupt service routine.  For now
   it's just used to catch events from control keys.  */
static void
grub_keyboard_isr (grub_uint8_t key)
{
  if (KEYBOARD_ISMAKE (key))
    switch (KEYBOARD_SCANCODE (key))
      {
	case SHIFT_L:
	  at_keyboard_status |= GRUB_TERM_STATUS_LSHIFT;
	  break;
	case SHIFT_R:
	  at_keyboard_status |= GRUB_TERM_STATUS_RSHIFT;
	  break;
	case CTRL:
	  at_keyboard_status |= GRUB_TERM_STATUS_LCTRL;
	  break;
	case ALT:
	  if (extended_pending)
	    at_keyboard_status |= GRUB_TERM_STATUS_RALT;
	  else
	    at_keyboard_status |= GRUB_TERM_STATUS_LALT;
	  break;
      }
  else
    switch (KEYBOARD_SCANCODE (key))
      {
	case SHIFT_L:
	  at_keyboard_status &= ~GRUB_TERM_STATUS_LSHIFT;
	  break;
	case SHIFT_R:
	  at_keyboard_status &= ~GRUB_TERM_STATUS_RSHIFT;
	  break;
	case CTRL:
	  at_keyboard_status &= ~GRUB_TERM_STATUS_LCTRL;
	  break;
	case ALT:
	  if (extended_pending)
	    at_keyboard_status &= ~GRUB_TERM_STATUS_RALT;
	  else
	    at_keyboard_status &= ~GRUB_TERM_STATUS_LALT;
	  break;
      }
  extended_pending = (key == 0xe0);
}

/* If there is a raw key pending, return it; otherwise return -1.  */
static int
grub_keyboard_getkey (void)
{
  grub_uint8_t key;
  if (! KEYBOARD_ISREADY (grub_inb (KEYBOARD_REG_STATUS)))
    return -1;
  key = grub_inb (KEYBOARD_REG_DATA);
  /* FIXME */ grub_keyboard_isr (key);
  if (! KEYBOARD_ISMAKE (key))
    return -1;
  return (KEYBOARD_SCANCODE (key));
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
      case CAPS_LOCK:
	/* Caps lock sends scan code twice.  Get the second one and discard it.  */
	while (grub_keyboard_getkey () == -1);

	at_keyboard_status ^= GRUB_TERM_STATUS_CAPS;
	led_status ^= KEYBOARD_LED_CAPS;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	grub_dprintf ("atkeyb", "caps_lock = %d\n", !!(at_keyboard_status & KEYBOARD_STATUS_CAPS_LOCK));
#endif
	return -1;
      case NUM_LOCK:
	/* Num lock sends scan code twice.  Get the second one and discard it.  */
	while (grub_keyboard_getkey () == -1);

	at_keyboard_status ^= GRUB_TERM_STATUS_NUM;
	led_status ^= KEYBOARD_LED_NUM;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	grub_dprintf ("atkeyb", "num_lock = %d\n", !!(at_keyboard_status & KEYBOARD_STATUS_NUM_LOCK));
#endif
	return -1;
      case SCROLL_LOCK:
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
  grub_keyboard_controller_write (grub_keyboard_controller_orig | KEYBOARD_SCANCODE_SET1);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_keyboard_controller_fini (struct grub_term_input *term __attribute__ ((unused)))
{
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
