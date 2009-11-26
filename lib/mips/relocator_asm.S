/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/symbol.h>
	
	.p2align	4	/* force 16-byte alignment */

VARIABLE (grub_relocator32_forward_start)
	move $12, $9
	move $13, $10

copycont1:	
	lb $11,0($8)
	sb $11,0($9)
	addiu $8, $8, 0x1
	addiu $9, $9, 0x1
	addiu $10, $10, 0xffff
	bne $10, $0, copycont1

	move $9, $12
	move $10, $13
cachecont1a:	
	cache 1,0($12)
	addiu $12, $12, 0x1
	addiu $13, $13, 0xffff
	bne $13, $0, cachecont1a

	sync

	move $12, $9
	move $13, $10
cachecont1b:	
	cache 0,0($12)
	addiu $12, $12, 0x1
	addiu $13, $13, 0xffff
	bne $13, $0, cachecont1b

	sync

VARIABLE (grub_relocator32_forward_end)

VARIABLE (grub_relocator32_backward_start)
	move $12, $9
	move $13, $10

	addu $9, $9, $10
	addu $8, $8, $10
	/* Backward movsl is implicitly off-by-one.  compensate that.  */
	addiu $9, $9, 0xffff
	addiu $8, $8, 0xffff
copycont2:	
	lb $11,0($8)
	sb $11,0($9)
	addiu $8, $8, 0xffff
	addiu $9, $9, 0xffff
	addiu $10, 0xffff
	bne $10, $0, copycont2

	move $9, $12
	move $10, $13
cachecont2a:	
	cache 1,0($12)
	addiu $12, $12, 0x1
	addiu $13, $13, 0xffff
	bne $13, $0, cachecont2a

	sync

	move $12, $9
	move $13, $10
cachecont2b:	
	cache 0,0($12)
	addiu $12, $12, 0x1
	addiu $13, $13, 0xffff
	bne $13, $0, cachecont2b

	sync
VARIABLE (grub_relocator32_backward_end)
