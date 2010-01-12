/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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

#ifdef __x86_64__
#define RAX %rax
#define RSI %rsi
#else
#define RAX %eax
#define RSI %esi
#endif

/* The code segment of the protected mode.  */
#define CODE_SEGMENT	0x10

/* The data segment of the protected mode.  */
#define DATA_SEGMENT	0x18

	.p2align	4	/* force 16-byte alignment */

VARIABLE(grub_relocator32_start)
LOCAL(base):
	/* %rax contains now our new 'base'.  */
	mov	RAX, RSI
	add	$(LOCAL(cont0) - LOCAL(base)), RAX
	jmp	*RAX
LOCAL(cont0):
	lea	(LOCAL(cont1) - LOCAL(base)) (RSI, 1), RAX
	movl	%eax, (LOCAL(jump_vector) - LOCAL(base)) (RSI, 1)

	lea	(LOCAL(gdt) - LOCAL(base)) (RSI, 1), RAX
	mov	RAX, (LOCAL(gdt_addr) - LOCAL(base)) (RSI, 1)

	/* Switch to compatibility mode. */

	lgdt	(LOCAL(gdtdesc) - LOCAL(base)) (RSI, 1)

	/* Update %cs.  */
	ljmp	*(LOCAL(jump_vector) - LOCAL(base)) (RSI, 1)

LOCAL(cont1):
	.code32

	/* Update other registers. */
	movl	$DATA_SEGMENT, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	/* Disable paging. */
	movl	%cr0, %eax
	andl	$(~GRUB_MEMORY_CPU_CR0_PAGING_ON), %eax
	movl	%eax, %cr0

	/* Disable amd64. */
	movl	$GRUB_MEMORY_CPU_AMD64_MSR, %ecx
	rdmsr
	andl	$(~GRUB_MEMORY_CPU_AMD64_MSR_ON), %eax
	wrmsr

	/* Turn off PAE. */
	movl	%cr4, %eax
	andl	$(~GRUB_MEMORY_CPU_CR4_PAE_ON), %eax
	movl	%eax, %cr4

	jmp	LOCAL(cont2)
LOCAL(cont2):
	.code32

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_relocator32_esp)
	.long	0

	movl	%eax, %esp

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_relocator32_esi)
	.long	0

	movl	%eax, %esi
	
	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_relocator32_eax)
	.long	0

	/* mov imm32, %ebx */
	.byte	0xbb
VARIABLE(grub_relocator32_ebx)
	.long	0

	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE(grub_relocator32_ecx)
	.long	0

	/* mov imm32, %edx */
	.byte	0xba
VARIABLE(grub_relocator32_edx)
	.long	0

	/* Cleared direction flag is of no problem with any current
	   payload and makes this implementation easier.  */
	cld

	.byte	0xea
VARIABLE(grub_relocator32_eip)
	.long	0
	.word	CODE_SEGMENT

	/* GDT. Copied from loader/i386/linux.c. */
	.p2align	4
LOCAL(gdt):
	/* NULL.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Reserved.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Code segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00

	/* Data segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

	.p2align	4
LOCAL(gdtdesc):
	.word	0x27
LOCAL(gdt_addr):
#ifdef __x86_64__
	/* Filled by the code. */
	.quad	0
#else
	/* Filled by the code. */
	.long	0
#endif

	.p2align	4
LOCAL(jump_vector):
	/* Jump location. Is filled by the code */
	.long	0
	.long	CODE_SEGMENT

VARIABLE(grub_relocator32_end)
