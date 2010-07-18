/* serial.h - serial device interface */
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

#ifndef GRUB_SERIAL_HEADER
#define GRUB_SERIAL_HEADER	1

#include <grub/types.h>
#include <grub/cpu/io.h>
#include <grub/usb.h>
#include <grub/list.h>

struct grub_serial_port;

struct grub_serial_driver
{
  grub_err_t (*configure) (struct grub_serial_port *port,
			   unsigned speed,
			   unsigned short word_len,
			   unsigned int   parity,
			   unsigned short stop_bits);
  int (*fetch) (struct grub_serial_port *port);
  void (*put) (struct grub_serial_port *port, const int c);
};

struct grub_serial_port
{
  struct grub_serial_port *next;
  char *name;
  unsigned speed;
  unsigned short word_len;
  unsigned int   parity;
  unsigned short stop_bits;
  struct grub_serial_driver *driver;
  int configured;
  /* This should be void *data but since serial is useful as an early console
     when malloc isn't available it's a union.
   */
  union
  {
    grub_port_t port;
    struct
    {
      grub_usb_device_t usbdev;
      struct grub_usb_desc_endp *in_endp;
      struct grub_usb_desc_endp *out_endp;
    };
  };
};

grub_err_t grub_serial_register (struct grub_serial_port *port);

void grub_serial_unregister (struct grub_serial_port *port);

/* The type of word length.  */
#define UART_5BITS_WORD	0x00
#define UART_6BITS_WORD	0x01
#define UART_7BITS_WORD	0x02
#define UART_8BITS_WORD	0x03

/* The type of parity.  */
#define UART_NO_PARITY		0x00
#define UART_ODD_PARITY		0x08
#define UART_EVEN_PARITY	0x18

/* The type of the length of stop bit.  */
#define UART_1_STOP_BIT		0x00
#define UART_2_STOP_BITS	0x04

  /* Set default settings.  */
static inline grub_err_t
grub_serial_config_defaults (struct grub_serial_port *port)
{
  return port->driver->configure (port,

#ifdef GRUB_MACHINE_MIPS_YEELOONG
				  115200,
#else
				  9600,
#endif
				  UART_8BITS_WORD, UART_NO_PARITY,
				  UART_1_STOP_BIT);
}

void grub_ns8250_init (void);
char *grub_serial_ns8250_add_port (grub_port_t port);

#endif
