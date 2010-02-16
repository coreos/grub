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

#include <grub/mips/yeeloong/serial.h>
#include <grub/serial.h>

	.set noreorder
	.set noat
	.set nomacro

	bal serial_hw_init
	 nop
self:	
	b self
	 nop

	/* Same as similarly named C function but in asm since
	   we need it early.  */
	/* In: none. Out: none. Clobbered: $t0, $t1, $a0.  */
serial_hw_init:
	lui $t0, GRUB_MACHINE_SERIAL_PORT_HI

	/* Turn off the interrupt.  */
	sb $zero, (GRUB_MACHINE_SERIAL_PORT_LO + UART_IER)($t0)

	/* Set DLAB.  */
	ori $t1, $zero, UART_DLAB
	sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_LCR)($t0)

	/* Set the baud rate 115200.  */
	ori $t1, $zero, 2
	sb $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_DLL)($t0) 
	sb $zero, (GRUB_MACHINE_SERIAL_PORT_LO + UART_DLH)($t0) 

	/* Set the line status.  */
	ori $t1, $zero, (UART_NO_PARITY | UART_8BITS_WORD | UART_1_STOP_BIT)
        sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_LCR)($t0)

	/* Enable the FIFO.  */
	ori $t1, $zero, UART_ENABLE_FIFO_TRIGGER1
	sb $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_FCR)($t0)

	/* Turn on DTR and RTS.  */
	ori $t1, $zero, UART_ENABLE_DTRRTS
	sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_MCR)($t0)

	lui $t0, 0xbff0
	addiu $t0, $t0, 0x03f8	

	/* Let message return to original caller.  */
	lui  $a0, %hi(notification_string)
	addiu $a0, $a0, %lo(notification_string)

	/* Print message on serial console.  */
	/* In: $a0 = asciiz message. Out: none. Clobbered: $t0, $t1, $a0.  */
message:
	lui $t0, GRUB_MACHINE_SERIAL_PORT_HI
1:	
	lb  $t1, 0($a0)
	addiu $a0, $a0, 1
	bne $t1, $zero, 1b
	 sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_TX)($t0)
	jr  $ra
	 nop

notification_string:	.asciz "GRUB "
