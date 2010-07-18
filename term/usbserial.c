/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/serial.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/usb.h>

/* Fetch a key.  */
static int
usbserial_hw_fetch (struct grub_serial_port *port)
{
  char cc[3];
  grub_usb_err_t err;
  err = grub_usb_bulk_read (port->usbdev, port->in_endp->endp_addr, 2, cc);
  if (err != GRUB_USB_ERR_NAK)
    return -1;

  err = grub_usb_bulk_read (port->usbdev, port->in_endp->endp_addr, 3, cc);
  if (err != GRUB_USB_ERR_NONE)
    return -1;

  return cc[2];
}

/* Put a character.  */
static void
usbserial_hw_put (struct grub_serial_port *port, const int c)
{
  char cc = c;
  grub_usb_bulk_write (port->usbdev, port->out_endp->endp_addr, 1, &cc);
}

/* FIXME */
static grub_err_t
usbserial_hw_configure (struct grub_serial_port *port,
			unsigned speed, unsigned short word_len,
			unsigned int   parity, unsigned short stop_bits)
{
  port->speed = speed;
  port->word_len = word_len;
  port->parity = parity;
  port->stop_bits = stop_bits;

  port->configured = 0;

  return GRUB_ERR_NONE;
}

static struct grub_serial_driver grub_usbserial_driver =
  {
    .configure = usbserial_hw_configure,
    .fetch = usbserial_hw_fetch,
    .put = usbserial_hw_put
  };

static int
grub_usbserial_attach (grub_usb_device_t usbdev, int configno, int interfno)
{
  static struct grub_serial_port *port;
  int j;
  struct grub_usb_desc_if *interf
    = usbdev->config[configno].interf[interfno].descif;

  port = grub_malloc (sizeof (*port));
  if (!port)
    {
      grub_print_error ();
      return 0;
    }

  port->name = grub_xasprintf ("usb%d", usbdev->addr);
  if (!port->name)
    {
      grub_free (port);
      grub_print_error ();
      return 0;
    }

  port->usbdev = usbdev;
  port->driver = &grub_usbserial_driver;
  for (j = 0; j < interf->endpointcnt; j++)
    {
      struct grub_usb_desc_endp *endp;
      endp = &usbdev->config[0].interf[interfno].descendp[j];

      if ((endp->endp_addr & 128) && (endp->attrib & 3) == 2)
	{
	  /* Bulk IN endpoint.  */
	  port->in_endp = endp;
	}
      else if (!(endp->endp_addr & 128) && (endp->attrib & 3) == 2)
	{
	  /* Bulk OUT endpoint.  */
	  port->out_endp = endp;
	}
    }
  if (!port->out_endp || !port->in_endp)
    {
      grub_free (port->name);
      grub_free (port);
      return 0;
    }

  grub_serial_config_defaults (port);
  grub_serial_register (port);

  return 1;
}

struct grub_usb_attach_desc attach_hook =
{
  .class = 0xff,
  .hook = grub_usbserial_attach
};

GRUB_MOD_INIT(usbserial)
{
  grub_usb_register_attach_hook_class (&attach_hook);
}
