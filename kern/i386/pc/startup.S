/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003 Free Software Foundation, Inc.
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


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:
	
   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

/*
 * Note: GRUB is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

#include <config.h>
#include <grub/symbol.h>
#include <grub/boot.h>
#include <grub/machine/boot.h>
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/machine/linux.h>
#include <grub/machine/kernel.h>
#include <grub/machine/multiboot.h>
		
#define ABS(x)	((x) - EXT_C(start) + GRUB_BOOT_MACHINE_KERNEL_ADDR + 0x200)
	
	.file	"startup.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

	.globl	start, _start
start:
_start:
	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200.
	 */
	ljmp $0, $ABS(codestart)

	/*
	 *  Compatibility version number
	 *
	 *  These MUST be at byte offset 6 and 7 of the executable
	 *  DO NOT MOVE !!!
	 */
	. = EXT_C(start) + 0x6
	.byte	GRUB_BOOT_VERSION_MAJOR, GRUB_BOOT_VERSION_MINOR

	/*
	 *  This is a special data area 8 bytes from the beginning.
	 */

	. = EXT_C(start) + 0x8

VARIABLE(grub_total_module_size)
	.long	0
VARIABLE(grub_kernel_image_size)
	.long	0
VARIABLE(grub_compressed_size)
	.long	0
VARIABLE(grub_install_dos_part)
	.long	0xFFFFFFFF
VARIABLE(grub_install_bsd_part)
	.long	0xFFFFFFFF
VARIABLE(grub_prefix)
	.string "/boot/grub"

	/*
	 *  Leave some breathing room for the prefix.
	 */

	. = EXT_C(start) + 0x50

/* the real mode code continues... */
codestart:
	cli		/* we're not safe here! */

	/* set up %ds, %ss, and %es */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	/* set up the real mode/BIOS stack */
	movl	$GRUB_MEMORY_MACHINE_REAL_STACK, %ebp
	movl	%ebp, %esp

	sti		/* we're safe again */

	/* save boot drive reference */
	ADDR32	movb	%dl, EXT_C(grub_boot_drive)

	/* reset disk system (%ah = 0) */
	int	$0x13

	/* transition to protected mode */
	DATA32	call real_to_prot

	/* The ".code32" directive takes GAS out of 16-bit mode. */
	.code32

	incl	%eax
	call	EXT_C(grub_gate_a20)

	/* decompress the compressed part and put the result at 1MB */
	movl	$0x100000, %esi
	movl	$(START_SYMBOL + GRUB_KERNEL_MACHINE_RAW_SIZE), %edi

	pushl	%esi
	pushl	EXT_C(grub_compressed_size)
	pushl	%edi
	call	lzo1x_decompress
	addl	$12, %esp

	/* copy back the decompressed part */
	movl	%eax, %ecx
	cld
	rep
	movsb

	/* copy modules before cleaning out the bss */
	movl	EXT_C(grub_total_module_size), %ecx
	movl	EXT_C(grub_kernel_image_size), %esi
	addl	%ecx, %esi
	addl	$START_SYMBOL, %esi
	decl	%esi
	movl	$END_SYMBOL, %edi
	addl	%ecx, %edi
	decl	%edi
	std
	rep
	movsb
	
	/* clean out the bss */
	movl	$BSS_START_SYMBOL, %edi

	/* compute the bss length */
	movl	$END_SYMBOL, %ecx
	subl	%edi, %ecx
			
	/* clean out */
	xorl	%eax, %eax
	cld
	rep
	stosb
	
	/*
	 *  Call the start of main body of C code.
	 */
	call EXT_C(grub_main)


/*
 *  This is the area for all of the special variables.
 */

	.p2align	2	/* force 4-byte alignment */

protstack:
	.long	GRUB_MEMORY_MACHINE_PROT_STACK

VARIABLE(grub_boot_drive)
	.long	0

VARIABLE(grub_start_addr)
	.long	START_SYMBOL

VARIABLE(grub_end_addr)
	.long	END_SYMBOL
	
