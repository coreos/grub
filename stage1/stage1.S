/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
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
 *  defines for the code go here
 */

#define SIGNATURE   0xaa55
#define BPBEND      0x3e
#define PARTSTART   0x1be	/* starting address of partition table */
#define PARTEND     0x1fe	/* ending addres of the partition table */
#define MINPARAMSIZ 13		/* size of extra data parameters */
#define LISTSIZ     8		/* size of sector list */
#define REALSTACK   0x2000	/* stack for this code and BIOS calls */
#define BUFFERSEG   0x7000	/* segment address of disk buffer, the
				   disk buffer MUST be 32K long and cannot
				   straddle a 64K boundary */
#define BIOS_HD_FLAG 0x80	/* bit set in BIOS drive number to designate
				   a hard disk vs. a floppy */

	/* Absolute addresses
	   This makes the assembler generate the address without support
	   from the linker. (ELF can't relocate 16-bit addresses!) */
#define ABS(x) (x-_start+0x7c00)

	/* Print message string */
#define MSG(x)	movw $x, %si; call message

	.file	"stage1.S"

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
sectors:
	.word	0
heads:
	.word	0
cylinders:
	.word	0
	/* more space... */

	. = _start + BPBEND

	/*
	 * End of BIOS parameter block.
	 */

after_BPB:

/* general setup */
	cli		/* we're not safe here! */

	/* set up %ds and %ss as offset from 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss

	/* set up the REAL stack */
	movw	$REALSTACK, %sp

	sti		/* we're safe again */

	/*
	 *  Check if we have a forced disk reference here
	 */
	movb	firstlist, %al
	cmpb	$0xff, %al
	je	1f
	movb	%al, %dl
1:
	/* save drive reference first thing! */
	pushw	%dx

	/*
	 *  Jump to floppy probe instead of the hard disk probe ?
	 */
	movb	%dl, %al
	andb	$BIOS_HD_FLAG, %al
	jz	floppy_probe

	/*
	 *  Determine the hard disk geometry from the BIOS!
	 */
	movb	$8, %ah
	int	$0x13

	/* if BIOS geometry call fails, display error and die! */
	jc	hd_probe_error

final_init:
	/* save number of heads */
	xorb	%ah, %ah
	movb	%dh, %al
	incw	%ax
	movw	%ax, heads

	xorw	%dx, %dx
	movb	%cl, %dl
	shlw	$2, %dx
	movb	%ch, %al
	movb	%dh, %ah

	/* save number of cylinders */
	incw	%ax
	movw	%ax, cylinders

	xorw	%ax, %ax
	movb	%dl, %al
	shrb	$2, %al

	/* save number of sectors */
	movw	$ABS(sectors), %si
	movw	%ax, (%si)

	/* this sets up for the first run through "bootloop" */
	movw	$ABS(firstlist), %di


        /* this is the loop for reading the secondary boot-loader in */
bootloop:

	/* update position to load from */
	subw	$LISTSIZ, %di

	/* check the number of sectors to read */
	cmpw	$0, 4(%di)

	/* if zero, go to the start function */
	je	bootit

	/* load logical sector start (bottom half) */
	movw	(%di), %ax

	/* load logical sector start (top half) */
	movw	2(%di), %dx

	/* divide by number of sectors */
	divw	(%si)

	/* save sector start */
	movb	%dl, (%di)

	xorw	%dx, %dx	/* zero %dx */
	divw	2(%si)		/* divide by number of heads */

	/* save head start */
	movb	%dl, 1(%di)

	/* save cylinder start */
	movw	%ax, 2(%di)

	/* do we need too many cylinders? */
	cmpw	4(%si), %ax
	jge	geometry_error

setup_sectors:

	/* determine the maximum sector length of this read */
	movw	(%si), %ax	/* get number of sectors per track/head */

	/* subtract sector start */
	subb	(%di), %al

	/* how many do we really want to read? */
	cmpw	%ax, 4(%di)	/* compare against total number of sectors */


	/* which is greater? */
	jg	more_sectors

	/* if less than, set to total */
	movw	4(%di), %ax

more_sectors:
	/* subtract from total */
	subw	%ax, 4(%di)

/*
 *  This is the loop for taking care of BIOS geometry translation (ugh!)
 */

	/* get high bits of cylinder */
	movb	3(%di), %dl

	shlb	$6, %dl		/* shift left by 6 bits */
	movb	(%di), %cl	/* get sector */

	incb	%cl		/* normalize sector (sectors go
					from 1-N, not 0-(N-1) ) */
	orb	%dl, %cl	/* composite together */
	movb	2(%di), %ch	/* sector+hcyl in cl, cylinder in ch */

	/* restore %dx */
	popw	%dx
	pushw	%dx

	/* head number */
	movb	1(%di), %dh

	pushw	%ax	/* save %ax from destruction! */

