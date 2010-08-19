/* Support for the HID Boot Protocol.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008, 2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/time.h>
#include <grub/cpu/io.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/usb.h>
#include <grub/dl.h>
#include <grub/time.h>
#include <grub/keyboard_layouts.h>



static grub_uint8_t usb_to_at_map[128] =
{
  /* 0x00 */ 0x00,                  0x00,
  /* 0x02 */ 0x00,                  0x00, 
  /* 0x04 */ 0x1e /* a */,          0x30 /* b */,
  /* 0x06 */ 0x2e /* c */,          0x20 /* d */, 
  /* 0x08 */ 0x12 /* e */,          0x21 /* f */,
  /* 0x0a */ 0x22 /* g */,          0x23 /* h */, 
  /* 0x0c */ 0x17 /* i */,          0x24 /* j */,
  /* 0x0e */ 0x25 /* k */,          0x26 /* l */, 
  /* 0x10 */ 0x32 /* m */,          0x31 /* n */, 
  /* 0x12 */ 0x18 /* o */,          0x19 /* p */, 
  /* 0x14 */ 0x10 /* q */,          0x13 /* r */,
  /* 0x16 */ 0x1f /* s */,          0x14 /* t */, 
  /* 0x18 */ 0x16 /* u */,          0x2f /* v */,
  /* 0x1a */ 0x11 /* w */,          0x2d /* x */, 
  /* 0x1c */ 0x15 /* y */,          0x2c /* z */,
  /* 0x1e */ 0x02 /* 1 */,          0x03 /* 2 */, 
  /* 0x20 */ 0x04 /* 3 */,          0x05 /* 4 */, 
  /* 0x22 */ 0x06 /* 5 */,          0x07 /* 6 */, 
  /* 0x24 */ 0x08 /* 7 */,          0x09 /* 8 */,
  /* 0x26 */ 0x0a /* 9 */,          0x0b /* 0 */, 
  /* 0x28 */ 0x1c /* Enter */,      0x01 /* Escape */,
  /* 0x2a */ 0x0e /* \b */,         0x0f /* \t */, 
  /* 0x2c */ 0x39 /* Space */,      0x0c /* - */,
  /* 0x2e */ 0x0d /* = */,          0x1a /* [ */, 
  /* 0x30 */ 0x1b /* ] */,          0x2b /* \ */, 
  /* 0x32 */ 0x00,                  0x27 /* ; */, 
  /* 0x34 */ 0x28 /* " */,          0x29 /* ` */,
  /* 0x36 */ 0x33 /* , */,          0x34 /* . */, 
  /* 0x38 */ 0x35 /* / */,          0x00,
  /* 0x3a */ 0x3b /* F1 */,         0x3c /* F2 */, 
  /* 0x3c */ 0x3d /* F3 */,         0x3e /* F4 */,
  /* 0x3e */ 0x3f /* F5 */,         0x40 /* F6 */,
  /* 0x40 */ 0x41 /* F7 */,         0x42 /* F8 */,
  /* 0x42 */ 0x43 /* F9 */,         0x44 /* F10 */, 
  /* 0x44 */ 0x57 /* F11 */,        0x58 /* F12 */,
  /* 0x46 */ 0x00,                  0x00, 
  /* 0x48 */ 0x00,                  0x00, 
  /* 0x4a */ 0x47 /* HOME */,       0x51 /* PPAGE */, 
  /* 0x4c */ 0x53 /* DC */,         0x4f /* END */, 
  /* 0x4e */ 0x49 /* NPAGE */,      0x4d /* RIGHT */, 
  /* 0x50 */ 0x4b /* LEFT */,       0x50 /* DOWN */, 
  /* 0x52 */ 0x48 /* UP */,         0x00, 
  /* 0x54 */ 0x00,                  0x00, 
  /* 0x56 */ 0x00,                  0x00, 
  /* 0x58 */ 0x00,                  0x00, 
  /* 0x5a */ 0x00,                  0x00, 
  /* 0x5c */ 0x00,                  0x00, 
  /* 0x5e */ 0x00,                  0x00, 
  /* 0x60 */ 0x00,                  0x00, 
  /* 0x62 */ 0x00,                  0x00, 
  /* 0x64 */ 0x56 /* 102nd key. */, 0x00, 
  /* 0x66 */ 0x00,                  0x00, 
  /* 0x68 */ 0x00,                  0x00, 
  /* 0x6a */ 0x00,                  0x00, 
  /* 0x6c */ 0x00,                  0x00, 
  /* 0x6e */ 0x00,                  0x00, 
  /* 0x70 */ 0x00,                  0x00,
  /* 0x72 */ 0x00,                  0x00, 
  /* 0x74 */ 0x00,                  0x00,
  /* 0x76 */ 0x00,                  0x00, 
  /* 0x78 */ 0x00,                  0x00,
  /* 0x7a */ 0x00,                  0x00, 
  /* 0x7c */ 0x00,                  0x00,
  /* 0x7e */ 0x00,                  0x00, 
};