VARIABLE(grub_apm_bios_info)
	.word	0	/* version */
	.word	0	/* cseg */
	.long	0	/* offset */
	.word	0	/* cseg_16 */
	.word	0	/* dseg_16 */
	.word	0	/* cseg_len */
	.word	0	/* cseg_16_len */
	.word	0	/* dseg_16_len */
	
/*
 * This is the Global Descriptor Table
 *
 *  An entry, a "Segment Descriptor", looks like this:
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|0|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |
 * |                             |                            |
 * ------------------------------------------------------------
 *
 *  Note the ordering of the data items is reversed from the above
 *  description.
 */

	.p2align	2	/* force 4-byte alignment */
gdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	/* code segment */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	/* data segment */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* 16 bit real mode CS */
	.word	0xFFFF, 0
	.byte	0, 0x9E, 0, 0

	/* 16 bit real mode DS */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0


/* this is the GDT descriptor */
gdtdesc:
	.word	0x27			/* limit */
	.long	gdt			/* addr */

	
/*
 *  These next two routines, "real_to_prot" and "prot_to_real" are structured
 *  in a very specific way.  Be very careful when changing them.
 *
 *  NOTE:  Use of either one messes up %eax and %ebp.
 */

real_to_prot:
	.code16
	cli

	/* load the GDT register */
	DATA32	ADDR32	lgdt	gdtdesc

	/* turn on protected mode */
	movl	%cr0, %eax
	orl	$GRUB_MEMORY_MACHINE_CR0_PE_ON, %eax
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	DATA32	ljmp	$GRUB_MEMORY_MACHINE_PROT_MODE_CSEG, $protcseg

	.code32
protcseg:
	/* reload other segment registers */
	movw	$GRUB_MEMORY_MACHINE_PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* put the return address in a known safe location */
	movl	(%esp), %eax
	movl	%eax, GRUB_MEMORY_MACHINE_REAL_STACK

	/* get protected mode stack */
	movl	protstack, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* get return address onto the right stack */
	movl	GRUB_MEMORY_MACHINE_REAL_STACK, %eax
	movl	%eax, (%esp)

	/* zero %eax */
	xorl	%eax, %eax

	/* return on the old (or initialized) stack! */
	ret


prot_to_real:
	/* just in case, set GDT */
	lgdt	gdtdesc

	/* save the protected mode stack */
	movl	%esp, %eax
	movl	%eax, protstack

	/* get the return address */
	movl	(%esp), %eax
	movl	%eax, GRUB_MEMORY_MACHINE_REAL_STACK

	/* set up new stack */
	movl	$GRUB_MEMORY_MACHINE_REAL_STACK, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* set up segment limits */
	movw	$GRUB_MEMORY_MACHINE_PSEUDO_REAL_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	/* jump to a 16 bit segment */
	ljmp	$GRUB_MEMORY_MACHINE_PSEUDO_REAL_CSEG, $tmpcseg

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andl 	$(~GRUB_MEMORY_MACHINE_CR0_PE_ON), %eax
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	DATA32	ljmp	$0, $realcseg

realcseg:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* zero %eax */
	xorl	%eax, %eax

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* restore interrupts */
	sti

	/* return on new stack! */
	DATA32	ret

	.code32


/*
 * grub_gate_a20(int on)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 */

FUNCTION(grub_gate_a20)
	movl	%eax, %ecx

	call    gloop1

	movb	$0xd1, %al
	outb	$0x64

gloopint1:
	inb	$0x64
	andb	$0x02, %al
	jnz	gloopint1

	movb	$0xdd, %al
	testl	%ecx, %ecx
	jz	gdoit

	orb	$0x02, %al
gdoit:
	outb	$0x60

	call	gloop1

	/* output a dummy command (USB keyboard hack) */
	movb	$0xff, %al
	outb	$0x64
	call	gloop1
	
	ret

gloop1:
	inb	$0x64
	andb	$0x02, %al
	jnz	gloop1

gloop2:
	inb	$0x64
	andb	$0x01, %al
	jz	gloop2ret
	inb	$0x60
	jmp	gloop2

