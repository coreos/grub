/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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
#include <grub/dl.h>

	.file	"setjmp.S"
	.syntax	unified
#if !defined (__thumb2__)
	.arm
#define ARM(x...)	x
#define THUMB(x...)
#else
	.thumb
#define THUMB(x...)	x
#define ARM(x...)
#endif

	.text

/*
 * int grub_setjmp (grub_jmp_buf env)
 */
FUNCTION(grub_setjmp)
 THUMB(	mov	ip, sp			)
 THUMB(	stm	r0, { r4-r11, ip, lr }	)
 ARM(	stm	r0, { r4-r11, sp, lr }	)
	mov	r0, #0
	bx	lr

/*
 * int grub_longjmp (grub_jmp_buf env, int val)
 */
FUNCTION(grub_longjmp)
 THUMB(	ldm	r0, { r4-r11, ip, lr }	)
 THUMB(	mov	sp, ip			)
 ARM(	ldm	r0, { r4-r11, sp, lr }	)
	movs	r0, r1
	moveq	r0, #1
	bx	lr
