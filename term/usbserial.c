/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/serial.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/terminfo.h>
#include <grub/cpu/io.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

static unsigned int registered = 0;

/* Argument options.  */
static const struct grub_arg_option options[] =
{
  {"unit",   'u', 0, N_("Set the serial unit."),             0, ARG_TYPE_INT},
  {"speed",  's', 0, N_("Set the serial port speed."),       0, ARG_TYPE_INT},
  {"word",   'w', 0, N_("Set the serial port word length."), 0, ARG_TYPE_INT},
  {"parity", 'r', 0, N_("Set the serial port parity."),      0, ARG_TYPE_STRING},
  {"stop",   't', 0, N_("Set the serial port stop bits."),   0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

/* Serial port settings.  */
struct usbserial_port
{
  grub_usb_device_t usbdev;
  int in_endp;
  int out_endp;
  unsigned short divisor;
  unsigned short word_len;
  unsigned int   parity;
  unsigned short stop_bits;
};

/* Serial port settings.  */
static struct serial_port serial_settings;

/* Fetch a key.  */
static int
serial_hw_fetch (void)
{
  /* FIXME */
  return -1;
}

/* Put a character.  */
static void
usbserial_hw_put (struct usbserial_port *port, const int c)
{
  char cc = c;
  grub_usb_bulk_write (port->usbdev, port->out_endp, 1, &cc);
}

static grub_uint16_t
grub_serial_getwh (struct grub_term_output *term __attribute__ ((unused)))
{
  const grub_uint8_t TEXT_WIDTH = 80;
  const grub_uint8_t TEXT_HEIGHT = 24;
  return (TEXT_WIDTH << 8) | TEXT_HEIGHT;
}

struct grub_terminfo_input_state grub_serial_terminfo_input =
  {
    .readkey = serial_hw_fetch
  };

struct grub_terminfo_output_state grub_serial_terminfo_output =
  {
    .put = usbserial_hw_put
  };

static struct grub_term_input grub_serial_term_input =
{
  .name = "usbserial",
  .init = grub_terminfo_input_init,
  .checkkey = grub_terminfo_checkkey,
  .getkey = grub_terminfo_getkey,
  .data = &grub_serial_terminfo_input
};

static struct grub_term_output grub_serial_term_output =
{
  .name = "usbserial",
  .putchar = grub_terminfo_putchar,
  .getwh = grub_serial_getwh,
  .getxy = grub_terminfo_getxy,
  .gotoxy = grub_terminfo_gotoxy,
  .cls = grub_terminfo_cls,
  .setcolorstate = grub_terminfo_setcolorstate,
  .setcursor = grub_terminfo_setcursor,
  .flags = GRUB_TERM_CODE_TYPE_ASCII,
  .data = &grub_serial_terminfo_output,
  .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
  .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
};




static grub_extcmd_t cmd;

GRUB_MOD_INIT(serial)
{
  cmd = grub_register_extcmd ("serial", grub_cmd_serial,
			      GRUB_COMMAND_FLAG_BOTH,
			      N_("[OPTIONS...]"),
			      N_("Configure serial port."), options);

  /* Set default settings.  */
  serial_settings.port      = serial_hw_get_port (0);
#ifdef GRUB_MACHINE_MIPS_YEELOONG
  serial_settings.divisor   = serial_get_divisor (115200);
#else
  serial_settings.divisor   = serial_get_divisor (9600);
#endif
  serial_settings.word_len  = UART_8BITS_WORD;
  serial_settings.parity    = UART_NO_PARITY;
  serial_settings.stop_bits = UART_1_STOP_BIT;

#ifdef GRUB_MACHINE_MIPS_YEELOONG
  {
    grub_err_t hwiniterr;
    hwiniterr = serial_hw_init ();

    if (hwiniterr == GRUB_ERR_NONE)
      {
	grub_term_register_input_active ("serial", &grub_serial_term_input);
	grub_term_register_output_active ("serial", &grub_serial_term_output);

	registered = 1;
      }
  }
#endif
}

GRUB_MOD_FINI(serial)
{
  grub_unregister_extcmd (cmd);
  if (registered == 1)		/* Unregister terminal only if registered. */
    {
      grub_term_unregister_input (&grub_serial_term_input);
      grub_term_unregister_output (&grub_serial_term_output);
      grub_terminfo_output_unregister (&grub_serial_term_output);
    }
}
