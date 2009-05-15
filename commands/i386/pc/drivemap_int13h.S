/* drivemap_int13h.S - interrupt handler for the BIOS drive remapper */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008, 2009  Free Software Foundation, Inc.
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

#define INT13H_OFFSET(x) ((x) - EXT_C(grub_drivemap_handler))

.code16

/* Copy starts here.  When deployed, this code must be segment-aligned.  */

/* The replacement int13 handler.   Preserve all registers.  */
FUNCTION(grub_drivemap_handler)
	/* Map the drive number (always in DL).  */
	push	%ax
	push	%bx
	movw	$INT13H_OFFSET(EXT_C(grub_drivemap_mapstart)), %bx

more_remaining:
	movw	%cs:(%bx), %ax
	cmpb	%ah, %al
	jz	not_found /* DRV=DST => map end - drive not remapped, keep DL.  */
	inc	%bx
	inc	%bx
	cmpb	%dl, %al
	jnz	more_remaining /* Not found, but more remaining, loop.  */
	movb	%ah, %dl /* Found - drive remapped, modify DL.  */

not_found:
	pop	%bx
	pop	%ax

	/* Upon arrival to this point the stack must be exactly like at entry.
	   This long jump will transfer the caller's stack to the old INT13
	   handler, thus making it return directly to the original caller.  */
	.byte	0xea

/* Far pointer to the old handler.  Stored as a CS:IP in the style of real-mode
   IVT entries (thus PI:SC in mem).  */
VARIABLE(grub_drivemap_oldhandler)
	.word 0x0, 0x0

/* This label MUST be at the end of the copied block, since the installer code
   reserves additional space for mappings at runtime and copies them over it.  */
.align 2
VARIABLE(grub_drivemap_mapstart)
