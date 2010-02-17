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
#include <grub/mips/yeeloong/pci.h>
#include <grub/mips/yeeloong/boot.h>
#include <grub/pci.h>
#include <grub/serial.h>
#include <grub/cs5536.h>

	.set noreorder
	.set noat
	.set nomacro

	.global start,_start,__start
start:
_start:
__start:	
	bal serial_hw_init
	 nop
	/* Find CS5536 controller.  */
	/* $t4 chooses device in priority encoding.  */
	/* Resulting value is kept in GRUB_MACHINE_PCI_CONF_CTRL_REG.
	   This way we don't need to sacrifice a register for it.  */
	/* We have only one bus (0). Function is 0.  */
	lui $t0, GRUB_MACHINE_PCI_CONF_CTRL_REG_HI
	lui $t1, %hi(GRUB_MACHINE_PCI_CONFSPACE)
	lui $t3, %hi(GRUB_CS5536_PCIID)
	addiu $t3, $t3, %lo(GRUB_CS5536_PCIID)
	ori $t4, $zero, 1
	lui $a0, %hi(no_cs5536)
1:
	andi $t4, $t4, 0xffff
	beql  $t4, $zero, fatal
	 addiu $a0, $a0, %lo(no_cs5536)
	sw   $t4, GRUB_MACHINE_PCI_CONF_CTRL_REG_LO($t0)
	lw   $t2, (%lo(GRUB_MACHINE_PCI_CONFSPACE) + GRUB_PCI_REG_PCI_ID) ($t1)
	bnel  $t2, $t3, 1b
	 sll $t4, $t4, 1

	bal message
	 addiu $a0, $a0, %lo(cs5536_found)
	bal printhex
	 move $a0, $t4

	/* Initialise SMBus controller.  */
	/* Set GPIO LBAR.  */
	lui $a0, %hi(GRUB_CS5536_MSR_GPIO_BAR)
	addiu $a0, $a0, %lo(GRUB_CS5536_MSR_GPIO_BAR)
	ori $a1, $zero, GRUB_CS5536_LBAR_GPIO
	/* Set mask to 0xf and enabled bit to 1.  */
	bal wrmsr
	 ori $a2, $zero, 0xf001

	/* Set SMBUS LBAR.  */
	lui $a0, %hi(GRUB_CS5536_MSR_SMB_BAR)
	addiu $a0, $a0, %lo(GRUB_CS5536_MSR_SMB_BAR)
	ori $a1, $zero, GRUB_CS5536_LBAR_SMBUS
	/* Set mask to 0xf and enabled bit to 1.  */
	bal wrmsr
	 ori $a2, $zero, 0xf001

	lui $a0, %hi(smbus_enabled)
	bal message
	  addiu $a0, $a0, %lo(smbus_enabled)

	/* Enable SMBus controller pins.  */
	lui $t0, %hi(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_GPIO)
	ori $t1, $zero, GRUB_GPIO_SMBUS_PINS
	sw  $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_GPIO + GRUB_GPIO_REG_OUT_EN) ($t0)
	sw  $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_GPIO + GRUB_GPIO_REG_OUT_AUX1) ($t0)
	sw  $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_GPIO + GRUB_GPIO_REG_IN_EN) ($t0)
	sw  $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_GPIO + GRUB_GPIO_REG_IN_AUX1) ($t0)

	lui $t0, %hi(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS)

	/* Disable SMB.  */
	sb $zero, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL2) ($t0)

	/* Disable interrupts.  */
	sb $zero, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1) ($t0)

	/* Set as master.  */
	sb $zero, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_ADDR) ($t0)

	/* Launch SMBus controller at slowest speed possible.  */
	ori $t1, $zero, 0xff
	sb $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL3) ($t0)
	sb $t1, %lo(GRUB_MACHINE_PCI_IO_BASE + GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL2) ($t0)

	ori $a0, $zero, 0x50
	bal read_spd
	 move $a1, $zero
	bal printhex
	 move $a0, $v0

	ori $a0, $zero, 0x50
	bal read_spd
	 ori $a1, $zero, 2
	bal printhex
	 move $a0, $v0
	
	lui $a0, %hi(not_implemented)
	b fatal
	 addiu $a0, $a0, %lo(not_implemented)

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
	lb $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_LSR)($t0)
	andi $t1, $t1, UART_EMPTY_TRANSMITTER
	beq $t1, $zero, 1b
	 nop
	lb  $t1, 0($a0)
	sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_TX)($t0)
	bne $t1, $zero, 1b
	 addiu $a0, $a0, 1
	jr  $ra
	 nop

	/* Print 32-bit hexadecimal on serial.
           In:	$a0. Out: None. Clobbered: $a0, $t0, $t1, $t2
	*/
printhex:
	lui $t0, GRUB_MACHINE_SERIAL_PORT_HI
	ori $t2, $zero, 8
1:
	lb $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_LSR)($t0)
	andi $t1, $t1, UART_EMPTY_TRANSMITTER
	beq $t1, $zero, 1b
	 nop
	srl  $t1, $a0, 28
	addiu $t1, $t1, -10
	blt  $t1, $zero, 2f
	 sll  $a0, $a0, 4
	addiu $t1, $t1, 'A'-10-'0' 