gloop2ret:
	ret


#include "lzo1x.S"


/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */

FUNCTION(grub_stop)
	call	prot_to_real

	/*
	 * This next part is sort of evil.  It takes advantage of the
	 * byte ordering on the x86 to work in either 16-bit or 32-bit
	 * mode, so think about it before changing it.
	 */

FUNCTION(grub_hard_stop)
	hlt
	jmp EXT_C(grub_hard_stop)


/*
 * grub_stop_floppy()
 *
 * Stop the floppy drive from spinning, so that other software is
 * jumped to with a known state.
 */
FUNCTION(grub_stop_floppy)
	movw	$0x3F2, %dx
	xorb	%al, %al
	outb	%al, %dx
	ret

/*
 * grub_reboot()
 *
 * Reboot the system. At the moment, rely on BIOS.
 */
FUNCTION(grub_reboot)
	call	prot_to_real
	.code16
	/* cold boot */
	movw	$0x0472, %di
	movw	%ax, (%di)
	ljmp	$0xFFFF, $0x0000
	.code32
	
/*
 * grub_halt(int no_apm)
 *
 * Halt the system, using APM if possible. If NO_APM is true, don't use
 * APM even if it is available.
 */
FUNCTION(grub_halt)
	/* see if zero */
	testl	%eax, %eax
	jnz	EXT_C(grub_stop)

	call	prot_to_real
	.code16
	
	/* detect APM */
	movw	$0x5300, %ax
	xorw	%bx, %bx
	int	$0x15
	jc	EXT_C(grub_hard_stop)
	/* don't check %bx for buggy BIOSes... */

	/* disconnect APM first */
	movw	$0x5304, %ax
	xorw	%bx, %bx
	int	$0x15

	/* connect APM */
	movw	$0x5301, %ax
	xorw	%bx, %bx
	int	$0x15
	jc	EXT_C(grub_hard_stop)

	/* set APM protocol level - 1.1 or bust. (this covers APM 1.2 also) */
	movw	$0x530E, %ax
	xorw	%bx, %bx
	movw	$0x0101, %cx
	int	$0x15
	jc	EXT_C(grub_hard_stop)
	
	/* set the power state to off */
	movw	$0x5307, %ax
	movw	$1, %bx
	movw	$3, %cx
	int	$0x15

	/* shouldn't reach here */
	jmp	EXT_C(grub_hard_stop)
	.code32
	
	
/*
 *  void grub_chainloader_real_boot (int drive, void *part_addr)
 *
 *  This starts another boot loader.
 */

FUNCTION(grub_chainloader_real_boot)
	pushl	%edx
	pushl	%eax

	call	EXT_C(grub_dl_unload_all)

	/* set up to pass boot drive */
	popl	%edx

	/* ESI must point to a partition table entry */
	popl	%esi

	/* Turn off Gate A20 */
	xorl	%eax, %eax
	call	EXT_C(grub_gate_a20)
	
	call	prot_to_real
	.code16
	ljmp	$0, $GRUB_MEMORY_MACHINE_BOOT_LOADER_ADDR
	.code32


/*
 * void grub_linux_boot_zimage (void)
 */
VARIABLE(grub_linux_prot_size)
	.long	0
VARIABLE(grub_linux_tmp_addr)
	.long	0
VARIABLE(grub_linux_real_addr)
	.long	0
	
FUNCTION(grub_linux_boot_zimage)
	/* copy the kernel */
	movl	EXT_C(grub_linux_prot_size), %ecx
	addl	$3, %ecx
	shrl	$2, %ecx
	movl	$GRUB_LINUX_BZIMAGE_ADDR, %esi
	movl	$GRUB_LINUX_ZIMAGE_ADDR, %edi
	cld
	rep
	movsl

FUNCTION(grub_linux_boot_bzimage)
	call	EXT_C(grub_dl_unload_all)
	
	movl	EXT_C(grub_linux_real_addr), %ebx

	/* copy the real mode code */
	movl	EXT_C(grub_linux_tmp_addr), %esi
	movl	%ebx, %edi
	movl	$GRUB_LINUX_SETUP_MOVE_SIZE, %ecx
	cld
	rep
	movsb

	/* change %ebx to the segment address */
	shrl	$4, %ebx
	movl	%ebx, %eax
	addl	$0x20, %eax
	movw	%ax, linux_setup_seg

	/* XXX new stack pointer in safe area for calling functions */
	movl	$0x4000, %esp
	call	EXT_C(grub_stop_floppy)

	/* final setup for linux boot */
	call	prot_to_real
	.code16

	cli
	movw	%bx, %ss
	movw	$GRUB_LINUX_SETUP_STACK, %sp

	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs

	/* ljmp */
	.byte	0xea
	.word	0
linux_setup_seg:
	.word	0
	.code32

		
/*
 * This starts the multiboot kernel.
 */

FUNCTION(grub_multiboot_real_boot)
	/* Push the entry address on the stack.  */
	pushl	%eax
	/* Move the address of the multiboot information structure to ebx.  */
	movl	%edx,%ebx
	
	/* Unload all modules and stop the floppy driver.  */
	call	EXT_C(grub_dl_unload_all)
	call	EXT_C(grub_stop_floppy)

	/* Interrupts should be disabled.  */
	cli
	
	/* Move the magic value into eax and jump to the kernel.  */
	movl	$GRUB_MB_MAGIC2,%eax
	popl	%ecx
	jmp 	*%ecx
	
	
/*
 *   int grub_biosdisk_rw_int13_extensions (int ah, int drive, void *dap)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

FUNCTION(grub_biosdisk_rw_int13_extensions)
	pushl	%ebp
	pushl	%esi

	/* compute the address of disk_address_packet */
	movw	%cx, %si
	xorw	%cx, %cx
	shrl	$4, %ecx	/* save the segment to cx */

	/* ah */
	movb	%al, %dh
	/* enter real mode */
	call	prot_to_real
	
	.code16
	movb	%dh, %ah
	movw	%cx, %ds
	int	$0x13		/* do the operation */
	movb	%ah, %dl	/* save return value */
	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* back to protected mode */
	DATA32	call	real_to_prot
	.code32

	movb	%dl, %al	/* return value in %eax */

	popl	%esi
	popl	%ebp

	ret
	
/*
 *   int grub_biosdisk_rw_standard (int ah, int drive, int coff, int hoff,
 *                                  int soff, int nsec, int segment)
 *
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */

FUNCTION(grub_biosdisk_rw_standard)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%edi
	pushl	%esi

	/* set up CHS information */

	/* set %ch to low eight bits of cylinder */
	xchgb	%cl, %ch
	/* set bits 6-7 of %cl to high two bits of cylinder */
	shlb	$6, %cl
	/* set bits 0-5 of %cl to sector */
	addb	0xc(%ebp), %cl
	/* set %dh to head */
	movb	0x8(%ebp), %dh
	/* set %ah to AH */
	movb	%al, %ah
	/* set %al to NSEC */
	movb	0x10(%ebp), %al
	/* save %ax in %di */
	movw	%ax, %di
	/* save SEGMENT in %bx */
	movw	0x14(%ebp), %bx
		
	/* enter real mode */
	call	prot_to_real

	.code16
	movw	%bx, %es
	xorw	%bx, %bx
	movw	$3, %si		/* attempt at least three times */

1:	
	movw	%di, %ax
	int	$0x13		/* do the operation */
	jnc	2f		/* check if successful */

	movb	%ah, %bl	/* save return value */
	/* if fail, reset the disk system */
	xorw	%ax, %ax
	int	$0x13
	
	decw	%si
	cmpw	$0, %si
	je	2f
	xorb	%bl, %bl
	jmp	1b		/* retry */
2:	
	/* back to protected mode */
	DATA32	call	real_to_prot
	.code32

	movb	%bl, %al	/* return value in %eax */
	
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp

	ret	$(4 * 4)


/*
 *   int grub_biosdisk_check_int13_extensions (int drive)
 *
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */

FUNCTION(grub_biosdisk_check_int13_extensions)
	pushl	%ebp
	pushl	%ebx

	/* drive */
	movb	%al, %dl
	/* enter real mode */
	call	prot_to_real

	.code16
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13		/* do the operation */
	
	/* check the result */
	jc	1f
	cmpw	$0xaa55, %bx
	jne	1f

	movb	%ah, %bl	/* save the major version into %bl */

	/* check if AH=0x42 is supported */
	andw	$1, %cx
	jnz	2f
	
1:
	xorb	%bl, %bl
2:
	/* back to protected mode */
	DATA32	call	real_to_prot
	.code32

	movb	%bl, %al	/* return value in %eax */

	popl	%ebx
	popl	%ebp

	ret


/*
 *   int grub_biosdisk_get_diskinfo_int13_extensions (int drive, void *drp)
 *
 *   Return the geometry of DRIVE in a drive parameters, DRP. If an error
 *   occurs, then return non-zero, otherwise zero.
 */

FUNCTION(grub_biosdisk_get_diskinfo_int13_extensions)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi

	/* compute the address of drive parameters */
	movw	%dx, %si
	xorw	%dx, %dx
	shrl	$4, %edx
	movw	%dx, %bx	/* save the segment into %bx */
	/* drive */
	movb	%al, %dl
	/* enter real mode */
	call	prot_to_real

	.code16
	movb	$0x48, %ah
	movw	%bx, %ds
	int	$0x13		/* do the operation */
	movb	%ah, %bl	/* save return value in %bl */
	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* back to protected mode */
	DATA32	call	real_to_prot
	.code32

	movb	%bl, %al	/* return value in %eax */

	popl	%esi
	popl	%ebx
	popl	%ebp
	
	ret


/*
 *   int grub_biosdisk_get_diskinfo_standard (int drive,
 *                                            unsigned long *cylinders, 
 *                                            unsigned long *heads,
 *                                            unsigned long *sectors)
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

FUNCTION(grub_biosdisk_get_diskinfo_standard)
	pushl	%ebp
	pushl	%ebx
	pushl	%edi

	/* push CYLINDERS */
	pushl	%edx
	/* push HEADS */
	pushl	%ecx
	/* SECTORS is on the stack */
	
	/* drive */
	movb	%al, %dl
	/* enter real mode */
	call	prot_to_real

	.code16
	movb	$0x8, %ah
	int	$0x13		/* do the operation */
	/* check if successful */
	testb	%ah, %ah
	jnz	1f
	/* bogus BIOSes may not return an error number */
	testb	$0x3f, %cl	/* 0 sectors means no disk */
	jnz	1f		/* if non-zero, then succeed */
	/* XXX 0x60 is one of the unused error numbers */
	movb	$0x60, %ah
1:
	movb	%ah, %bl	/* save return value in %bl */
	/* back to protected mode */
	DATA32	call	real_to_prot
	.code32

	/* pop HEADS */
	popl	%edi
	movb	%dh, %al
	incl	%eax	/* the number of heads is counted from zero */
	movl	%eax, (%edi)

	/* pop CYLINDERS */
	popl	%edi
	movb	%ch, %al
	movb	%cl, %ah
	shrb	$6, %ah	/* the number of cylinders is counted from zero */
	incl	%eax
	movl	%eax, (%edi)

	/* SECTORS */
	movl	0x10(%esp), %edi
	andb	$0x3f, %cl
	movzbl	%cl, %eax
	movl	%eax, (%edi)
	
	xorl	%eax, %eax
	movb	%bl, %al	/* return value in %eax */

	popl	%edi
	popl	%ebx
	popl	%ebp

	ret	$4


/*
 * int grub_biosdisk_get_num_floppies (void)
 */
FUNCTION(grub_biosdisk_get_num_floppies)
	pushl	%ebp

	xorl	%edx, %edx
	call	prot_to_real
	
	.code16
	/* reset the disk system first */
	int	$0x13
