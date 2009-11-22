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

#ifdef BACKWARD
#define RELOCATOR_VARIABLE(x) VARIABLE(grub_relocator32_backward_ ## x)
#else
#define RELOCATOR_VARIABLE(x) VARIABLE(grub_relocator32_forward_ ## x)
#endif
	
	.p2align	4	/* force 16-byte alignment */

VARIABLE (grub_relocator32_forward_start)
copycont1:	
	lb $11,0($8)
	sb $11,0($9)
	cache 1, 0($9)
	cache 0, 0($9)
	addiu $8, $8, 0x1
	addiu $9, $9, 0x1
	addiu $10, $10, 0xffff
	subu $11,$10,$0
	bne $11, $0, copycont1
VARIABLE (grub_relocator32_forward_end)

VARIABLE (grub_relocator32_backward_start)
	addu $9, $9, $10
	addu $8, $8, $10
	/* Backward movsl is implicitly off-by-one.  compensate that.  */
	addiu $9, $9, 0xffff
	addiu $8, $8, 0xffff
copycont2:	
	lb $11,0($8)
	sb $11,0($9)
	cache 1, 0($9)
	cache 0, 0($9)
	addiu $8, $8, 0xffff
	addiu $9, $9, 0xffff
	addiu $10, 0xffff
	subu $11,$10,$0
	bne $11, $0, copycont2
VARIABLE (grub_relocator32_backward_end)
