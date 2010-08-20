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


static char keyboard_map[128] =
  {
    '\0', '\0', '\0', '\0', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '0',
    '\n', GRUB_TERM_ESC, GRUB_TERM_BACKSPACE, GRUB_TERM_TAB, ' ', '-', '=', '[',
    ']', '\\', '#', ';', '\'', '`', ',', '.',
    '/', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', GRUB_TERM_HOME, GRUB_TERM_PPAGE, GRUB_TERM_DC, GRUB_TERM_END, GRUB_TERM_NPAGE, GRUB_TERM_RIGHT,
    GRUB_TERM_LEFT, GRUB_TERM_DOWN, GRUB_TERM_UP
  };

static char keyboard_map_shift[128] =
  {
    '\0', '\0', '\0', '\0', 'A', 'B', 'C', 'D',
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
    '#', '$', '%', '^', '&', '*', '(', ')',
    '\n', '\0', '\0', '\0', ' ', '_', '+', '{',
    '}', '|', '#', ':', '"', '`', '<', '>',
    '?'
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

struct grub_usb_keyboard_data
{
  grub_usb_device_t usbdev;
  grub_uint8_t status;
  int key;
  struct grub_usb_desc_endp *endp;
};

static struct grub_term_input grub_usb_keyboards[16];

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
  data->endp = endp;

  /* Place the device in boot mode.  */
  grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_PROTOCOL, 0, 0, 0, 0);

  /* Reports every time an event occurs and not more often than that.  */
  grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_IDLE, 0<<8, 0, 0, 0);

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

  {
    grub_uint8_t report[8];
    grub_usb_err_t err;
    grub_memset (report, 0, sizeof (report));
    err = grub_usb_control_msg (usbdev, GRUB_USB_REQTYPE_CLASS_INTERFACE_IN,
    				USB_HID_GET_REPORT, 0x0000, interfno,
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

  grub_term_register_input_active ("usb_keyboard", &grub_usb_keyboards[curnum]);

  return 1;
}



static int
grub_usb_keyboard_checkkey (struct grub_term_input *term)
{
  grub_uint8_t data[8];
  grub_usb_err_t err;
  struct grub_usb_keyboard_data *termdata = term->data;
  grub_size_t actual;

  if (termdata->key != -1)
    return termdata->key;

  data[2] = 0;
  /* Poll interrupt pipe.  */
  err = grub_usb_bulk_read_extended (termdata->usbdev,
				     termdata->endp->endp_addr, sizeof (data),
				     (char *) data, 10, &actual);
  if (err || actual < 1)
    return -1;

  termdata->status = data[0];

  if (actual < 3 || !data[2])
    return -1;

  grub_dprintf ("usb_keyboard",
		"report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  /* Check if the Control or Shift key was pressed.  */
  if (data[0] & 0x01 || data[0] & 0x10)
    termdata->key = keyboard_map[data[2]] - 'a' + 1;
  else if (data[0] & 0x02 || data[0] & 0x20)
    termdata->key = keyboard_map_shift[data[2]];
  else
    termdata->key = keyboard_map[data[2]];

  if (termdata->key == 0)
    grub_printf ("Unknown key 0x%x detected\n", data[2]);

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
  int mods = 0;

  /* Check Shift, Control, and Alt status.  */
  if (termdata->status & 0x02 || termdata->status & 0x20)
    mods |= GRUB_TERM_STATUS_SHIFT;
  if (termdata->status & 0x01 || termdata->status & 0x10)
    mods |= GRUB_TERM_STATUS_CTRL;
  if (termdata->status & 0x04 || termdata->status & 0x40)
    mods |= GRUB_TERM_STATUS_ALT;

  return mods;
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
	grub_term_unregister_input (&grub_usb_keyboards[i]);
	grub_free ((char *) grub_usb_keyboards[i].name);
	grub_usb_keyboards[i].name = NULL;
	grub_usb_keyboards[i].data = 0;
      }
  grub_usb_unregister_attach_hook_class (&attach_hook);
}