1:
	stc
	
	/* call GET DISK TYPE */
	movb	$0x15, %ah
	int	$0x13

	jc	2f

	/* check if this drive exists */	
	testb	$0x3, %ah
	jz	2f

	incb	%dl
	cmpb	$2, %dl
	jne	1b
2:
	DATA32	call	real_to_prot
	.code32

	movl	%edx, %eax
	popl	%ebp
	ret
	
	
/*
 *
 * grub_get_memsize(i) :  return the memory size in KB. i == 0 for conventional
 *		memory, i == 1 for extended memory
 *	BIOS call "INT 12H" to get conventional memory size
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *		Both have the return value in AX.
 *
 */

FUNCTION(grub_get_memsize)
	pushl	%ebp

	movl	%eax, %edx

	call	prot_to_real	/* enter real mode */
	.code16

	testl	%edx, %edx
	jnz	xext

	int	$0x12
	jmp	xdone

xext:
	movb	$0x88, %ah
	int	$0x15

xdone:
	movw	%ax, %dx

	DATA32	call	real_to_prot
	.code32

	movw	%dx, %ax

	popl	%ebp
	ret


/*
 *
 * grub_get_eisa_mmap() :  return packed EISA memory map, lower 16 bits is
 *		memory between 1M and 16M in 1K parts, upper 16 bits is
 *		memory above 16M in 64K parts.  If error, return zero.
 *	BIOS call "INT 15H, AH=E801H" to get EISA memory map,
 *		AX = memory between 1M and 16M in 1K parts.
 *		BX = memory above 16M in 64K parts.
 *
 */

FUNCTION(grub_get_eisa_mmap)
	pushl	%ebp
	pushl	%ebx

	call	prot_to_real	/* enter real mode */
	.code16

	movw	$0xe801, %ax
	int	$0x15

	shll	$16, %ebx
	movw	%ax, %bx

	DATA32	call	real_to_prot
	.code32

	cmpb	$0x86, %bh
	je	xnoteisa

	movl	%ebx, %eax

xnoteisa:
	popl	%ebx
	popl	%ebp
	ret

/*
 *
 * grub_get_mmap_entry(addr, cont) : address and old continuation value (zero to
 *		start), for the Query System Address Map BIOS call.
 *
 *  Sets the first 4-byte int value of "addr" to the size returned by
 *  the call.  If the call fails, sets it to zero.
 *
 *	Returns:  new (non-zero) continuation value, 0 if done.
 */

FUNCTION(grub_get_mmap_entry)
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	/* push ADDR */
	pushl	%eax
	
	/* place address (+4) in ES:DI */
	addl	$4, %eax
	movl	%eax, %edi
	andl	$0xf, %edi
	shrl	$4, %eax
	movl	%eax, %esi

	/* set continuation value */
	movl	%edx, %ebx

	/* set default maximum buffer size */
	movl	$0x14, %ecx

	/* set EDX to 'SMAP' */
	movl	$0x534d4150, %edx

	call	prot_to_real	/* enter real mode */
	.code16

	movw	%si, %es
	movl	$0xe820, %eax
	int	$0x15

	DATA32	jc	xnosmap

	cmpl	$0x534d4150, %eax
	jne	xnosmap

	cmpl	$0x14, %ecx
	jl	xnosmap

	cmpl	$0x400, %ecx
	jg	xnosmap

	jmp	xsmap

xnosmap:
	xorl	%ecx, %ecx

xsmap:
	DATA32	call	real_to_prot
	.code32

	/* write length of buffer (zero if error) into ADDR */
	popl	%eax
	movl	%ecx, (%eax)

	/* set return value to continuation */
	movl	%ebx, %eax

	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	ret

	
/*
 * void grub_console_real_putchar (int c)
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUPUT BIOS call doesn't
 * support setting a background attribute.
 */
