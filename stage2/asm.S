/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999   Free Software Foundation, Inc.
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

#define ASM_FILE

#include "shared.h"

	.file	"asm.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

ENTRY(start)
	/* Suppress ld warnings. */
	.globl _start
_start:
	/*
	 *  Guarantee that "start" is loaded at 0x0:0x8000 in stage2 and
	 *  at 0x0:0x2000 in stage1.5.
	 */
#ifndef STAGE1_5
	ljmp $0, $(codestart - EXT_C(start) + 0x8000)
#else
	ljmp $0, $(codestart - EXT_C(start) + 0x2000)
#endif

	/*
	 *  Compatibility version number
	 *
	 *  These MUST be at byte offset 6 and 7 of the executable
	 *  DO NOT MOVE !!!
	 */
	. = EXT_C(start) + 0x6
	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR

	/*
	 *  This is a special data area 8 bytes from the beginning.
	 */

	. = EXT_C(start) + 0x8

VARIABLE(install_partition)
	.long	0x020000
VARIABLE(stage2_id)
	.byte	STAGE2_ID
VARIABLE(version_string)
	.string VERSION
VARIABLE(config_file)
#ifndef CONFIG_FILE_ASM
	.string "/boot/grub/menu.lst"
#else   /* CONFIG_FILE_ASM */
	CONFIG_FILE_ASM
#endif  /* CONFIG_FILE_ASM */

	/*
	 *  Leave some breathing room for the config file name.
	 */

	. = EXT_C(start) + 0x70

/* the real mode code continues... */
codestart:
	cli		/* we're not safe here! */

	/* set up %ds, %ss, and %es */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	/* set up the real mode/BIOS stack */
	movl	$STACKOFF, %ebp
	movl	%ebp, %esp

	sti		/* we're safe again */

	/* save boot drive reference */
	ADDR32	movb	%dl, EXT_C(boot_drive)

	/* reset disk system (%ah = 0) */
	int	$0x13

	/* transition to protected mode */
	DATA32	call EXT_C(real_to_prot)

	/* The ".code32" directive takes GAS out of 16-bit mode. */
	.code32

	/*
	 *  Call the start of main body of C code, which does some
	 *  of it's own initialization before transferring to "cmain".
	 */
	call EXT_C(init_bios_info)


/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */

ENTRY(stop)
	call	EXT_C(prot_to_real)

	/*
	 * This next part is sort of evil.  It takes advantage of the
	 * byte ordering on the x86 to work in either 16-bit or 32-bit
	 * mode, so think about it before changing it.
	 */

ENTRY(hard_stop)
	hlt
	jmp EXT_C(hard_stop)

#ifndef STAGE1_5
/*
 * stop_floppy()
 *
 * Stops the floppy drive from spinning, so that other software is
 * jumped to with a known state.
 */
ENTRY(stop_floppy)
	movw	$0x3F2, %dx
	xorb	%al, %al
	outb	%al, %dx
	ret

/*
 * chain_stage1(segment, offset, part_table_addr)
 *
 *  This starts another stage1 loader, at segment:offset.
 */

ENTRY(chain_stage1)
	/* no need to save anything, just use %esp */

	/* store %ESI, presuming %ES is 0 */
	movl	0xc(%esp), %esi

	/* store new offset */
	movl	0x8(%esp), %eax
	movl	%eax, offset

	/* store new segment */
	movw	0x4(%esp), %ax
	movw	%ax, segment

	/* set up to pass boot drive */
	movb	EXT_C(boot_drive), %dl

	call	EXT_C(prot_to_real)
	.code16

	DATA32	ADDR32	ljmp	(offset)
	.code32
#endif /* STAGE1_5 */


#ifdef STAGE1_5
/*
 * chain_stage2(segment, offset)
 *
 *  This starts another stage2 loader, at segment:offset.  It presumes
 *  that the other one starts with this same "asm.S" file, and passes
 *  parameters by writing the embedded install variables.
 */

