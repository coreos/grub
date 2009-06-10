/* -*-Asm-*- */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/boot.h>
#include <grub/machine/boot.h>
#include <grub/machine/kernel.h>
#include <multiboot.h>

        .file   "lnxboot.S"

#ifdef APPLE_CC
#error Building lnxboot.img with Apple's as results in an unusable image
#endif

#define CODE_ADDR	0x6000
#ifndef APPLE_CC
#define CODE_LENG	(code_end - start)
#endif
#define DATA_ADDR	((GRUB_BOOT_MACHINE_KERNEL_ADDR) + 0x200)

#define BLCK_LENG	0x4000

	.text

        .code16

        .globl  start, _start

data_start:
	xorl	%ebp, %ebp
	jmp	linux_next

	. = data_start + 0x1F1

setup_sects:
/* Apple's cc can't fill this value.  */
#ifdef APPLE_CC
	.byte	0
#else
	.byte	(CODE_LENG >> 9)
#endif
root_flags:
	.word	0
syssize:
	.word	0
swap_dev:
	.word	0
ram_size:
	.word	0
vid_mode:
	.word	0
root_dev:
	.word	0
boot_flag:
	.word	0xAA55

start:
_start:

	jmp linux_init

	.ascii	"HdrS"			/* Header signature.  */
	.word	0x0203			/* Header version number.  */

realmode_swtch:
	.word	0, 0			/* default_switch, SETUPSEG.  */
start_sys_seg:
	.word	0x1000			/* Obsolete.  */
version_ptr:
	.word	0			/* Version string ptr.  */
type_of_loader:
	.byte	0			/* Filled in by boot loader.  */
loadflags:
	.byte	1			/* Please load high.  */
setup_move_size:
	.word	0			/* Unused.  */
code32_start:
	.long	0x100000		/* 32-bit start address.  */
ramdisk_image:
	.long	0			/* Loaded ramdisk image address.  */
ramdisk_size:
	.long	0			/* Size of loaded ramdisk.  */
bootsect_kludge:
	.word	0, 0
heap_end_ptr:
	.word	0
pad1:
	.word	0
cmd_line_ptr:
	.long	0			/* Command line.  */
ramdisk_max:
	.long	0xffffffff		/* Highest allowed ramdisk address.  */

gdt:
	.long	0, 0, 0, 0		/* Must be zero.  */
	.word	0xffff			/* 64 K segment size.  */
gdt_src1:
	.byte	0, 0 ,0			/* Low 24 bits of source address.  */
	.byte	0x93			/* Access rights.  */
	.byte	0			/* Extended access rights.  */
gdt_src2:
	.byte	0			/* High 8 bits of source address.  */
	.word	0xffff			/* 64 K segment size.  */
gdt_dst1:
	.byte	0, 0, 0			/* Low 24 bits of target address.  */
	.byte	0x93			/* Access rights.  */
	.byte	0			/* Extended access rights.  */
gdt_dst2:
	.byte	0			/* High 8 bits of source address.  */
	.long	0, 0, 0, 0		/* More space for the BIOS.  */

reg_edx:
	.byte	0x80, 0, 0xFF, 0xFF

data_leng:
	.long	0

linux_init:

#ifdef APPLE_CC
	reg_edx_rel = reg_edx - start
	code32_start_rel = code32_start - start
	movw	%cs:(reg_edx_rel), %dx
	movl	%cs:(code32_start_rel), %ebp
#else
	movw	%cs:(reg_edx - start), %dx
	movl	%cs:(code32_start - start), %ebp
#endif

linux_next:

	call	normalize

normalize:
	popw	%bx
#ifdef APPLE_CC
	normalize_rel = normalize - start
	subw	$(normalize_rel), %bx
#else
	subw	$(normalize - start), %bx
#endif
	shrw	$4, %bx
	movw	%cs, %ax
	addw	%bx, %ax
	pushw	%ax
#ifdef APPLE_CC
	real_code_rel = real_code - start
	pushw	$(real_code_rel)
#else
	pushw	$(real_code - start)
#endif
	lret				/* Jump to real_code.  */

real_code:
	subw	$0x20, %ax
	movw	%ax, %ds
	movw	(setup_sects - data_start), %cx
	shlw	$7, %cx

	/* Setup stack.  */

	xorw	%si, %si
	movw	%si, %ss
	movw	$(CODE_ADDR), %sp

	/* Move itself to 0:CODE_ADDR.  */

	cld
	movw	%cs, %ax
	movw	%ax, %ds
	movw	$(CODE_ADDR >> 4), %ax
	movw	%ax, %es
	movw	%si, %di

	rep
	movsl

#ifdef APPLE_CC
	real_code_2_rel = real_code_2 - start
	ljmp	$(CODE_ADDR >> 4), $(real_code_2_rel)
