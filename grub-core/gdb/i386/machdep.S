/* machdep.S - machine dependent assembly routines for the GDB stub */
/*
 *  Copyright (C) 2006  Lubomir Kundrak
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

#define ASM	1
#include <grub/cpu/gdb.h>

#define EC_PRESENT	1
#define EC_ABSENT	0

#define GRUB_GDB_STACKSIZE	40000

/*
 * The .data index for the address vector.
 */

#define	VECTOR		1

	.globl grub_gdb_trap
	.globl grub_gdb_regs

	.bss
	.globl grub_gdb_stack
	.space GRUB_GDB_STACKSIZE
grub_gdb_stack:

/*
 * Supplemental macros for register saving/restoration
 * on exception handler entry/leave.
 */

.macro save32 reg ndx
	movl \reg, grub_gdb_regs+(\ndx * 4)
.endm

.macro save16 reg ndx
	mov $0, %eax
	movw \reg, grub_gdb_regs+(\ndx * 4)
	movw %ax, grub_gdb_regs+(\ndx * 4 + 2)
	movl grub_gdb_regs+(EAX * 4), %eax
.endm

.macro load32 ndx reg
	movl grub_gdb_regs+(\ndx * 4), \reg
.endm

.macro load16 ndx reg
	movw grub_gdb_regs+(\ndx * 4), \reg
.endm

.macro save_context
	save32 %eax EAX
	save32 %ecx ECX
	save32 %edx EDX
	save32 %ebx EBX
	save32 %ebp EBP
	save32 %esi ESI
	save32 %edi EDI

	popl %ebx
	save32 %ebx EIP
	popl %ebx
	save32 %ebx CS
	popl %ebx
	save32 %ebx EFLAGS

	save32 %esp ESP

	save16 %ds DS
	save16 %es ES
	save16 %fs FS
	save16 %gs GS
	save16 %ss SS
.endm

.macro load_context
	load16 SS %ss
	load32 ESP %esp

	load32 EBP %ebp
	load32 ESI %esi
	load32 EDI %edi

	load16 DS %ds
	load16 ES %es
	load16 FS %fs
	load16 GS %gs

	load32 EFLAGS %eax
	pushl %eax
	load32 CS %eax
	pushl %eax
	load32 EIP %eax
	pushl %eax

	load32 EBX %ebx
	load32 EDX %edx
	load32 ECX %ecx
	load32 EAX %eax
.endm

/*
 * This macro creates handlers for a given range of exception numbers
 * and adds their addresses to the grub_gdb_trapvec array.
 */

.macro ent ec beg end=0

	/*
	 * Wrapper body itself.
	 */

	.text
1:	
	.if \ec
		add $4,%esp
	.endif

	save_context
	mov	$grub_gdb_stack, %esp
	mov	$\beg, %eax	/* trap number */
	call	grub_gdb_trap
	load_context
	iret

	/*
	 * Address entry in trapvec array.
	 */

	.data VECTOR
	.long 1b

	/*
	 * Next... (recursion).
	 */

	.if \end-\beg > 0
		ent \ec "(\beg+1)" \end
	.endif
.endm

/*
 * Here does the actual construction of the address array and handlers
 * take place.
 */

	.data VECTOR
	.globl grub_gdb_trapvec
grub_gdb_trapvec:
	ent EC_ABSENT	0 7
	ent EC_PRESENT	8
	ent EC_ABSENT	9
	ent EC_PRESENT	10 14
	/*
	 * You may have to split this further or as(1)
	 * will complain about nesting being too deep.
	 */
	ent EC_ABSENT	15 GRUB_GDB_LAST_TRAP
