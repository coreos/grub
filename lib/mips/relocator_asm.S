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
	lb $4,0($1)
	sb $4,0($2)
	addiu $3, $3, 0xffff
	subu $4,$3,$0
	bne $4, $0, copycont1
VARIABLE (grub_relocator32_forward_end)

VARIABLE (grub_relocator32_backward_start)
	addu $2, $2, $3
	addu $1, $1, $3
	/* Backward movsl is implicitly off-by-one.  compensate that.  */
	addiu $2, $2, 0xffff
	addiu $1, $1, 0xffff
copycont2:	
	lb $4,0($1)
	sb $4,0($2)
	addiu $3, 0xffff
	subu $4,$3,$0
	bne $4, $0, copycont2
VARIABLE (grub_relocator32_backward_end)