#else
	ljmp	$(CODE_ADDR >> 4), $(real_code_2  - start)
#endif

real_code_2:

	xchgl	%ebp, %esi
	orl	%esi, %esi
	jnz	1f
	movw	%ds, %si
	shll	$4, %esi
	addl	%ebp, %esi
1:

	pushw	%es
	popw	%ds

	movl	$0x200, %ecx
	addl	%ecx, %esi
	movl	$DATA_ADDR, %edi

	call	move_memory

	/* Check for multiboot signature.  */
	cmpl	$MULTIBOOT_MAGIC, %ss:(DATA_ADDR + GRUB_KERNEL_MACHINE_DATA_END)
	jz	1f

#ifdef APPLE_CC
	ramdisk_image_rel = ramdisk_image - start
	ramdisk_size_rel = ramdisk_size - start
	movl	(ramdisk_image_rel), %esi
	movl	(ramdisk_size_rel), %ecx
#else
	movl	(ramdisk_image - start), %esi
	movl	(ramdisk_size - start), %ecx
#endif
	movl	$(DATA_ADDR - 0x200), %edi
	jmp	2f

1:

	movl	%ss:(DATA_ADDR + GRUB_KERNEL_MACHINE_COMPRESSED_SIZE), %ecx
	addl	$(GRUB_KERNEL_MACHINE_RAW_SIZE - 0x200), %ecx

2:
	call	move_memory

	movsbl	%dh, %eax
	movl	%eax, %ss:(DATA_ADDR + GRUB_KERNEL_MACHINE_INSTALL_DOS_PART)

#ifdef APPLE_CC
	reg_edx_rel = reg_edx - start
	movsbl	(reg_edx_rel + 2), %eax
#else
	movsbl	(reg_edx + 2 - start), %eax
#endif
	movl	%eax, %ss:(DATA_ADDR + GRUB_KERNEL_MACHINE_INSTALL_BSD_PART)

	movb	$0xFF, %dh

	ljmp	$(DATA_ADDR >> 4), $0

/*
 * Parameters:
 *   esi: source address
 *   edi: target address
 *   ecx: number of bytes
 */

move_memory:
	incl	%ecx
	andb	$0xFE, %cl
	pushw	%dx
1:
	pushl	%esi
	pushl	%edi
	pushl	%ecx
	cmpl	$BLCK_LENG, %ecx
	jbe	2f
	movl	$BLCK_LENG, %ecx
2:
	pushl	%ecx

	movl	%esi, %eax
#ifdef APPLE_CC
	gdt_src1_rel = gdt_src1 - start
	gdt_src2_rel = gdt_src2 - start
	gdt_dst1_rel = gdt_dst1 - start
	gdt_dst2_rel = gdt_dst2 - start
	gdt_rel = gdt - start

	movw	%si, (gdt_src1_rel)
	shrl	$16, %eax
	movb	%al, (gdt_src1_rel + 2)
	movb	%ah, (gdt_src2_rel)

	movl	%edi, %eax
	movw	%di, (gdt_dst1_rel)
	shrl	$16, %eax
	movb	%al, (gdt_dst1_rel + 2)
	movb	%ah, (gdt_dst2_rel)

	movw	$(gdt_rel), %si
	movb	$0x87, %ah
	shrw	$1, %cx
#else
	movw	%si, (gdt_src1 - start)
	shrl	$16, %eax
	movb	%al, (gdt_src1 + 2 - start)
	movb	%ah, (gdt_src2 - start)

	movl	%edi, %eax
	movw	%di, (gdt_dst1 - start)
	shrl	$16, %eax
	movb	%al, (gdt_dst1 + 2 - start)
	movb	%ah, (gdt_dst2 - start)

	movw	$(gdt - start), %si
	movb	$0x87, %ah
	shrw	$1, %cx
#endif

	int	$0x15

	popl	%eax
	popl	%ecx
	popl	%edi
	popl	%esi

	jnc	2f
#ifdef APPLE_CC
	err_int15_msg_rel = err_int15_msg - start
	movw	$(err_int15_msg_rel), %si
#else
	movw	$(err_int15_msg - start), %si
#endif
	jmp	fail

2:

	addl	%eax, %esi
	addl	%eax, %edi
	subl	%eax, %ecx
	jnz	1b


	popw	%dx
	ret

/*
 * Parameters:
 *   si: message
 */

fail:
	movb	$0x0e, %ah
	xorw	%bx, %bx
1:
	lodsb	(%si), %al
	int	$0x10
	cmpb	$0, %al
	jne	1b
1:	jmp	1b

err_int15_msg:
	.ascii	"move memory fails\0"

	/* Unsupported feature in Apple's cc.  */
#ifndef APPLE_CC
	. = (.  & (~0x1FF)) + 0x1FF
#endif

	.byte	0

code_end:
