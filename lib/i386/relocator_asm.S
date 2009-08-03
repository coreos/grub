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

RELOCATOR_VARIABLE(start)
#ifdef BACKWARD
base:
#endif
	cli

#ifndef __x86_64__
	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE(dest)
	.long 0
	mov %eax, %edi

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE(src)
	.long 0
	mov %eax, %esi

	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE(size)
	.long 0
	mov %edi, %eax
#ifndef BACKWARD
	add %ecx, %eax
#endif
	add $0x3, %ecx
	shr $2, %ecx

#ifdef BACKWARD
	/* Backward movsl is implicitly off-by-four.  compensate that.  */
	subl	$4,	%esi
	subl	$4,	%edi

	/* Backward copy.  */
	std
	addl	%ecx, %esi
	addl	%ecx, %edi

	rep
	movsl

#else
	/* Forward copy.  */
	cld
	rep
	movsl
#endif
	/* %eax contains now our new base.  */
	mov %eax, %esi
	add $(cont0 - base), %eax
	jmp *%eax
cont0:
#else
	xorq %rax, %rax

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE(dest)
	.long 0
	mov %eax, %edi

	/* mov imm64, %rax */
	.byte	0xb8
RELOCATOR_VARIABLE(src)
	.long 0, 0
	mov %rax, %rsi

	xorl %rcx, %rcx
	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE(size)
	.long 0

	mov %rdi, %rax
#ifndef BACKWARD
	add %rcx, %rax
#endif
	add $0x3, %rcx
	shr $2, %rcx

#ifdef BACKWARD
	/* Backward movsl is implicitly off-by-four.  compensate that.  */
	subl	$4,	%rsi
	subl	$4,	%rdi

	/* Backward copy.  */
	std
	addl	%rcx, %rsi
	addl	%rcx, %rdi

	rep
	movsl

#else
	/* Forward copy.  */
	cld
	rep
	movsl
#endif

	/* %rax contains now our new 'base'.  */
	mov %rax, %rsi
#ifdef APPLE_CC
	add $(cont0 - base), %eax
#else
	add $(cont0 - base), %rax
#endif
	jmp *%rax
cont0:
#ifdef APPLE_CC
	lea (cont1 - base) (%esi, 1), %eax
	mov %eax, (jump_vector - base) (%esi, 1)

	lea (gdt - base) (%esi, 1), %eax
	mov %eax, (gdt_addr - base) (%esi, 1)

	/* Switch to compatibility mode. */

	lgdt (gdtdesc - base) (%esi, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp *(jump_vector - base) (%esi,1)
#else
	lea (cont1 - base) (%rsi, 1), %rax
	mov %eax, (jump_vector - base) (%rsi, 1)

	lea (gdt - base) (%rsi, 1), %rax
	mov %rax, (gdt_addr - base) (%rsi, 1)

	/* Switch to compatibility mode. */

	lgdt (gdtdesc - base) (%rsi, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp *(jump_vector - base) (%rsi, 1)
#endif

cont1:
	.code32

	/* Update other registers. */
	mov $0x18, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss

	/* Disable paging. */
	mov %cr0, %eax
	and $0x7fffffff, %eax
	mov %eax, %cr0

	/* Disable amd64. */
	mov $0xc0000080, %ecx
	rdmsr
	and $0xfffffeff, %eax
	wrmsr

	/* Turn off PAE. */
	movl %cr4, %eax
	and $0xffffffcf, %eax
	mov %eax, %cr4

	jmp cont2
cont2:
#endif
	.code32

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE (esp)
	.long 0
	
	movl %eax, %esp
	
	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE (eax)
	.long 0

	/* mov imm32, %ebx */
	.byte	0xbb
RELOCATOR_VARIABLE (ebx)
	.long 0

	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE (ecx)
	.long 0

	/* mov imm32, %edx */
	.byte	0xba
RELOCATOR_VARIABLE (edx)
	.long 0

	/* Cleared direction flag is of no problem with any current
	   payload and makes this implementation easier.  */
	cld

	.byte 0xea
RELOCATOR_VARIABLE (eip)
	.long 0
	.word 0x08

#ifdef __x86_64__
	/* GDT. Copied from loader/i386/linux.c. */
	.p2align 4
gdt:
	/* NULL.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Reserved.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Code segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00

	/* Data segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdtdesc:
	.word 31
gdt_addr:
	/* Filled by the code. */
	.quad 0

	.p2align 4
jump_vector:
	/* Jump location. Is filled by the code */
	.long 0
	.long 0x10
#endif

#ifndef BACKWARD
base:
#endif

	
RELOCATOR_VARIABLE(end)
