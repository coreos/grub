/* crt0.S - Startup code for the PowerPC.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.section ".note"
	.align	2
.note_section_header:
	.long	8
	.long	24
	.long	0x1275
	.string	"PowerPC"
.note_descriptor:
	.long   0x0             /* real-mode */
	.long   0xffffffff      /* real-base */
	.long   0xffffffff      /* real-size */
	.long   0xffffffff      /* virt-base */
	.long   0xffffffff      /* virt-size */
	.long   0x00030000      /* load-base */
	
	.text
	.align	2
	.globl	_start
_start:	
	lis	1, init_stack@ha
	la	1, init_stack@l(1)
	addi	1, 1, -32

	li      2, 0
	li      13, 0
	

	mr	3, 5	
	bl	cmain
1:	b	1b

	.section ".bss"
	.lcomm	_ppc_init_stack, 4096*2, 16
init_stack:

