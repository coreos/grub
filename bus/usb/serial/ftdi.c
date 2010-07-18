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

enum
  {
    GRUB_FTDI_MODEM_CTRL = 0x01,
    GRUB_FTDI_FLOW_CTRL = 0x02,
    GRUB_FTDI_SPEED_CTRL = 0x03,
    GRUB_FTDI_DATA_CTRL = 0x04
  };

#define GRUB_FTDI_MODEM_CTRL_DTRRTS 3
#define GRUB_FTDI_FLOW_CTRL_DTRRTS 3

/* Convert speed to divisor.  */
static grub_uint32_t
get_divisor (unsigned int speed)
{
  unsigned int i;

  /* The structure for speed vs. divisor.  */
  struct divisor
  {
    unsigned int speed;
    grub_uint32_t div;
  };

  /* The table which lists common configurations.  */
  /* Computed with a division formula with 3MHz as base frequency. */
  static struct divisor divisor_tab[] =
    {
      { 2400,   0x04e2 },
      { 4800,   0x0271 },
      { 9600,   0x4138 },
      { 19200,  0x809c },
      { 38400,  0xc04e },
      { 57600,  0xc034 },
      { 115200, 0x001a }
    };

  /* Set the baud rate.  */
  for (i = 0; i < ARRAY_SIZE (divisor_tab); i++)
    if (divisor_tab[i].speed == speed)
      return divisor_tab[i].div;
  return 0;
}

static void
real_config (struct grub_serial_port *port)
{
  grub_uint32_t divisor;
  const grub_uint16_t parities[] = {
    [GRUB_SERIAL_PARITY_NONE] = 0x0000,
    [GRUB_SERIAL_PARITY_ODD] = 0x0100,
    [GRUB_SERIAL_PARITY_EVEN] = 0x0200
  };
  const grub_uint16_t stop_bits[] = {
    [GRUB_SERIAL_STOP_BITS_1] = 0x0000,
    [GRUB_SERIAL_STOP_BITS_2] = 0x1000,
  };

  if (port->configured)
    return;

  grub_usb_control_msg (port->usbdev, GRUB_USB_REQTYPE_VENDOR_OUT,
			GRUB_FTDI_MODEM_CTRL,
			GRUB_FTDI_MODEM_CTRL_DTRRTS, 0, 0, 0);

  grub_usb_control_msg (port->usbdev, GRUB_USB_REQTYPE_VENDOR_OUT,
			GRUB_FTDI_FLOW_CTRL,
			GRUB_FTDI_FLOW_CTRL_DTRRTS, 0, 0, 0);

  divisor = get_divisor (port->config.speed);
  grub_usb_control_msg (port->usbdev, GRUB_USB_REQTYPE_VENDOR_OUT,
			GRUB_FTDI_SPEED_CTRL,
			divisor & 0xffff, divisor >> 16, 0, 0);

  grub_usb_control_msg (port->usbdev, GRUB_USB_REQTYPE_VENDOR_OUT,
			GRUB_FTDI_DATA_CTRL,
			parities[port->config.parity]
			| stop_bits[port->config.stop_bits]
			| port->config.word_len, 0, 0, 0);

  port->configured = 1;
}

/* Fetch a key.  */
static int
ftdi_hw_fetch (struct grub_serial_port *port)
{
  char cc[3];
  grub_usb_err_t err;

  real_config (port);

  err = grub_usb_bulk_read (port->usbdev, port->in_endp->endp_addr, 3, cc);
  if (err != GRUB_USB_ERR_NONE)
    return -1;

  return cc[2];
}

/* Put a character.  */
static void
ftdi_hw_put (struct grub_serial_port *port, const int c)
{
  char cc = c;

  real_config (port);

  grub_usb_bulk_write (port->usbdev, port->out_endp->endp_addr, 1, &cc);
}

static grub_err_t
ftdi_hw_configure (struct grub_serial_port *port,
			struct grub_serial_config *config)
{
  grub_uint16_t divisor;

  divisor = get_divisor (config->speed);
  if (divisor == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad speed");

  if (config->parity != GRUB_SERIAL_PARITY_NONE
      && config->parity != GRUB_SERIAL_PARITY_ODD
      && config->parity != GRUB_SERIAL_PARITY_EVEN)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unsupported parity");

  if (config->stop_bits != GRUB_SERIAL_STOP_BITS_1
      && config->stop_bits != GRUB_SERIAL_STOP_BITS_2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unsupported stop bits");

  if (config->word_len < 5 || config->word_len > 8)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unsupported word length");

  port->config = *config;
  port->configured = 0;

  return GRUB_ERR_NONE;
}

static void
ftdi_fini (struct grub_serial_port *port)
{
  port->usbdev->config[port->configno].interf[port->interfno].detach_hook = 0;
  port->usbdev->config[port->configno].interf[port->interfno].attached = 0;
}

static struct grub_serial_driver grub_ftdi_driver =
  {
    .configure = ftdi_hw_configure,
    .fetch = ftdi_hw_fetch,
    .put = ftdi_hw_put,
    .fini = ftdi_fini
  };

static const struct 
{
  grub_uint16_t vendor, product;
} products[] =
  {
    {0x0403, 0x6001} /* QEMU virtual USBserial.  */
  };

static int ftdinum = 0;

static void
ftdi_detach (grub_usb_device_t usbdev, int configno, int interfno)
{
  static struct grub_serial_port *port;
  port = usbdev->config[configno].interf[interfno].detach_data;

  grub_serial_unregister (port);
}

static int
grub_ftdi_attach_real (grub_usb_device_t usbdev, int configno, int interfno)
{
  static struct grub_serial_port *port;
  int j;
  struct grub_usb_desc_if *interf;

  interf = usbdev->config[configno].interf[interfno].descif;

  port = grub_malloc (sizeof (*port));
  if (!port)
    {
      grub_print_error ();
      return 0;
    }

  port->name = grub_xasprintf ("ftdi%d", ftdinum++);
  if (!port->name)
    {
      grub_free (port);
      grub_print_error ();
      return 0;
    }

  port->usbdev = usbdev;
  port->driver = &grub_ftdi_driver;
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

  port->configno = configno;
  port->interfno = interfno;

  grub_serial_config_defaults (port);
  grub_serial_register (port);

  port->usbdev->config[port->configno].interf[port->interfno].detach_hook
    = ftdi_detach;
  port->usbdev->config[port->configno].interf[port->interfno].detach_data
    = port;

  return 1;
}

static int
grub_ftdi_attach (grub_usb_device_t usbdev, int configno, int interfno)
{
  unsigned j;

  for (j = 0; j < ARRAY_SIZE (products); j++)
    if (usbdev->descdev.vendorid == products[j].vendor
	&& usbdev->descdev.prodid == products[j].product)
      break;
  if (j == ARRAY_SIZE (products))
    return 0;

  return grub_ftdi_attach_real (usbdev, configno, interfno);
}

struct grub_usb_attach_desc attach_hook =
{
  .class = 0xff,
  .hook = grub_ftdi_attach
};

GRUB_MOD_INIT(usbserial_ftdi)
{
  grub_usb_register_attach_hook_class (&attach_hook);
}

GRUB_MOD_FINI(usbserial_ftdi)
{
  grub_serial_unregister_driver (&grub_ftdi_driver);
  grub_usb_unregister_attach_hook_class (&attach_hook);
}