ENTRY(chain_stage2)
	/* no need to save anything, just use %esp */

	/* store new offset */
	movl	0x8(%esp), %eax
	movl	%eax, offset
	movl	%eax, %ebx

	/* store new segment */
	movw	0x4(%esp), %ax
	movw	%ax, segment
	shll	$4, %eax

	/* generate linear address */
	addl	%eax, %ebx

	/* set up to pass the partition where stage2 is located in */
	movl	EXT_C(current_partition), %eax
	movl	%eax, EXT_C(install_partition)-EXT_C(start)(%ebx)

	/* set up to pass the drive where stage2 is located in */
	movb	EXT_C(current_drive), %dl

	call	EXT_C(prot_to_real)
	.code16

	DATA32	ADDR32	ljmp	(offset)

	.code32
#endif /* STAGE1_5 */
	
/*
 *  These next two routines, "real_to_prot" and "prot_to_real" are structured
 *  in a very specific way.  Be very careful when changing them.
 *
 *  NOTE:  Use of either one messes up %eax and %ebp.
 */

ENTRY(real_to_prot)
	.code16
	cli

	/* load the GDT register */
	DATA32	ADDR32	lgdt	gdtdesc

	/* turn on protected mode */
	movl	%cr0, %eax
	orl	$CR0_PE_ON, %eax
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	DATA32	ljmp	$PROT_MODE_CSEG, $protcseg

	/*
	 *  The ".code32" directive only works in GAS, the GNU assembler!
	 *  This gets out of "16-bit" mode.
	 */
	.code32

protcseg:
	/* reload other segment registers */
	movw	$PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* put the return address in a known safe location */
	movl	(%esp), %eax
	movl	%eax, STACKOFF

	/* get protected mode stack */
	movl	protstack, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* get return address onto the right stack */
	movl	STACKOFF, %eax
	movl	%eax, (%esp)

	/* zero %eax */
	xorl	%eax, %eax

	/* return on the old (or initialized) stack! */
	ret


ENTRY(prot_to_real)
	/* just in case, set GDT */
	lgdt	gdtdesc

	/* save the protected mode stack */
	movl	%esp, %eax
	movl	%eax, protstack

	/* get the return address */
	movl	(%esp), %eax
	movl	%eax, STACKOFF

	/* set up new stack */
	movl	$STACKOFF, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	ljmp	$PSEUDO_RM_CSEG, $tmpcseg	/* jump to a 16 bit segment */

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andl 	$CR0_PE_OFF, %eax
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
 *   int biosdisk_int13_extensions (int ah, int drive, void *dap)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

ENTRY(biosdisk_int13_extensions)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ecx
	pushl	%edx
	pushl	%esi

	/* compute the address of disk_address_packet */
	movl	0x10(%ebp), %eax
	movw	%ax, %si
	xorw	%ax, %ax
	shrl	$4, %eax
	movw	%ax, %cx	/* save the segment to cx */

	/* drive */
	movb	0xc(%ebp), %dl
	/* ah */
	movb	0x8(%ebp), %dh
	/* enter real mode */
	call	EXT_C(prot_to_real)
	
	.code16
	movb	%dh, %ah
	movw	%cx, %ds
	int	$0x13		/* do the operation */
	movb	%ah, %dl	/* save return value */
	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dl, %al	/* return value in %eax */

	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebp

	ret
	
/*
 *   int biosdisk_standard (int ah, int drive, int coff, int hoff, int soff,
 *                          int nsec, int segment)
 *
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */

ENTRY(biosdisk_standard)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi

	/* set up CHS information */
	movl	0x10(%ebp), %eax
	movb	%al, %ch
	movb	0x18(%ebp), %al
	shlb	$2, %al
	shrw	$2, %ax
	movb	%al, %cl
	movb	0x14(%ebp), %dh
	/* drive */
	movb	0xc(%ebp), %dl
	/* segment */
	movw	0x20(%ebp), %bx
	/* save nsec and ah to %di */
	movb	0x8(%ebp), %ah
	movb	0x1c(%ebp), %al
	movw	%ax, %di
	/* enter real mode */
	call	EXT_C(prot_to_real)

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
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%bl, %al	/* return value in %eax */
	
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp

	ret


/*
 *   int check_int13_extensions (int drive)
 *
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */

ENTRY(check_int13_extensions)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13		/* do the operation */
	
	/* check the result */
	jc	1f
	cmpw	$0xaa55, %bx
	jne	1f
	andw	$1, %cx
	jz	1f

	movb	%ah, %bl	/* save the major version into %bl */
	jmp	2f
