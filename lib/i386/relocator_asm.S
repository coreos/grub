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
#include <grub/i386/memory.h>

#ifdef BACKWARD
#define RELOCATOR_VARIABLE(x) VARIABLE(grub_relocator32_backward_ ## x)
#else
#define RELOCATOR_VARIABLE(x) VARIABLE(grub_relocator32_forward_ ## x)
#endif
#ifdef __x86_64__
#define RAX %rax
#define RCX %rcx
#define RDI %rdi
#define RSI %rdi
#else
#define RAX %eax
#define RCX %ecx
#define RDI %edi	
#define RSI %esi	
#endif

/* Apple's linker has a problem with 64-bit relocations.  */
#if defined (__apple__) || ! defined (__x86_64__)
#define RSIA %esi
#define RAXA %eax
#else
#define RSIA %rsi
#define RAXA %rax
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
	.long	0
	movl	%eax, %edi

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE(src)
	.long	0
	movl	%eax, %esi

	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE(size)
	.long	0
#else
	xorq	%rax, %rax

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE(dest)
	.long	0
	movq	%rax, %rdi

	/* mov imm64, %rax */
	.byte	0x48
	.byte	0xb8
RELOCATOR_VARIABLE(src)
	.long	0, 0
	movq	%rax, %rsi

	xorq	%rcx, %rcx
	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE(size)
	.long	0

#endif

	mov	RDI, RAX
	
#ifdef BACKWARD
	add	RCX, RSI
	add	RDX, RDI
#endif

#ifndef BACKWARD
	add	RCX, RAX
#endif
	addq	$0x3, RCX
	shrq	$2, RCX

	
#ifdef BACKWARD
	/* Backward movsl is implicitly off-by-four.  compensate that.  */
	subq	$4,	RSI
	subq	$4,	RDI

	/* Backward copy.  */
	std

	rep
	movsl

#else
	/* Forward copy.  */
	cld
	rep
	movsl
#endif

	/* %rax contains now our new 'base'.  */
	mov	RAX, RSI
	add	$(cont0 - base), RAXA
	jmp	*RAX
cont0:
	lea	(cont1 - base) (RSIA, 1), RAXA
	movl	%eax, (jump_vector - base) (RSIA, 1)

	lea	(gdt - base) (RSIA, 1), RAXA
	mov	RAXA, (gdt_addr - base) (RSIA, 1)

	/* Switch to compatibility mode. */

	lgdt	(gdtdesc - base) (RSIA, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp	*(jump_vector - base) (RSIA, 1)

cont1:
	.code32

	/* Update other registers. */
	movl	$GRUB_MEMORY_MACHINE_PROT_MODE_DSEG, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	/* Disable paging. */
	movl	%cr0, %eax
	andl	$0x7fffffff, %eax
	movl	%eax, %cr0

	/* Disable amd64. */
	movl	$0xc0000080, %ecx
	rdmsr
	andl	$0xfffffeff, %eax
	wrmsr

	/* Turn off PAE. */
	movl	%cr4, %eax
	andl	$0xffffffcf, %eax
	movl	%eax, %cr4

	jmp	cont2
cont2:
	.code32

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE (esp)
	.long	0

	movl	%eax, %esp

	/* mov imm32, %eax */
	.byte	0xb8
RELOCATOR_VARIABLE (eax)
	.long	0

	/* mov imm32, %ebx */
	.byte	0xbb
RELOCATOR_VARIABLE (ebx)
	.long	0

	/* mov imm32, %ecx */
	.byte	0xb9
RELOCATOR_VARIABLE (ecx)
	.long	0

	/* mov imm32, %edx */
	.byte	0xba
RELOCATOR_VARIABLE (edx)
	.long	0

	/* Cleared direction flag is of no problem with any current
	   payload and makes this implementation easier.  */
	cld

	.byte	0xea
RELOCATOR_VARIABLE (eip)
	.long	0
	.word	0x08

	/* GDT. The same as is used in 32-bit GRUB. */
	.p2align	4
gdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	/* -- code segment --
	 * base = 0x00000000, limit = 0xFFFFF (4 KiB Granularity), present
	 * type = 32bit code execute/read, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	/* -- data segment --
	 * base = 0x00000000, limit 0xFFFFF (4 KiB Granularity), present
	 * type = 32 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* -- 16 bit real mode CS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit code execute/read only/conforming, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x9E, 0, 0

	/* -- 16 bit real mode DS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0

	.p2align	4
gdtdesc:
	.word	0x27
gdt_addr:
#ifdef __x86_64__
	/* Filled by the code. */
	.quad	0
#else
	/* Filled by the code. */
	.long	0
#endif
	
	.p2align	4
jump_vector:
	/* Jump location. Is filled by the code */
	.long	0
	.long	GRUB_MEMORY_MACHINE_PROT_MODE_CSEG

#ifndef BACKWARD
base:
#endif

RELOCATOR_VARIABLE(end)
