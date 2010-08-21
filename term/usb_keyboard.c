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
  /* According to usage table 0x31 should be remapped to 0x2b
     but testing with real keyboard shows that 0x32 is remapped to 0x2b.  */
  /* 0x30 */ 0x1b /* ] */,          0x00, 
  /* 0x32 */ 0x2b /* \ */,          0x27 /* ; */, 
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
  /* 0x48 */ 0x00,                  0x52 /* Insert */, 
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

enum
  {
    KEY_NO_KEY = 0x00,
    KEY_ERR_BUFFER = 0x01,
    KEY_ERR_POST  = 0x02,
    KEY_ERR_UNDEF = 0x03,
    KEY_CAPS_LOCK = 0x39,
    KEY_NUM_LOCK  = 0x53,
  };

enum
  {
    LED_NUM_LOCK = 0x01,
    LED_CAPS_LOCK = 0x02
  };

/* Valid values for bRequest.  See HID definition version 1.11 section 7.2. */
#define USB_HID_GET_REPORT	0x01
#define USB_HID_GET_IDLE	0x02
#define USB_HID_GET_PROTOCOL	0x03
#define USB_HID_SET_REPORT	0x09
#define USB_HID_SET_IDLE	0x0A
#define USB_HID_SET_PROTOCOL	0x0B

#define USB_HID_BOOT_SUBCLASS	0x01
#define USB_HID_KBD_PROTOCOL	0x01

#define GRUB_USB_KEYBOARD_LEFT_CTRL   0x01
#define GRUB_USB_KEYBOARD_LEFT_SHIFT  0x02
#define GRUB_USB_KEYBOARD_LEFT_ALT    0x04
#define GRUB_USB_KEYBOARD_RIGHT_CTRL  0x10
#define GRUB_USB_KEYBOARD_RIGHT_SHIFT 0x20
#define GRUB_USB_KEYBOARD_RIGHT_ALT   0x40

struct grub_usb_keyboard_data
{
  grub_usb_device_t usbdev;
  grub_uint8_t status;
  grub_uint16_t mods;
  int key;
  int interfno;
  struct grub_usb_desc_endp *endp;
  grub_usb_transfer_t transfer;
  grub_uint8_t report[8];
  int dead;
};

static struct grub_term_input grub_usb_keyboards[16];

static int grub_usb_keyboard_checkkey (struct grub_term_input *term);
static int grub_usb_keyboard_getkey (struct grub_term_input *term);
static int grub_usb_keyboard_getkeystatus (struct grub_term_input *term);

static struct grub_term_input grub_usb_keyboard_term =
  {
    .checkkey = grub_usb_keyboard_checkkey,
    .getkey = grub_usb_keyboard_getkey,
    .getkeystatus = grub_usb_keyboard_getkeystatus,
    .next = 0
  };

static struct grub_term_input grub_usb_keyboards[16];

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
grub_usb_keyboard_detach (grub_usb_device_t usbdev,
			  int config __attribute__ ((unused)),
			  int interface __attribute__ ((unused)))
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (grub_usb_keyboards); i++)
    {
      struct grub_usb_keyboard_data *data = grub_usb_keyboards[i].data;

      if (!data)
	continue;

      if (data->usbdev != usbdev)
	continue;

      if (data->transfer)
	grub_usb_cancel_transfer (data->transfer);

      grub_term_unregister_input (&grub_usb_keyboards[i]);
      grub_free ((char *) grub_usb_keyboards[i].name);
      grub_usb_keyboards[i].name = NULL;
      grub_free (grub_usb_keyboards[i].data);
      grub_usb_keyboards[i].data = 0;
    }
}

