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

#include <stage1.h>
#include <stage2_size.h>
		
/*
 *  defines for the code go here
 */

	/* Absolute addresses
	   This makes the assembler generate the address without support
	   from the linker. (ELF can't relocate 16-bit addresses!) */
#define ABS(x) (x-_start+0x7c00)

	/* Print message string */
#define MSG(x)	movw $ABS(x), %si; call message

	.file	"stage1_lba.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

.globl _start; _start:
	/*
	 * _start is loaded at 0x7c00 and is jumped to with CS:IP 0:0x7c00
	 */

	/*
	 * Beginning of the sector is compatible with the FAT/HPFS BIOS
	 * parameter block.
	 */

	jmp after_BPB
	nop	/* do I care about this ??? */

	/*
	 * This space is for the BIOS parameter block!!!!  Don't change
	 * the first jump, nor start the code anywhere but right after
	 * this area.
	 */

	. = _start + 4

	/* scratch space */
disk_address_packet:
	/* Actually, must set these values at the runtime.  */
	.byte	0x10
	.byte	0
	.word	0
	.long	0
	.quad	0
	/* more space... */

	. = _start + STAGE1_BPBEND

	/*
	 * End of BIOS parameter block.
	 */

stage1_id:
	.byte	STAGE1_ID_LBA

after_BPB:

/* general setup */
	cli		/* we're not safe here! */

	/* set up %ds and %ss as offset from 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss

	/* set up the REAL stack */
	movw	$STAGE1_STACKSEG, %sp

	sti		/* we're safe again */

	/*
	 *  Check if we have a forced disk reference here
	 */
	movb	ABS(firstlist), %al
	cmpb	$0xff, %al
	je	1f
	movb	%al, %dl
1:
	/* save drive reference first thing! */
	pushw	%dx

	/* check if LBA is supported */
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13

	jc	lba_probe_error
	cmpw	$0xaa55, %bx
	jne	lba_probe_error

	/* get the geometry (limited to 2TB!) */
	movb	$0x48, %ah
	movw	$STAGE1_DRP_ADDR, %si
	movw	$STAGE1_DRP_SIZE, (%si)
	int	$0x13

	jc	lba_probe_error

	/* save the total number of sectors */
	movl	0x10(%si), %ecx

	/* set %si to disk address packet */
	movw	$ABS(disk_address_packet), %si

	/* this sets up for the first run through "bootloop" */
	movw	$ABS(firstlist - STAGE1_LISTSIZE), %di

        /* this is the loop for reading the secondary boot-loader in */
bootloop:

	/* check the number of sectors to read */
	cmpw	$0, 4(%di)

	/* if zero, go to the start function */
	je	bootit

setup_sectors:
	/* load logical sector start */
	movl	(%di), %ebx

	/* check for the geometry */
	cmpl	%ecx, %ebx
	jge	geometry_error

	/* the maximum is limited to 0x7f because of Phoenix EDD */
	xorl	%eax, %eax
	movb	$0x7f, %al

	/* how many do we really want to read? */
	cmpw	%ax, 4(%di)	/* compare against total number of sectors */

	/* which is greater? */
	jg	more_sectors

	/* if less than, set to total */
	movw	4(%di), %ax

more_sectors:
	/* subtract from total */
	subw	%ax, 4(%di)

	/* add into logical sector start */
	addl	%eax, (%di)

	/* set up disk address packet */

	/* the size and the reserved byte */
	movw	$0x0010, (%si)

	/* the number of sectors */
	movw	%ax, 2(%si)

	/* the absolute address (low 32 bits) */
	movl	%ebx, 8(%si)

	/* the segment of buffer address */
	movw	$STAGE1_BUFFERSEG, 6(%si)

	pushw	%ax	/* save %ax from destruction! */

	/* zero %eax */
	xorl	%eax, %eax

	/* the offset of buffer address */
	movw	%ax, 4(%si)

	/* the absolute address (high 32 bits) */
	movl	%eax, 12(%si)