1:
	xorb	%bl, %bl
2:
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%bl, %al	/* return value in %eax */

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp

	ret


/*
 *   int get_diskinfo_int13_extensions (int drive, void *drp)
 *
 *   Return the geometry of DRIVE in a drive parameters, DRP. If an error
 *   occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_int13_extensions)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%edx
	pushl	%esi

	/* compute the address of drive parameters */
	movl	0xc(%ebp), %eax
	movw	%ax, %si
	xorw	%ax, %ax
	shrl	$4, %eax
	movw	%ax, %bx	/* save the segment into %bx */
	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	movb	$0x48, %ah
	movw	%bx, %ds
	int	$0x13		/* do the operation */
	movb	%ah, %bl	/* save return value in %bl */
	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%bl, %al	/* return value in %eax */

	popl	%esi
	popl	%edx
	popl	%ebx
	popl	%ebp
	
	ret


/*
 *   int get_diskinfo_standard (int drive, unsigned long *cylinders, 
 *                              unsigned long *heads, unsigned long *sectors)
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_standard)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

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
	DATA32	call	EXT_C(real_to_prot)
	.code32

	/* restore %ebp */
	leal	0x10(%esp), %ebp
	
	/* heads */
	movb	%dh, %al
	incl	%eax		/* the number of heads is counted from zero */
	movl	0x10(%ebp), %edi
	movl	%eax, (%edi)

	/* sectors */
	xorl	%eax, %eax
	movb	%cl, %al
	andb	$0x3f, %al
	movl	0x14(%ebp), %edi
	movl	%eax, (%edi)

	/* cylinders */
	shrb	$6, %cl
	movb	%cl, %ah
	movb	%ch, %al
	incl	%eax		/* the number of cylinders is 
				   counted from zero */
	movl	0xc(%ebp), %edi
	movl	%eax, (%edi)

	xorl	%eax, %eax
	movb	%bl, %al	/* return value in %eax */

	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp

	ret

		
/*
 *   int get_diskinfo_floppy (int drive, unsigned long *cylinders, 
 *                            unsigned long *heads, unsigned long *sectors)
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_floppy)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	/* init probe value */
	movl	$probe_values-1, %esi
1:
	xorw	%ax, %ax
	int	$0x13		/* reset floppy controller */

	incw	%si
	movb	(%si), %cl
	cmpb	$0, %cl		/* probe failed if zero */
	je	2f

	/* perform read */
	movw	$SCRATCHSEG, %ax
	movw	%ax, %es
	xorw	%bx, %bx
	movw	$0x0201, %ax
	movb	$0, %ch
	movb	$0, %dh
	int	$0x13

	/* FIXME: Read from floppy may fail even if the geometry is correct.
	   So should retry at least three times.  */
	jc	1b		/* next value */
	
	/* succeed */
	jmp	2f
	
probe_values:
	.byte	36, 18, 15, 9, 0
	
2:
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	/* restore %ebp */
	leal	0x10(%esp), %ebp
	
	/* cylinders */
	movl	0xc(%ebp), %eax
	movl	$80, %ebx
	movl	%ebx, (%eax)
	/* heads */
	movl	0x10(%ebp), %eax
	movl	$2, %ebx
	movl	%ebx, (%eax)
	/* sectors */
	movl	0x14(%ebp), %eax
	movzbl	%cl, %ebx
	movl	%ebx, (%eax)

	/* return value in %eax */
	xorl	%eax, %eax
	cmpb	$0, %cl
	jne	3f
	incl	%eax		/* %eax = 1 (non-zero) */
3:
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp

	ret


/*
 * putchar(c)   : Puts character on the screen, interpreting '\n' as in the
 *                UNIX fashion.
 *
 * BIOS call "INT 10H Function 0Eh" to write character to console
 *      Call with       %ah = 0x0e
 *                      %al = character
 *                      %bh = page
 *                      %bl = foreground color ( graphics modes)
 */


ENTRY(putchar)
	push	%ebp
	push	%eax
	push	%ebx

	movb	0x10(%esp), %bl

	/* if not '\n', just print the character */
	cmpb	$0xa, %bl
	jne	pc_notnewline

	/* if newline, print CR as well */
	pushl	$0xd
	call	EXT_C(putchar)
	popl	%eax

pc_notnewline:
	call	EXT_C(prot_to_real)
	.code16

	movb	%bl, %al
	movb	$0xe, %ah
	movw	$1, %bx
	int	$0x10

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%ebx
	pop	%eax
	pop	%ebp
	ret

#ifndef STAGE1_5
/* get_code_end() :  return the address of the end of the code
 * This is here so that it can be replaced by asmstub.c.
 */
ENTRY(get_code_end)
	movl	$EXT_C(end), %eax	/* will be the end of the bss */
	shrl	$2, %eax		/* Round up to the next word. */
	incl	%eax
	shll	$2, %eax
	ret
#endif /* ! STAGE1_5 */

/*
 *
 * get_memsize(i) :  return the memory size in KB. i == 0 for conventional
 *		memory, i == 1 for extended memory
 *	BIOS call "INT 12H" to get conventional memory size
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *		Both have the return value in AX.
 *
 */

ENTRY(get_memsize)
	push	%ebp
	push	%ebx

	mov	0xc(%esp), %ebx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	cmpb	$0x1, %bl
	DATA32	je	xext

	int	$0x12
	DATA32	jmp	xdone

xext:
	movb	$0x88, %ah
	int	$0x15

xdone:
	movw	%ax, %bx

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movw	%bx, %ax
	pop	%ebx
	pop	%ebp
	ret


#ifndef STAGE1_5

/*
 *
 * get_eisamemsize() :  return packed EISA memory map, lower 16 bits is
 *		memory between 1M and 16M in 1K parts, upper 16 bits is
 *		memory above 16M in 64K parts.  If error, return -1.
 *	BIOS call "INT 15H, AH=E801H" to get EISA memory map,
 *		AX = memory between 1M and 16M in 1K parts.
 *		BX = memory above 16M in 64K parts.
 *
 */

ENTRY(get_eisamemsize)
	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movw	$0xe801, %ax
	int	$0x15

	shll	$16, %ebx
	movw	%ax, %bx

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movl	$0xFFFFFFFF, %eax
	cmpb	$0x86, %bh
	je	xnoteisa

	movl	%ebx, %eax

xnoteisa:
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%ebp
	ret

/*
 *
 * get_mmap_entry(addr, cont) :  address and old continuation value (zero to
 *		start), for the Query System Address Map BIOS call.
 *
 *  Sets the first 4-byte int value of "addr" to the size returned by
 *  the call.  If the call fails, sets it to zero.
 *
 *	Returns:  new (non-zero) continuation value, 0 if done.
 *
 * NOTE: Currently hard-coded for a maximum buffer length of 1024.
 */

ENTRY(get_mmap_entry)
	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx
	push	%edi
	push	%esi

	/* place address (+4) in ES:DI */
	movl	0x1c(%esp), %eax
	addl	$4, %eax
	movl	%eax, %edi
	andl	$0xf, %edi
	shrl	$4, %eax
	movl	%eax, %esi

	/* set continuation value */
	movl	0x20(%esp), %ebx

	/* set default maximum buffer size */
	movl	$0x14, %ecx

	/* set EDX to 'SMAP' */
	movl	$0x534d4150, %edx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movw	%si, %es
	movl	$0xe820, %eax
	int	$0x15

	DATA32	jc	xnosmap

	cmpl	$0x534d4150, %eax
	DATA32	jne	xnosmap

	cmpl	$0x14, %ecx
	DATA32	jl	xnosmap

	cmpl	$0x400, %ecx
	DATA32	jg	xnosmap

	DATA32	jmp	xsmap

xnosmap:
	movl	$0, %ecx

xsmap:
	DATA32	call	EXT_C(real_to_prot)
	.code32

	/* write length of buffer (zero if error) into "addr" */
	movl	0x1c(%esp), %eax
	movl	%ecx, (%eax)

	/* set return value to continuation */
	movl	%ebx, %eax

	pop	%esi
	pop	%edi
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%ebp
	ret

/*
 * gateA20(int linear)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 */

ENTRY(gateA20)
	pushl	%eax

	call    gloop1

	movb	$KC_CMD_WOUT, %al
	outb	$K_CMD

gloopint1:
	inb	$K_STATUS
	andb	$K_IBUF_FUL, %al
	jnz	gloopint1

	movb	$KB_OUTPUT_MASK, %al
	cmpb	$0, 0x8(%esp)
	jz	gdoit

	orb	$KB_A20_ENABLE, %al
gdoit:
	outb	$K_RDWR

	call	gloop1

	popl	%eax
	ret

gloop1:
	inb	$K_STATUS
	andb	$K_IBUF_FUL, %al
	jnz	gloop1

gloop2:
	inb	$K_STATUS
	andb	$K_OBUF_FUL, %al
	jz	gloop2ret
	inb	$K_RDWR
	jmp	gloop2

gloop2ret:
	ret


ENTRY(patch_code)	/* labels start with "pc_" */
	.code16

	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	ADDR32	movl	$0, 0
pc_stop:
	hlt
	DATA32	jmp	pc_stop
ENTRY(patch_code_end)

	.code32


/*
 * linux_boot()
 *
 * Does some funky things (including on the stack!), then jumps to the
 * entry point of the Linux setup code.
 */

ENTRY(linux_boot)
	/* don't worry about saving anything, we're committed at this point */
	cld	/* forward copying */

	/* copy kernel */
	movl	$LINUX_SETUP, %eax
	movl	LINUX_KERNEL_LEN_OFFSET(%eax), %ecx
	shll	$2, %ecx
	movl	$LINUX_STAGING_AREA, %esi
	movl	$LINUX_KERNEL, %edi

	rep
	movsl

ENTRY(big_linux_boot)
	/* XXX new stack pointer in safe area for calling functions */
	movl	$0x4000, %esp
	call	EXT_C(stop_floppy)

	/* final setup for linux boot */

	movw	$LINUX_SETUP_SEG, %ax
	movw	%ax, segment

	xorl	%eax, %eax
	movl	%eax, offset

	call	EXT_C(prot_to_real)
	.code16

	/* final setup for linux boot */
	movw	$LINUX_SETUP_STACK, %sp

	movw	$LINUX_INIT_SEG, %ax
	movw	%ax, %ss

	/* jump to start */
	DATA32	ADDR32	ljmp	(offset)
	.code32


/*
 * multi_boot(int start, int mbi)
 *
 *  This starts a kernel in the manner expected of the multiboot standard.
 */

ENTRY(multi_boot)
	/* no need to save anything */
	call	EXT_C(stop_floppy)

	movl	$0x2BADB002, %eax
	movl	0x8(%esp), %ebx

	/* boot kernel here (absolute address call) */
	call	*0x4(%esp)

	/* error */
	call	EXT_C(stop)


/*
 * cls()
 * BIOS call "INT 10H Function 0Fh" to get current video mode
 *	Call with	%ah = 0x0f
 *      Returns         %al = (video mode)
 *                      %bh = (page number)
 * BIOS call "INT 10H Function 00h" to set the video mode (clears screen)
 *	Call with	%ah = 0x00
 *                      %al = (video mode)
 */


ENTRY(cls)
	push	%ebp
	push	%eax
	push	%ebx                    /* save EBX */

	call	EXT_C(prot_to_real)
	.code16

	movb	$0xf, %ah
	int	$0x10			/* Get Current Video mode */
        xorb	%ah, %ah
        int	$0x10                   /* Set Video mode (clears screen) */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%ebx
	pop	%eax
	pop	%ebp
	ret

	
/*
 * nocursor()
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */

ENTRY(nocursor)
	push	%ebp
	push	%eax
	push	%ebx                    /* save EBX */
	push	%edx

	call	EXT_C(prot_to_real)
	.code16

	movw    $0x2000, %cx
	movb    $0x1, %ah
	int     $0x10 

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%edx
	pop	%ebx
	pop	%eax
	pop	%ebp
	ret
		

/*
 * getxy()
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(getxy)
	push	%ebp
	push	%ebx                    /* save EBX */
	push	%ecx                    /* save ECX */
	push	%edx

	call	EXT_C(prot_to_real)
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dl, %ah
	movb	%dh, %al

	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%ebp
	ret


/*
 * gotoxy(x,y)
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(gotoxy)
	push	%ebp
	push	%eax
	push	%ebx                    /* save EBX */
	push	%edx

	movb	0x14(%esp), %dl          /* %dl = x */
	movb	0x18(%esp), %dh          /* %dh = y */

	call	EXT_C(prot_to_real)
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x2, %ah
	int	$0x10			/* set cursor position */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%edx
	pop	%ebx
	pop	%eax
	pop	%ebp
	ret


/*
 * set_attrib(attr) :  Sets the character attributes for character at
 *		current cursor position.
 *
 *  Bitfields for character's display attribute:
 *  Bit(s)	Description
 *   7		foreground blink
 *   6-4	background color
 *   3		foreground bright
 *   2-0	foreground color
 *
 *  Values for character color:
 *		Normal		Bright
 *   000b	black		dark gray
 *   001b	blue		light blue
 *   010b	green		light green
 *   011b	cyan		light cyan
 *   100b	red		light red
 *   101b	magenta		light magenta
 *   110b	brown		yellow
 *   111b	light gray	white
 *
 * BIOS call "INT 10H Function 08h" to read character and attribute data
 *	Call with	%ah = 0x08
 *			%bh = page
 *	Returns		%ah = character attribute
 *			%al = character value
 * BIOS call "INT 10H Function 09h" to write character and attribute data
 *	Call with	%ah = 0x09
 *			%al = character value
 *			%bh = page
 *			%bl = character attribute
 *			%cx = count to display (???, possible side-effects!!)
 */

ENTRY(set_attrib)
	push	%ebp
	push	%eax
	push	%ebx
	push	%ecx

	movl	0x14(%esp), %ecx
	xorl	%ebx, %ebx

	call	EXT_C(prot_to_real)
	.code16

	movb	$0x8, %ah
	int	$0x10
	movb	$0x9, %ah
	movb	%cl, %bl
	movw	$1, %cx
	int	$0x10

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%ecx
	pop	%ebx
	pop	%eax
	pop	%ebp
	ret


/*
 * getrtsecs()
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
ENTRY(getrtsecs)
	push	%ebp
	push	%ecx
	push	%edx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movb	$0x2, %ah
	int	$0x1a

	DATA32	jnc	gottime
	movb	$0xff, %dh

gottime:
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dh, %al

	pop	%edx
	pop	%ecx
	pop	%ebp
	ret

	
#ifndef STAGE1_5
/*
 * currticks()
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
ENTRY(currticks)
	pushl	%ebp
	pushl	%ecx
	pushl	%edx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

        xorl	%eax, %eax
        int	$0x1a

	DATA32	call	EXT_C(real_to_prot)
	.code32

        shll	$16, %ecx
        movl	%edx, %eax
        orl	%ecx, %eax

	popl	%edx
	popl	%ecx
	popl	%ebp
	ret
#endif /* ! STAGE1_5 */


/*
 * getkey()
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

ENTRY(getkey)
	push	%ebp
	push	%ebx			/* save %ebx */

	call	EXT_C(prot_to_real)
	.code16

	int	$0x16

	movw	%ax, %bx		/* real_to_prot uses %eax */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movw	%bx, %ax

	pop	%ebx
	pop	%ebp
	ret


/*
 * checkkey()
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
ENTRY(checkkey)
	push	%ebp
	push	%ebx

	xorl	%ebx, %ebx

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movb	$0x1, %ah
	int	$0x16

	DATA32	jz	notpending
	movw	%ax, %bx
	DATA32	jmp	pending

notpending:
	movl	$0xFFFFFFFF, %ebx

pending:
	DATA32	call	EXT_C(real_to_prot)
	.code32

	mov	%ebx, %eax

	pop	%ebx
	pop	%ebp
	ret

#endif /* STAGE1_5 */

/*
 *  This is the area for all of the special variables.
 */

	.p2align	2	/* force 4-byte alignment */

protstack:
	.long	PROTSTACKINIT

VARIABLE(boot_drive)
	.long	0

	/* an address can only be long-jumped to if it is in memory, this
	   is used by multiple routines */
offset:
	.long	0x8000
segment:
	.word	0

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