2:	addiu $t1, $t1, '0'+10
	sb  $t1, (GRUB_MACHINE_SERIAL_PORT_LO + UART_TX)($t0)
	addiu $t2, $t2, -1
	bne $t2, $zero, 1b
	 nop
	jr  $ra
	 nop

fatal:
	bal message
	 nop
self:
	b self
	 nop

	. = start + GRUB_MACHINE_FLASH_TLB_REFILL - GRUB_MACHINE_FLASH_START
tlb_refill:	
	lui $a0, %hi(unhandled_tlb_refill)
	b fatal
	 addiu $a0, $a0, %lo(unhandled_tlb_refill)

	. = start + GRUB_MACHINE_FLASH_CACHE_ERROR - GRUB_MACHINE_FLASH_START
cache_error:
	lui $a0, %hi(unhandled_cache_error)
	b fatal
	 addiu $a0, $a0, %lo(unhandled_cache_error)

	. = start + GRUB_MACHINE_FLASH_OTHER_EXCEPTION - GRUB_MACHINE_FLASH_START
other_exception:
	lui $a0, %hi(unhandled_exception)
	b fatal
	 addiu $a0, $a0, %lo(unhandled_exception)

	/* Write CS5536 MSR.
           In:   $a0 address, $a1 lower word, $a2 upper word.
           Out:	 None
           Clobbered:	 $t0
	*/
wrmsr:
	lui $t0, %hi(GRUB_MACHINE_PCI_CONFSPACE)
	sw  $a0, (%lo(GRUB_MACHINE_PCI_CONFSPACE) + GRUB_CS5536_MSR_MAILBOX_ADDR) ($t0)
	sw  $a1, (%lo(GRUB_MACHINE_PCI_CONFSPACE) + GRUB_CS5536_MSR_MAILBOX_DATA0) ($t0)
	jr $ra
	 sw  $a2, (%lo(GRUB_MACHINE_PCI_CONFSPACE) + GRUB_CS5536_MSR_MAILBOX_DATA1) ($t0)

	/* Wait for SMBus data or empty transmitter.  */
	/* In: $a0 = exception handler. Out: none. Clobbered: $t0, $t1  */
smbus_wait:
1:	
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_STATUS + GRUB_MACHINE_PCI_IO_BASE)
	lb $t0, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_STATUS + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	andi $t1, $t0, GRUB_CS5536_SMB_REG_STATUS_SDAST
	bne $t1, $zero, return
	 nop
	andi $t1, $t0, (GRUB_CS5536_SMB_REG_STATUS_BER | GRUB_CS5536_SMB_REG_STATUS_NACK)
	beq $t1, $zero, 1b
	 nop
	jr $a0
	 nop
return:
	jr $ra
	 nop
	
	/* Read SPD byte. In: $a0 device, $a1 byte. Out: $v0 read byte (0x100 on failure).
           Clobbered: $t0, $t1, $t2, $t3, $a0. */
read_spd:
	move $t2, $a0
	move $t3, $ra
	lui $a0, %hi(read_spd_fail)
	addiu $a0, $a0, %hi(read_spd_fail)

	/* Send START.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE)
	lb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	ori $t1, $t1, GRUB_CS5536_SMB_REG_CTRL1_START
	bal smbus_wait
	 sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	
	/* Send device address.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE)
	sll $t1, $t2, 1
	bal smbus_wait
	 sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	/* Send ACK.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE)
	lb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	ori $t1, $t1, GRUB_CS5536_SMB_REG_CTRL1_ACK
	sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	/* Send byte address.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE)
	bal smbus_wait
	 sb $a1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	/* Send START.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE)
	lb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	ori $t1, $t1, GRUB_CS5536_SMB_REG_CTRL1_START
	bal smbus_wait
	 sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	/* Send device address.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE)
	sll $t1, $t2, 1
	ori $t1, $t1, 1
	bal smbus_wait
	 sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	/* Send STOP.  */
	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE)
	lb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	ori $t1, $t1, GRUB_CS5536_SMB_REG_CTRL1_STOP
	bal smbus_wait
	 sb $t1, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_CTRL1 + GRUB_MACHINE_PCI_IO_BASE) ($t0)

	lui $t0, %hi(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE)
	lb $v0, %lo(GRUB_CS5536_LBAR_SMBUS + GRUB_CS5536_SMB_REG_DATA + GRUB_MACHINE_PCI_IO_BASE) ($t0)
	jr $t3
	 andi $v0, $v0, 0xff
read_spd_fail:
	jr $t3
	 ori $v0, $v0, 0x100

notification_string:	.asciz "GRUB "
no_cs5536:	.asciz "No CS5536 found.\n\r"
cs5536_found:	.asciz "CS5536 at "
sm_failed: .asciz "SM transaction failed.\n\r"
not_implemented: .asciz "Nothing more is implemented. Bye.\n\r"
unhandled_tlb_refill:	.asciz "Unhandled TLB refill.\n\r"
unhandled_cache_error:	.asciz "Unhandled cache error.\n\r"
unhandled_exception:	.asciz "Unhandled exception.\n\r"
smbus_enabled:	.asciz "SMBus controller enabled.\n\r"