/*
 * BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
 *	Call with	%ah = 0x2
 *			%al = number of sectors
 *			%ch = cylinder
 *			%cl = sector (bits 6-7 are high bits of "cylinder")
 *			%dh = head
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *			%es:%bx = segment:offset of buffer
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */

	movw	$BUFFERSEG, %bx
	movw	%bx, %es	/* load %es segment with disk buffer */

	xorw	%bx, %bx	/* %bx = 0, put it at 0 in the segment */
	movb	$0x2, %ah	/* function 2 */
	int	$0x13

	jc	read_error

	/* save source segment */
	movw	%es, %dx

	/* load addresses for copy from disk buffer to destination */
	movw	6(%di), %es	/* load destination segment */

	/* restore %ax */
	popw	%ax

	/* determine the next possible destination address (presuming
		512 byte sectors!) */
	shlw	$5, %ax		/* shift %ax five bits to the left */
	addw	%ax, 6(%di)	/* add the corrected value to the destination
				   address for next time */

	/* get the copy length */
	shlw	$4, %ax
	movw	%ax, %cx

	/* save addressing regs */
	pushw	%ds
	pushw	%si
	pushw	%di

	xorw	%di, %di	/* zero offset of destination addresses */
	xorw	%si, %si	/* zero offset of source addresses */
	movw	%dx, %ds	/* restore the source segment */

	cld		/* sets the copy direction to forward */

	/* perform copy */
	rep		/* sets a repeat */
	movsb		/* this runs the actual copy */

	/* restore addressing regs */
	popw	%di
	popw	%si
	popw	%ds

	/* check if finished with this dataset */
	cmpw	$0, 4(%di)
	je	bootloop

	/* find out the next BIOS set to load in */
	movb	$0, (%di)	/* set the sector start */

	xorb	%ah, %ah	/* zero %ah */
	movb	1(%di), %al	/* load head number into %al */

	incw	%ax		/* increment current head number */
	cmpw	2(%si), %ax	/* compare to total number of heads */
	jne	update_heads

	movw	2(%di), %ax	/* load cylinder number into %ax */

	incw	%ax		/* increment current cylinder number */
	cmpw	4(%si), %ax	/* compare to total number of cylinders */

	je	geometry_error	/* display error and die if greater */

	movw	%ax, 2(%di)	/* store new cylinder number */

	movb	$0, %al		/* for storing new head number */

update_heads:
	movb	%al, 1(%di)	/* store new head number */

	/* jump to "setup_sectors" to determine length of the new read */
	jmp	setup_sectors

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
hd_probe_error:
	MSG(hd_probe_error_string)
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
hd_probe_error_string:	.string "Hard Disk"
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
	. = _start + PARTSTART - MINPARAMSIZ - LISTSIZ

	/* this next data area before the partition area is specifically
	   sized, you should update "MINPARAMSIZ" to reflect any additions
	   or deletions to this area */

	.word 0
	.word 0

        /* fill the first data listing with the default */
#ifdef  FFS_STAGE1_5
	.long 2		/* this is the sector start parameter, in logical
			   sectors from the start of the disk, sector 0 */
	.word 14	/* this is the number of sectors to read */
	.word 0x0200	/* this is the segment of the starting address
			   to load the data into */
#else
	.long 1		/* this is the sector start parameter, in logical
			   sectors from the start of the disk, sector 0 */
	.word 80	/* this is the number of sectors to read */
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
	 *  See ../stage2/shared.h for COMPAT_VERSION_* definitions used
	 *  in stage2 and stage1_5 modules.
	 *
	 *  DO NOT MOVE THIS!!!
	 */
	.byte	2, 0

	/*
	 *  This is where an MBR would go if on a hard disk.  The code
	 *  here isn't even referenced unless we're on a floppy.  Kinda
	 *  sneaky, huh?
	 */

	. = _start + PARTSTART

probe_values:
	.byte	36, 18, 15, 9, 0

floppy_probe:
/*
 *  Perform floppy probe.
 */

	movw	$ABS(probe_values-1), %si

probe_loop:
	/* reset floppy controller INT 13h AH=0 */
	xorw	%ax, %ax
	int	$0x13

	incw	%si
	movb	(%si), %cl

	/* if number of sectors is 0, display error and die */
	cmpb	$0, %cl
	jne	1f

/*
 * Floppy disk probe failure.
 */
	movw	$fd_probe_error_string, %si
	call	message
	jmp	general_error

fd_probe_error_string:	.string "Floppy"

1:
	/* perform read */
	movw	$REALSTACK, %bx
	movw	$0x201, %ax
	movb	$0, %ch
	movb	$0, %dh
	int	$0x13

	/* if error, jump to "probe_loop" */
	jc	probe_loop

	/* %cl is already the correct value! */
	movb	$1, %dh
	movb	$79, %ch

	jmp	final_init

	. = _start + PARTEND

/* the last 2 bytes in the sector 0 contain the signature */
	.word	SIGNATURE