/*
 * BIOS call "INT 0x13 Function 0x42" to read sectors from disk into memory
 *	Call with	%ah = 0x42
 *			%dl = drive number
 *			%ds:%si = segment:offset of disk address packet
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */

	movb	$0x42, %ah
	int	$0x13

	jc	read_error

	/* load addresses for copy from disk buffer to destination */
	movw	6(%di), %es	/* load destination segment */

	/* restore %ax */
	popw	%ax

	/* determine the next possible destination address (presuming
		512 byte sectors!) */
	shlw	$5, %ax		/* shift %ax five bits to the left */
	addw	%ax, 6(%di)	/* add the corrected value to the destination
				   address for next time */

	/* save addressing regs */
	pushw	%ds
	pushw	%si
	pushw	%di
	pushw	%cx

	/* get the copy length */
	shlw	$4, %ax
	movw	%ax, %cx

	xorw	%di, %di	/* zero offset of destination addresses */
	xorw	%si, %si	/* zero offset of source addresses */
	movw	$STAGE1_BUFFERSEG, %ax
	movw	%ax, %ds

	cld		/* sets the copy direction to forward */

	/* perform copy */
	rep		/* sets a repeat */
	movsb		/* this runs the actual copy */

	/* restore addressing regs */
	popw	%cx
	popw	%di
	popw	%si
	popw	%ds

	/* check if finished with this dataset */
	cmpw	$0, 4(%di)
	jne	setup_sectors

	/* update position to load from */
	subw	$STAGE1_LISTSIZE, %di

	/* jump to bootloop */
	jmp	bootloop

/* END OF MAIN LOOP */

/*
 * BIOS Geometry translation error (past the end of the disk geometry!).
 */
geometry_error:
	MSG(geometry_error_string)
	jmp	general_error

/*
 * Disk probe failure.
 */
lba_probe_error:
	MSG(lba_probe_error_string)
	jmp	general_error

/*
 * Read error on the disk.
 */
read_error:
	MSG(read_error_string)

general_error:
	MSG(general_error_string)

/* go here when you need to stop the machine hard after an error condition */
stop:	jmp	stop

geometry_error_string:	.string "Geom"
lba_probe_error_string:	.string "LBA"
read_error_string:	.string "Read"
general_error_string:	.string " Error"

/*
 * message: write the string pointed to by %si
 *
 *   WARNING: trashes %si, %ax, and %bx
 */

	/*
	 * Use BIOS "int 10H Function 0Eh" to write character in teletype mode
	 *	%ah = 0xe	%al = character
	 *	%bh = page	%bl = foreground color (graphics modes)
	 */
1:
	movw	$0x0001, %bx
	movb	$0xe, %ah
	int	$0x10		/* display a byte */

	incw	%si
message:
	movb	(%si), %al
	cmpb	$0, %al
	jne	1b	/* if not end of string, jmp to display */
	ret
lastlist:

/*
 *  This area is an empty space between the main body of code below which
 *  grows up (fixed after compilation, but between releases it may change
 *  in size easily), and the lists of sectors to read, which grows down
 *  from a fixed top location.
 */

        /*
         *  This data area is for keeping general parameters.
         */

	. = _start + STAGE1_PARTSTART - STAGE1_MINPARAMSIZE - STAGE1_LISTSIZE

	/* this next data area before the partition area is specifically
	   sized, you should update "STAGE1_MINPARAMSIZE" to reflect any
	   additions or deletions to this area.  */

	.word 0
	.word 0

        /* fill the first data listing with the default */
#ifdef  FFS_STAGE1_5
blocklist_default_start:
	.long 2		/* this is the sector start parameter, in logical
			   sectors from the start of the disk, sector 0 */
blocklist_default_len:
	.word 14	/* this is the number of sectors to read */
blocklist_default_seg:
	.word 0x0200	/* this is the segment of the starting address
			   to load the data into */
#else
blocklist_default_start:
	.long 1		/* this is the sector start parameter, in logical
			   sectors from the start of the disk, sector 0 */
blocklist_default_len:
			/* this is the number of sectors to read */
	.word (STAGE2_SIZE + 511) / 512
blocklist_default_seg:
	.word 0x0800	/* this is the segment of the starting address
			   to load the data into */
#endif
firstlist:	/* this label has to be after the list data!!! */

	.byte 0xff	/* the disk to load stage2 from */
			/* 0xff means use the boot drive */

	/*
	 *  Jump here when all data loading is done.  This
	 *  goes to the second stage bootloader.
	 */

bootit:
	popw	%dx	/* this makes sure %dl is our "boot" drive */
	ljmp	$0, $0x8000	/* FIXME: make 0x2000 for stage1_5 */

	/*
	 *  This is the compatibility version number.
	 *  See stage1.h for COMPAT_VERSION_* definitions used
	 *  in stage2 and stage1_5 modules.
	 *
	 *  DO NOT MOVE THIS!!!
	 */
	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR

	/*
	 *  This is where an MBR would go if on a hard disk.  The code
	 *  here isn't even referenced unless we're on a floppy.  Kinda
	 *  sneaky, huh?
	 */

	. = _start + STAGE1_PARTSTART

	/* These values are not useful in stage1_lba at all, but these
	   prevent the installation command `install' from failing.  */
probe_values:
	.byte	36, 18, 15, 9, 0

	/* This space cannot be used by any bootloader, uggh...  */

	. = _start + STAGE1_PARTEND

/* the last 2 bytes in the sector 0 contain the signature */
	.word	STAGE1_SIGNATURE