static grub_usb_device_t usbdev;

/* Valid values for bmRequestType.  See HID definition version 1.11 section
   7.2.  */
#define USB_HID_HOST_TO_DEVICE	0x21
#define USB_HID_DEVICE_TO_HOST	0xA1

/* Valid values for bRequest.  See HID definition version 1.11 section 7.2. */
#define USB_HID_GET_REPORT	0x01
#define USB_HID_GET_IDLE	0x02
#define USB_HID_GET_PROTOCOL	0x03
#define USB_HID_SET_REPORT	0x09
#define USB_HID_SET_IDLE	0x0A
#define USB_HID_SET_PROTOCOL	0x0B

#define GRUB_USB_KEYBOARD_LEFT_CTRL   0x01
#define GRUB_USB_KEYBOARD_LEFT_SHIFT  0x02
#define GRUB_USB_KEYBOARD_LEFT_ALT    0x04
#define GRUB_USB_KEYBOARD_RIGHT_CTRL  0x10
#define GRUB_USB_KEYBOARD_RIGHT_SHIFT 0x20
#define GRUB_USB_KEYBOARD_RIGHT_ALT   0x40

static int
interpret_status (grub_uint8_t data0)
{
  int mods = 0;

  /* Check Shift, Control, and Alt status.  */
  if (data0 & GRUB_USB_KEYBOARD_LEFT_SHIFT)
    mods |= GRUB_TERM_STATUS_LSHIFT;
  if (data0 & GRUB_USB_KEYBOARD_RIGHT_SHIFT)
    mods |= GRUB_TERM_STATUS_RSHIFT;
  if (data0 & GRUB_USB_KEYBOARD_LEFT_CTRL)
    mods |= GRUB_TERM_STATUS_LCTRL;
  if (data0 & GRUB_USB_KEYBOARD_RIGHT_CTRL)
    mods |= GRUB_TERM_STATUS_RCTRL;
  if (data0 & GRUB_USB_KEYBOARD_LEFT_ALT)
    mods |= GRUB_TERM_STATUS_LALT;
  if (data0 & GRUB_USB_KEYBOARD_RIGHT_ALT)
    mods |= GRUB_TERM_STATUS_RALT;

  return mods;
}

static void
grub_usb_hid (void)
{
  struct grub_usb_desc_device *descdev;

  auto int usb_iterate (grub_usb_device_t dev);
  int usb_iterate (grub_usb_device_t dev)
    {
      descdev = &dev->descdev;

      grub_dprintf ("usb_keyboard", "%x %x %x\n",
		   descdev->class, descdev->subclass, descdev->protocol);

#if 0
      if (descdev->class != 0x09
	  || descdev->subclass == 0x01
	  || descdev->protocol != 0x02)
	return 0;
#endif

      if (descdev->class != 0 || descdev->subclass != 0 || descdev->protocol != 0)
	return 0;

      grub_printf ("HID found!\n");

      usbdev = dev;

      return 1;
    }
  grub_usb_iterate (usb_iterate);

  /* Place the device in boot mode.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_PROTOCOL,
			0, 0, 0, 0);

  /* Reports every time an event occurs and not more often than that.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);
}

static grub_err_t
grub_usb_keyboard_getreport (grub_usb_device_t dev, grub_uint8_t *report)
{
  return grub_usb_control_msg (dev, USB_HID_DEVICE_TO_HOST, USB_HID_GET_REPORT,
			       0, 0, 8, (char *) report);
}



static int
grub_usb_keyboard_checkkey (struct grub_term_input *term __attribute__ ((unused)))
{
  grub_uint8_t data[8];
  int key = 0;
  grub_err_t err;
  grub_uint64_t currtime;
  int timeout = 50;

  data[2] = 0;
  currtime = grub_get_time_ms ();
  do
    {
      /* Get_Report.  */
      err = grub_usb_keyboard_getreport (usbdev, data);

      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	break;
    }
  while (err || !data[2]);

  if (err || !data[2])
    return -1;

  grub_dprintf ("usb_keyboard",
		"report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  if (usb_to_at_map[data[2]] == 0)
    grub_printf ("Unknown key 0x%x detected\n", data[2]);
  else
    key = grub_term_map_key (usb_to_at_map[data[2]], interpret_status (data[0]));

  grub_errno = GRUB_ERR_NONE;

  return key;

#if 0
  /* Wait until the key is released.  */
  while (!err && data[2])
    {
      err = grub_usb_control_msg (usbdev, USB_HID_DEVICE_TO_HOST,
				  USB_HID_GET_REPORT, 0, 0,
				  sizeof (data), (char *) data);
      grub_dprintf ("usb_keyboard",
		    "report2: 0x%02x 0x%02x 0x%02x 0x%02x"
		    " 0x%02x 0x%02x 0x%02x 0x%02x\n",
		    data[0], data[1], data[2], data[3],
		    data[4], data[5], data[6], data[7]);
    }
#endif

  return key;
}