FUNCTION(grub_console_real_putchar)
	movl	%eax, %edx
	pusha
	movb	EXT_C(grub_console_cur_color), %bl
	
	call	prot_to_real
	.code16
	movb	%dl, %al
	xorb	%bh, %bh

	/* use teletype output if control character */
	cmpb	$0x7, %al
	je	1f
	cmpb	$0x8, %al
	je	1f
	cmpb	$0xa, %al
	je	1f
	cmpb	$0xd, %al
	je	1f

	/* save the character and the attribute on the stack */
	pushw	%ax
	pushw	%bx
	
	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

	/* check the column with the width */
	cmpb	$79, %dl
	jl	2f
	
	/* print CR and LF, if next write will exceed the width */	
	movw	$0x0e0d, %ax
	int	$0x10
	movb	$0x0a, %al
	int	$0x10
	
	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

2:	
	/* restore the character and the attribute */
	popw	%bx
	popw	%ax
	
	/* write the character with the attribute */
	movb	$0x9, %ah
	movw	$1, %cx
	int	$0x10

	/* move the cursor forward */
	incb	%dl
	movb	$0x2, %ah
	int	$0x10

	jmp	3f

1:	movw	$1, %bx
	movb	$0xe, %ah
	int	$0x10
	
3:	DATA32	call	real_to_prot
	.code32
	
	popa
	ret
	

/*
 * int grub_console_getkey (void)
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

/* this table is used in translate_keycode below */
translation_table:
	.word	GRUB_CONSOLE_KEY_LEFT, 2
	.word	GRUB_CONSOLE_KEY_RIGHT, 6
	.word	GRUB_CONSOLE_KEY_UP, 16
	.word	GRUB_CONSOLE_KEY_DOWN, 14
	.word	GRUB_CONSOLE_KEY_HOME, 1
	.word	GRUB_CONSOLE_KEY_END, 5
	.word	GRUB_CONSOLE_KEY_DC, 4
	.word	GRUB_CONSOLE_KEY_BACKSPACE, 8
	.word	GRUB_CONSOLE_KEY_PPAGE, 7
	.word	GRUB_CONSOLE_KEY_NPAGE, 3
	.word	0
	
/*
 * translate_keycode translates the key code %dx to an ascii code.
 */
	.code16

translate_keycode:
	pushw	%bx
	pushw	%si
	
	movw	$ABS(translation_table), %si
	
1:	lodsw
	/* check if this is the end */
	testw	%ax, %ax
	jz	2f
	/* load the ascii code into %ax */
	movw	%ax, %bx
	lodsw
	/* check if this matches the key code */
	cmpw	%bx, %dx
	jne	1b
	/* translate %dx, if successful */
	movw	%ax, %dx

2:	popw	%si
	popw	%bx
	ret

	.code32
	
FUNCTION(grub_console_getkey)
	pushl	%ebp

	call	prot_to_real
	.code16

	int	$0x16

	movw	%ax, %dx		/* real_to_prot uses %eax */
	call	translate_keycode
		
	DATA32	call	real_to_prot
	.code32

	movw	%dx, %ax

	popl	%ebp
	ret


/*
 * int grub_console_checkkey (void)
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 */
FUNCTION(grub_console_checkkey)
	pushl	%ebp
	xorl	%edx, %edx
	
	call	prot_to_real	/* enter real mode */
	.code16

	movb	$0x1, %ah
	int	$0x16

	jz	notpending
	
	movw	%ax, %dx
	DATA32	jmp	pending

notpending:
	decl	%edx

pending:
	DATA32	call	real_to_prot
	.code32

	movl	%edx, %eax

	popl	%ebp
	ret

	
/*
 * grub_uint16_t grub_console_getxy (void)
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


FUNCTION(grub_console_getxy)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	call	prot_to_real
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

	DATA32	call	real_to_prot
	.code32

	movb	%dl, %ah
	movb	%dh, %al

	popl	%ebx
	popl	%ebp
	ret


/*
 * void grub_console_gotoxy(grub_uint8_t x, grub_uint8_t y)
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


FUNCTION(grub_console_gotoxy)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	movb	%dl, %dh	/* %dh = y */
	movb	%al, %dl	/* %dl = x */

	call	prot_to_real
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x2, %ah
	int	$0x10			/* set cursor position */

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret

	
/*
 * void grub_console_cls (void)
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */

FUNCTION(grub_console_cls)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	call	prot_to_real
	.code16

	/* move the cursor to the beginning */
	movb	$0x02, %ah
	xorb	%bh, %bh
	xorw	%dx, %dx
	int	$0x10

	/* write spaces to the entire screen */
	movw	$0x0920, %ax
	movw	$0x07, %bx
	movw	$(80 * 25), %cx
        int	$0x10

	/* move back the cursor */
	movb	$0x02, %ah
	int	$0x10

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret

	
/*
 * void grub_console_setcursor (int on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */

console_cursor_state:
	.byte	1
console_cursor_shape:
	.word	0
	
FUNCTION(grub_console_setcursor)
	pushl	%ebp
	pushl	%ebx

	/* push ON */
	pushl	%eax
	
	/* check if the standard cursor shape has already been saved */
	movw	console_cursor_shape, %ax
	testw	%ax, %ax
	jne	1f

	call	prot_to_real
	.code16

	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10

	DATA32	call	real_to_prot
	.code32

	movw	%cx, console_cursor_shape
1:
	/* set %cx to the designated cursor shape */
	movw	$0x2000, %cx
	popl	%eax
	testl	%eax, %eax
	jz	2f
	movw	console_cursor_shape, %cx
2:	
	call	prot_to_real
	.code16

	movb    $0x1, %ah
	int     $0x10 

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret
		
/*
 * grub_getrtsecs()
 *	if a seconds value can be read, read it and return it (BCD),
 *      otherwise return 0xFF
 * BIOS call "INT 1AH Function 02H" to check whether a character is pending
 *	Call with	%ah = 0x2
 *	Return:
 *		If RT Clock can give correct values
 *			%ch = hour (BCD)
 *			%cl = minutes (BCD)
 *                      %dh = seconds (BCD)
 *                      %dl = daylight savings time (00h std, 01h daylight)
 *			Carry flag = clear
 *		else
 *			Carry flag = set
 *                         (this indicates that the clock is updating, or
 *                          that it isn't running)
 */
FUNCTION(grub_getrtsecs)
	pushl	%ebp

	call	prot_to_real	/* enter real mode */
	.code16

	clc
	movb	$0x2, %ah
	int	$0x1a

	DATA32	jnc	gottime
	movb	$0xff, %dh

gottime:
	DATA32	call	real_to_prot
	.code32

	movb	%dh, %al

	popl	%ebp
	ret

	
/*
 * grub_get_rtc()
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
FUNCTION(grub_get_rtc)
	pushl	%ebp

	call	prot_to_real	/* enter real mode */
	.code16

	/* %ax is already zero */
        int	$0x1a

	DATA32	call	real_to_prot
	.code32

	movl	%ecx, %eax
	shll	$16, %eax
	movw	%dx, %ax

	popl	%ebp
	ret


/*
 * unsigned char grub_vga_set_mode (unsigned char mode)
 */
FUNCTION(grub_vga_set_mode)
	pushl	%ebp
	pushl	%ebx
	movl	%eax, %ecx

	call	prot_to_real
	.code16
	/* get current mode */
	xorw	%bx, %bx
	movb	$0x0f, %ah
	int	$0x10
	movb	%al, %dl

	/* set the new mode */
	movb	%cl, %al
	xorb	%ah, %ah
	int	$0x10

	DATA32	call	real_to_prot
	.code32

	movb	%dl, %al
	popl	%ebx
	popl	%ebp
	ret


/*
 * unsigned char *grub_vga_get_font (void)
 */
FUNCTION(grub_vga_get_font)
	pushl	%ebp
	pushl	%ebx

	call	prot_to_real
	.code16
	movw	$0x1130, %ax
	movb	$0x06, %bh
	int	$0x10
	movw	%es, %bx
	movw	%bp, %dx
	DATA32	call	real_to_prot
	.code32

	movzwl	%bx, %ecx
	shll	$4, %ecx
	movw	%dx, %ax
	addl	%ecx, %eax

	popl	%ebx
	popl	%ebp
	ret