static int
grub_usb_keyboard_attach (grub_usb_device_t usbdev, int configno, int interfno)
{
  unsigned curnum;
  struct grub_usb_keyboard_data *data;
  struct grub_usb_desc_endp *endp = NULL;
  int j;

  grub_dprintf ("usb_keyboard", "%x %x %x %d %d\n",
		usbdev->descdev.class, usbdev->descdev.subclass,
		usbdev->descdev.protocol, configno, interfno);

  for (curnum = 0; curnum < ARRAY_SIZE (grub_usb_keyboards); curnum++)
    if (!grub_usb_keyboards[curnum].data)
      break;

  if (curnum == ARRAY_SIZE (grub_usb_keyboards))
    return 0;

  if (usbdev->descdev.class != 0 
      || usbdev->descdev.subclass != 0 || usbdev->descdev.protocol != 0)
    return 0;

  if (usbdev->config[configno].interf[interfno].descif->subclass
      != USB_HID_BOOT_SUBCLASS
      || usbdev->config[configno].interf[interfno].descif->protocol
      != USB_HID_KBD_PROTOCOL)
    return 0;

  for (j = 0; j < usbdev->config[configno].interf[interfno].descif->endpointcnt;
       j++)
    {
      endp = &usbdev->config[configno].interf[interfno].descendp[j];

      if ((endp->endp_addr & 128) && grub_usb_get_ep_type(endp)
	  == GRUB_USB_EP_INTERRUPT)
	break;
    }
  if (j == usbdev->config[configno].interf[interfno].descif->endpointcnt)
    return 0;

  grub_dprintf ("usb_keyboard", "HID found!\n");

  data = grub_malloc (sizeof (*data));
  if (!data)
    {
      grub_print_error ();
      return 0;
    }

  data->usbdev = usbdev;
  data->interfno = interfno;
  data->endp = endp;

  /* Place the device in boot mode.  */
  grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_PROTOCOL, 0, interfno, 0, 0);

  /* Reports every time an event occurs and not more often than that.  */
  grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_IDLE, 0<<8, interfno, 0, 0);

  grub_memcpy (&grub_usb_keyboards[curnum], &grub_usb_keyboard_term,
	       sizeof (grub_usb_keyboards[curnum]));
  grub_usb_keyboards[curnum].data = data;
  usbdev->config[configno].interf[interfno].detach_hook
    = grub_usb_keyboard_detach;
  grub_usb_keyboards[curnum].name = grub_xasprintf ("usb_keyboard%d", curnum);
  if (!grub_usb_keyboards[curnum].name)
    {
      grub_print_error ();
      return 0;
    }

  /* Test showed that getting report may make the keyboard go nuts.
     Moreover since we're reattaching keyboard it usually sends
     an initial message on interrupt pipe and so we retrieve
     the same keystatus.
   */
#if 0
  {
    grub_uint8_t report[8];
    grub_usb_err_t err;
    grub_memset (report, 0, sizeof (report));
    err = grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_IN,
    				USB_HID_GET_REPORT, 0x0100, interfno,
				sizeof (report), (char *) report);
    if (err)
      {
	data->status = 0;
	data->key = -1;
      }
    else
      {
	data->status = report[0];
	data->key = report[2] ? : -1;
      }
  }
#else
  data->status = 0;
  data->key = -1;
#endif

  data->transfer = grub_usb_bulk_read_background (usbdev,
						  data->endp->endp_addr,
						  sizeof (data->report),
						  (char *) data->report);
  if (!data->transfer)
    {
      grub_print_error ();
      return 0;
    }

  data->mods = 0;

  grub_term_register_input_active ("usb_keyboard", &grub_usb_keyboards[curnum]);

  data->dead = 0;

  return 1;
}



static void
send_leds (struct grub_usb_keyboard_data *termdata)
{
  char report[1];
  report[0] = 0;
  if (termdata->mods & GRUB_TERM_STATUS_CAPS)
    report[0] |= LED_CAPS_LOCK;
  if (termdata->mods & GRUB_TERM_STATUS_NUM)
    report[0] |= LED_NUM_LOCK;
  grub_usb_control_msg (termdata->usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_OUT,
			USB_HID_SET_REPORT, 0x0200, termdata->interfno,
			sizeof (report), (char *) report);
  grub_errno = GRUB_ERR_NONE;
}

