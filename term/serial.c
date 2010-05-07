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
  {"port",   'p', 0, N_("Set the serial port address."),     0, ARG_TYPE_STRING},
  {"speed",  's', 0, N_("Set the serial port speed."),       0, ARG_TYPE_INT},
  {"word",   'w', 0, N_("Set the serial port word length."), 0, ARG_TYPE_INT},
  {"parity", 'r', 0, N_("Set the serial port parity."),      0, ARG_TYPE_STRING},
  {"stop",   't', 0, N_("Set the serial port stop bits."),   0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

/* Serial port settings.  */
struct serial_port
{
  grub_port_t port;
  unsigned short divisor;
  unsigned short word_len;
  unsigned int   parity;
  unsigned short stop_bits;
};

/* Serial port settings.  */
static struct serial_port serial_settings;

#ifdef GRUB_MACHINE_PCBIOS
static const unsigned short *serial_hw_io_addr = (const unsigned short *) GRUB_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR;
#define GRUB_SERIAL_PORT_NUM 4
#else
#include <grub/machine/serial.h>
static const grub_port_t serial_hw_io_addr[] = GRUB_MACHINE_SERIAL_PORTS;
#define GRUB_SERIAL_PORT_NUM (ARRAY_SIZE(serial_hw_io_addr))
#endif

/* Return the port number for the UNITth serial device.  */
static inline grub_port_t
serial_hw_get_port (const unsigned int unit)
{
  if (unit < GRUB_SERIAL_PORT_NUM)
    return serial_hw_io_addr[unit];
  else
    return 0;
}

/* Fetch a key.  */
static int
serial_hw_fetch (void)
{
  if (grub_inb (serial_settings.port + UART_LSR) & UART_DATA_READY)
    return grub_inb (serial_settings.port + UART_RX);

  return -1;
}

/* Put a character.  */
static void
serial_hw_put (const int c)
{
  unsigned int timeout = 100000;

  /* Wait until the transmitter holding register is empty.  */
  while ((grub_inb (serial_settings.port + UART_LSR) & UART_EMPTY_TRANSMITTER) == 0)
    {
      if (--timeout == 0)
        /* There is something wrong. But what can I do?  */
        return;
    }

  grub_outb (c, serial_settings.port + UART_TX);
}

/* Convert speed to divisor.  */
static unsigned short
serial_get_divisor (unsigned int speed)
{
  unsigned int i;

  /* The structure for speed vs. divisor.  */
  struct divisor
  {
    unsigned int speed;
    unsigned short div;
  };

  /* The table which lists common configurations.  */
  /* 1843200 / (speed * 16)  */
  static struct divisor divisor_tab[] =
    {
      { 2400,   0x0030 },
      { 4800,   0x0018 },
      { 9600,   0x000C },
      { 19200,  0x0006 },
      { 38400,  0x0003 },
      { 57600,  0x0002 },
      { 115200, 0x0001 }
    };

  /* Set the baud rate.  */
  for (i = 0; i < sizeof (divisor_tab) / sizeof (divisor_tab[0]); i++)
    if (divisor_tab[i].speed == speed)
  /* UART in Yeeloong runs twice the usual rate.  */
#ifdef GRUB_MACHINE_MIPS_YEELOONG
      return 2 * divisor_tab[i].div;
#else
      return divisor_tab[i].div;
#endif
  return 0;
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
static grub_err_t
serial_hw_init (void)
{
  unsigned char status = 0;

  /* Turn off the interrupt.  */
  grub_outb (0, serial_settings.port + UART_IER);

  /* Set DLAB.  */
  grub_outb (UART_DLAB, serial_settings.port + UART_LCR);

  /* Set the baud rate.  */
  grub_outb (serial_settings.divisor & 0xFF, serial_settings.port + UART_DLL);
  grub_outb (serial_settings.divisor >> 8, serial_settings.port + UART_DLH);

  /* Set the line status.  */
  status |= (serial_settings.parity
	     | serial_settings.word_len
	     | serial_settings.stop_bits);
  grub_outb (status, serial_settings.port + UART_LCR);

  /* In Yeeloong serial port has only 3 wires.  */
#ifndef GRUB_MACHINE_MIPS_YEELOONG
  /* Enable the FIFO.  */
  grub_outb (UART_ENABLE_FIFO, serial_settings.port + UART_FCR);

  /* Turn on DTR, RTS, and OUT2.  */
  grub_outb (UART_ENABLE_MODEM, serial_settings.port + UART_MCR);
#endif

  /* Drain the input buffer.  */
  while (serial_hw_fetch () != -1);

  /*  FIXME: should check if the serial terminal was found.  */

  return GRUB_ERR_NONE;
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
    .put = serial_hw_put
  };

static struct grub_term_input grub_serial_term_input =
{
  .name = "serial",
  .init = grub_terminfo_input_init,
  .checkkey = grub_terminfo_checkkey,
  .getkey = grub_terminfo_getkey,
  .data = &grub_serial_terminfo_input
};

static struct grub_term_output grub_serial_term_output =
{
  .name = "serial",
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



static grub_err_t
grub_cmd_serial (grub_extcmd_t cmd,
                 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  struct grub_arg_list *state = cmd->state;
  struct serial_port backup_settings = serial_settings;
  grub_err_t hwiniterr;

  if (state[0].set)
    {
      unsigned int unit;

      unit = grub_strtoul (state[0].arg, 0, 0);
      serial_settings.port = serial_hw_get_port (unit);
      if (!serial_settings.port)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad unit number");
    }

  if (state[1].set)
    serial_settings.port = (grub_port_t) grub_strtoul (state[1].arg, 0, 0);

  if (state[2].set)
    {
      unsigned long speed;

      speed = grub_strtoul (state[2].arg, 0, 0);
      serial_settings.divisor = serial_get_divisor ((unsigned int) speed);
      if (serial_settings.divisor == 0)
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad speed");
	}
    }

  if (state[3].set)
    {
      if (! grub_strcmp (state[3].arg, "5"))
	serial_settings.word_len = UART_5BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "6"))
	serial_settings.word_len = UART_6BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "7"))
	serial_settings.word_len = UART_7BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "8"))
	serial_settings.word_len = UART_8BITS_WORD;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad word length");
	}
    }

  if (state[4].set)
    {
      if (! grub_strcmp (state[4].arg, "no"))
	serial_settings.parity = UART_NO_PARITY;
      else if (! grub_strcmp (state[4].arg, "odd"))
	serial_settings.parity = UART_ODD_PARITY;
      else if (! grub_strcmp (state[4].arg, "even"))
	serial_settings.parity = UART_EVEN_PARITY;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad parity");
	}
    }

  if (state[5].set)
    {
      if (! grub_strcmp (state[5].arg, "1"))
	serial_settings.stop_bits = UART_1_STOP_BIT;
      else if (! grub_strcmp (state[5].arg, "2"))
	serial_settings.stop_bits = UART_2_STOP_BITS;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad number of stop bits");
	}
    }

  /* Initialize with new settings.  */
  hwiniterr = serial_hw_init ();

  if (hwiniterr == GRUB_ERR_NONE)
    {
      /* Register terminal if not yet registered.  */
      if (registered == 0)
	{
	  grub_term_register_input ("serial", &grub_serial_term_input);
	  grub_term_register_output ("serial", &grub_serial_term_output);
	  grub_terminfo_output_register (&grub_serial_term_output, "vt100");
	  registered = 1;
	}
    }
  else
    {
      /* Initialization with new settings failed.  */
      if (registered == 1)
	{
	  /* If the terminal is registered, attempt to restore previous
	     settings.  */
	  serial_settings = backup_settings;
	  if (serial_hw_init () != GRUB_ERR_NONE)
	    {
	      /* If unable to restore settings, unregister terminal.  */
	      grub_term_unregister_input (&grub_serial_term_input);
	      grub_term_unregister_output (&grub_serial_term_output);
	      grub_terminfo_output_unregister (&grub_serial_term_output);
	      registered = 0;
	    }
	}
    }

  return hwiniterr;
}

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
