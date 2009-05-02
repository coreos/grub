/* Mmap management. */
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

#define DS(x) ((x) - segstart)

segstart:	
VARIABLE(grub_machine_mmaphook_start)
	.code16
VARIABLE(grub_machine_mmaphook_int12)
	push %ds
	push %cs
	pop %ds
	movw DS (EXT_C (grub_machine_mmaphook_kblow)), %ax
	pop %ds
	iret

VARIABLE(grub_machine_mmaphook_int15)
	pushf
	cmpw $0xe801, %ax
	jz e801
	cmpw $0xe820, %ax
	jz e820
	cmpb $0x88, %ah
	jz h88
	popf
	/* ljmp */
	.byte	0xea
VARIABLE (grub_machine_mmaphook_int15offset)
	.word	0
VARIABLE (grub_machine_mmaphook_int15segment)
	.word	0
	
e801:
	popf
	push %ds
	push %cs
	pop %ds
	movw DS (EXT_C (grub_machine_mmaphook_kbin16mb)), %ax
	movw DS (EXT_C (grub_machine_mmaphook_64kbin4gb)), %bx
	movw %ax, %cx
	movw %bx, %dx
	pop %ds
	clc
	iret

h88:
	popf
	push %ds
	push %cs
	pop %ds
	movw DS (EXT_C (grub_machine_mmaphook_kbin16mb)), %ax
	pop %ds
	clc
	iret

e820:
	popf
	push %ds
	push %cs
	pop %ds
	cmpw $20, %cx
	jb errexit
	cmpw DS (EXT_C (grub_machine_mmaphook_mmap_num)), %bx
	jae errexit
	cmp $0x534d4150, %edx
	jne errexit
	push %si
	push %di
	movw $20, %cx
	movw $(DS(mmaphook_mmap)), %si
	mov %bx, %ax
	imul $20, %ax
	add %ax, %si
	rep movsb
	pop %di
	pop %si
	movl $20, %ecx
	inc %bx
	cmpw DS(EXT_C(grub_machine_mmaphook_mmap_num)), %bx
	jb noclean
	xor %bx, %bx	
noclean:	
	mov $0x534d4150, %eax
	pop %ds
	clc
	iret
errexit:
	mov $0x534d4150, %eax
	pop %ds
	stc
	xor %bx, %bx
	iret	
	
VARIABLE(grub_machine_mmaphook_mmap_num)
	.word 0
VARIABLE(grub_machine_mmaphook_kblow)
	.word 0
VARIABLE (grub_machine_mmaphook_kbin16mb)
	.word 0
VARIABLE (grub_machine_mmaphook_64kbin4gb)
	.word 0
mmaphook_mmap:
	/* Memory map is placed just after the interrupt handlers. */
VARIABLE(grub_machine_mmaphook_end)