static int
grub_usb_keyboard_checkkey (struct grub_term_input *term)
{
  grub_usb_err_t err;
  struct grub_usb_keyboard_data *termdata = term->data;
  grub_uint8_t data[sizeof (termdata->report)];
  grub_size_t actual;

  if (termdata->key != -1)
    return termdata->key;

  if (termdata->dead)
    return -1;

  /* Poll interrupt pipe.  */
  err = grub_usb_check_transfer (termdata->transfer, &actual);

  if (err == GRUB_USB_ERR_WAIT)
    return -1;

  grub_memcpy (data, termdata->report, sizeof (data));

  termdata->transfer = grub_usb_bulk_read_background (termdata->usbdev,
						      termdata->endp->endp_addr,
						      sizeof (termdata->report),
						      (char *) termdata->report);
  if (!termdata->transfer)
    {
      grub_printf ("%s failed. Stopped\n", term->name);
      termdata->dead = 1;
    }


  grub_dprintf ("usb_keyboard",
		"err = %d, actual = %d report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		err, actual,
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  if (err || actual < 1)
    return -1;

  termdata->status = data[0];

  if (actual < 3)
    return -1;

  if (data[2] == KEY_NO_KEY || data[2] == KEY_ERR_BUFFER
      || data[2] == KEY_ERR_POST || data[2] == KEY_ERR_UNDEF)
    return -1;

  if (data[2] == KEY_CAPS_LOCK)
    {
      termdata->mods ^= GRUB_TERM_STATUS_CAPS;
      send_leds (termdata);
      return -1;
    }

  if (data[2] == KEY_NUM_LOCK)
    {
      termdata->mods ^= GRUB_TERM_STATUS_NUM;
      send_leds (termdata);
      return -1;
    }

  if (data[2] > ARRAY_SIZE (usb_to_at_map) || usb_to_at_map[data[2]] == 0)
    grub_printf ("Unknown key 0x%02x detected\n", data[2]);
  else
    termdata->key = grub_term_map_key (usb_to_at_map[data[2]],
				       interpret_status (data[0])
				       | termdata->mods);

  grub_errno = GRUB_ERR_NONE;

  return termdata->key;
}

static int
grub_usb_keyboard_getkey (struct grub_term_input *term)
{
  int ret;
  struct grub_usb_keyboard_data *termdata = term->data;

  while (termdata->key == -1)
    grub_usb_keyboard_checkkey (term);

  ret = termdata->key;

  termdata->key = -1;

  return ret;
}

static int
grub_usb_keyboard_getkeystatus (struct grub_term_input *term)
{
  struct grub_usb_keyboard_data *termdata = term->data;

  grub_usb_keyboard_checkkey (term);

  return interpret_status (termdata->status) | termdata->mods;
}

struct grub_usb_attach_desc attach_hook =
{
  .class = GRUB_USB_CLASS_HID,
  .hook = grub_usb_keyboard_attach
};

GRUB_MOD_INIT(usb_keyboard)
{
  grub_usb_register_attach_hook_class (&attach_hook);
}

GRUB_MOD_FINI(usb_keyboard)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (grub_usb_keyboards); i++)
    if (grub_usb_keyboards[i].data)
      {
	struct grub_usb_keyboard_data *data = grub_usb_keyboards[i].data;

	if (!data)
	  continue;
	
	if (data->transfer)
	  grub_usb_cancel_transfer (data->transfer);

	grub_term_unregister_input (&grub_usb_keyboards[i]);
	grub_free ((char *) grub_usb_keyboards[i].name);
	grub_usb_keyboards[i].name = NULL;
	grub_free (grub_usb_keyboards[i].data);
	grub_usb_keyboards[i].data = 0;
      }
  grub_usb_unregister_attach_hook_class (&attach_hook);
}