typedef enum
{
  GRUB_HIDBOOT_REPEAT_NONE,
  GRUB_HIDBOOT_REPEAT_FIRST,
  GRUB_HIDBOOT_REPEAT
} grub_usb_keyboard_repeat_t;

static int
grub_usb_keyboard_getkey (struct grub_term_input *term)
{
  int key;
  grub_err_t err;
  grub_uint8_t data[8];
  grub_uint64_t currtime;
  int timeout;
  static grub_usb_keyboard_repeat_t repeat = GRUB_HIDBOOT_REPEAT_NONE;

 again:

  do
    {
      key = grub_usb_keyboard_checkkey (term);
    } while (key == -1);

  data[2] = !0; /* Or whatever.  */
  err = 0;

  switch (repeat)
    {
    case GRUB_HIDBOOT_REPEAT_FIRST:
      timeout = 500;
      break;
    case GRUB_HIDBOOT_REPEAT:
      timeout = 50;
      break;
    default:
      timeout = 100;
      break;
    }

  /* Wait until the key is released.  */
  currtime = grub_get_time_ms ();
  while (!err && data[2])
    {
      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	{
	  if (repeat == 0)
	    repeat = 1;
	  else
	    repeat = 2;

	  grub_errno = GRUB_ERR_NONE;
	  return key;
	}

      err = grub_usb_keyboard_getreport (usbdev, data);
    }

  if (repeat)
    {
      repeat = 0;
      goto again;
    }

  repeat = 0;

  grub_errno = GRUB_ERR_NONE;

  return key;
}

static int
grub_usb_keyboard_getkeystatus (struct grub_term_input *term __attribute__ ((unused)))
{
  grub_uint8_t data[8];
  grub_err_t err;
  grub_uint64_t currtime;
  int timeout = 50;

  /* Set idle time to the minimum offered by the spec (4 milliseconds) so
     that we can find out the current state.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);

  currtime = grub_get_time_ms ();
  do
    {
      /* Get_Report.  */
      err = grub_usb_keyboard_getreport (usbdev, data);

      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	break;
    }
  while (err || !data[0]);

  /* Go back to reporting every time an event occurs and not more often than
     that.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);

  /* We allowed a while for modifiers to show up in the report, but it is
     not an error if they never did.  */
  if (err)
    return -1;

  grub_dprintf ("usb_keyboard",
		"report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  grub_errno = GRUB_ERR_NONE;

  return interpret_status (data[0]);
}

static struct grub_term_input grub_usb_keyboard_term =
  {
    .name = "usb_keyboard",
    .checkkey = grub_usb_keyboard_checkkey,
    .getkey = grub_usb_keyboard_getkey,
    .getkeystatus = grub_usb_keyboard_getkeystatus,
    .next = 0
  };

GRUB_MOD_INIT(usb_keyboard)
{
  grub_usb_hid ();
  grub_term_register_input ("usb_keyboard", &grub_usb_keyboard_term);
}

GRUB_MOD_FINI(usb_keyboard)
{
  grub_term_unregister_input (&grub_usb_keyboard_term);
}
