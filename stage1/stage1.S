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
#define MINPARAMSIZ 18		/* size of extra data parameters */
#define LISTSIZ     8		/* size of sector list */
#define REALSTACK   0x2000	/* stack for this code and BIOS calls */
#define BUFFERSEG   0x7000	/* segment address of disk buffer, the
				   disk buffer MUST be 32K long and cannot
				   straddle a 64K boundary */
#define BIOS_HD_FLAG 0x80	/* bit set in BIOS drive number to designate
				   a hard disk vs. a floppy */

	/* data for debugging */
#define	CHAR_1 '1'
#define	CHAR_2 '2'
#define	CHAR_3 '3'
#define	CHAR_4 '4'
#define	CHAR_5 '5'
#define	CHAR_6 '6'
#define CHAR_a 'a'

	/* messages displayed at all times */
#define	CHAR_S 'S'
#define	CHAR_J 'J'

  /* error messages */

	/* Read error (trying to read non-existent sector, perhaps) */
#define CHAR_R 'R'
	/* Geometry translation error (off of probed geometry) */
#define CHAR_G 'G'
	/* Disk geometry probe failed! */
#define CHAR_P 'P'

	/* Absolute addresses
	   This makes the assembler generate the address without support
	   from the linker. (ELF can't relocate 16bit addresses!) */
#define ABS(x) (x-_start+0x7c00)
	
	/* Relative addresses (for jumps)
	   These macros use the local label 0, so start with your local
	   label at 1! */
#define REL(x)  .word x-0f; 0:
#define RELB(x) .byte x-0f; 0:
	
	/* Print message
	   movb $x, %al; call message */
#define MSG(x)	movb $x, %al; .byte 0xe8; REL(message)

	.file	"stage1.S"

	.text

	/*
	 *  The ".code16" directive only works in GAS, the GNU assembler!
	 *  This adds 32-bit data or addressing directives so that this
	 *  code will work in real mode!
	 */
	.code16

.globl _start; _start:
	/*
	 * _start is loaded at 0x7c00 and is jumped to with CS:IP 0:0x7c00
	 */

	/*
	 * Beginning of the sector is compatible with the FAT/HPFS BIOS
	 * parameter block.
	 */

	/* jmp after_BPB */
	.byte 0xeb; RELB(after_BPB)
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

	/* set up %ds and %ss as offset from 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	cld		/* sets the copy direction to forward */

	/* set up the REAL stack */
	movw	%ax, %ss
	movw	$REALSTACK, %sp

	/*
	 *  Check if we have a forced disk reference here
	 */
	/* movb	firstlist, %al */
	.byte	0xa0; .word ABS(firstlist)
	cmpb	$0xff, %al
	/* je	1f */
	.byte	0x74; RELB(1f)
	movb	%al, %dl
1:	
	/* save drive reference first thing! */
	pushw	%dx

	/* tell the user that we're here before anything else! */
	MSG(CHAR_S)

#ifndef BIOS_PROBE_ONLY
	/*
	 *  Jump to floppy probe instead of the hard disk probe ?
	 */
	movb	%dl, %al
	andb	$BIOS_HD_FLAG, %al

	/* jz	floppy_probe */
	.byte	0x0f, 0x84; REL(floppy_probe)
#endif

#ifdef DEBUG
	MSG(CHAR_6)
#endif

	/*
	 *  Determine the hard disk geometry from the BIOS!
	 */
	movb	$8, %ah
	int	$0x13

	/* if BIOS geometry call fails, display error and die! */
	/* jc	probe_error   (16-bit)*/
	.byte	0x0f, 0x82; REL(probe_error)

final_init:
	xorb	%ah, %ah
	movb	%dh, %al
	incw	%ax

	/* movw	%ax, heads */	/* save num heads */
	.byte	0xA3; .word ABS(heads)

	xorw	%dx, %dx
	movb	%cl, %dl
	shlw	$2, %dx
	movb	%ch, %al
	movb	%dh, %ah

	incw	%ax

	/* movw	%ax, cylinders */	/* save num cylinders */
	.byte	0xA3; .word ABS(cylinders)

	xorw	%ax, %ax
	movb	%dl, %al
	shrb	$2, %al

	/* save a byte on addressing by moving this forward ?? */
	movw	$ABS(sectors), %si

	/* movw	%ax, (%si) */	/* save num sectors */
	.byte	0x89, 0x04

	/* this sets up for the first run through "bootloop" */
	movw	$ABS(firstlist), %di


        /* this is the loop for reading the secondary boot-loader in */
bootloop:

#ifdef DEBUG
	MSG(CHAR_2)
#endif

	/* update position to load from */
	subw	$LISTSIZ, %di

	/* cmpw	$0, 4(%di) */   /* check the number of sectors to read */
	.byte	0x81, 0x7d, 0x04; .word	0

	/* je	bootit	  (16-bit)*/	/* if zero, go to the start function */
	.byte	0x0f, 0x84; REL(bootit)

	/* movl	(%di), %ax */	/* load logical sector start (bottom half) */
	.byte	0x8b, 0x05

	/* movl	2(%di), %dx */	/* load logical sector start (top half) */
	.byte	0x8b, 0x55, 0x02

	/* divw	(%si), %dx:%ax */	/* divide by number of sectors */
	.byte	0xf7, 0x34

	/* movb	%dl, (%di) */	/* save sector start */
	.byte	0x88, 0x15

	xorw	%dx, %dx	/* zero %dx */
	/* divw	2(%si), %dx:%ax */	/* divide by number of heads */
	.byte	0xf7, 0x74, 0x02

	/* movb	%dl, 1(%di) */	/* save head start */
	.byte	0x88, 0x55, 0x01

	/* movw	%ax, 2(%di) */	/* save cylinder start */
	.byte	0x89, 0x45, 0x02

	/* cmpw	4(%si), %ax */	/* do we need too many cylinders? */
	.byte	0x3b, 0x44, 0x04

	/* jge	geometry_error (16-bit) */
	.byte	0x0f, 0x8d; REL(geometry_error)

setup_sectors:

#ifdef DEBUG
	MSG(CHAR_3)
#endif

	/* determine the maximum sector length of this read */
	/* movw	(%si), %ax */	/* get number of sectors per track/head */
	.byte	0x8b, 0x04

	/* subb	(%di), %al */	/* subtract sector start */
	.byte	0x2a, 0x05

	/* how many do we really want to read? */
	/* cmpw	%ax, 4(%di) */	/* compare against total number of sectors */
	.byte	0x39, 0x45, 0x04

	/* jg	more_sectors  (8-bit)*/	/* which is greater? */
	.byte	0x7f; RELB(more_sectors)

	/* movw	4(%di), %ax */	/* if less than, set to total */
	.byte	0x8b, 0x45, 0x04

#ifdef DEBUG
	pushw	%ax
	MSG(CHAR_4)
	popw	%ax
#endif

more_sectors:
	/* subw	%ax, 4(%di) */	/* subtract from total */
	.byte	0x29, 0x45, 0x04

	/* this is the loop for taking care of BIOS translation (ugh!) */

#ifdef DEBUG
	pushw	%ax
	addb	$CHAR_a, %al
	/* call message */
	.byte	0xe8; REL(message)
	popw	%ax
#endif

	/* movb	3(%di), %dl */	/* get high bits of cylinder */
	.byte	0x8a, 0x55, 0x03

	shlb	$6, %dl		/* shift left by 6 bits */
	/* movw	(%di), %cl */	/* get sector */
	.byte	0x8b, 0x0d

	incb	%cl		/* normalize sector (sectors go
					from 1-N, not 0-(N-1) ) */
	orb	%dl, %cl	/* composite together */
	/* movb	2(%di), %ch */	/* sector+hcyl in cl, cylinder in ch */
	.byte	0x8a, 0x6d, 0x02

	/* restore %dx */
	popw	%dx
	pushw	%dx

	/* movb	1(%di), %dh */	/* head num */
	.byte	0x8a, 0x75, 0x01

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

	/* jc	read_error */
	.byte	0x72
	RELB(read_error)

	/* load addresses for copy from disk buffer to destination */
	/* movw	6(%di), %es */	/* load destination segment */
	.byte	0x8e, 0x45, 0x06

	movw	$BUFFERSEG, %ax
	movw	%ax, %fs	/* load source segment */

	/* restore %ax */
	popw	%ax

	/* determine the next possible destination address (presuming
		512 byte sectors!) */
	shlw	$5, %ax		/* shift %ax five bits to the left */
	/* addw	%ax, 6(%di) */	/* add the corrected value to the destination
				   address for next time */
	.byte	0x01, 0x45, 0x06

	/* get the copy length */
	shlw	$4, %ax
	movw	%ax, %cx

	/* save addressing regs */
	pushw	%si
	pushw	%di

	xorw	%di, %di	/* zero offset of destination addresses */
	xorw	%si, %si	/* zero offset of source addresses */

	/* perform copy */
	rep		/* sets a repeat */
	fs		/* this overrides the source segment from %ds to %fs */
	movsb		/* this runs the actual copy */

	/* restore addressing regs */
	popw	%di
	popw	%si

	/* check if finished with this dataset */
	/* cmpw	$0, 4(%di) */
	.byte	0x81, 0x7d, 0x04; .word	0

	/* je	bootloop */
	.byte	0x0f, 0x84; REL(bootloop)

#ifdef DEBUG
	MSG(CHAR_5)
#endif

	/* find out the next BIOS set to load in */
	/* movb	$0, (%di) */	/* set the sector start */
	.byte	0xc6, 0x05, 0

	xorb	%ah, %ah	/* zero %ah */
	/* movb	1(%di), %al */	/* load head number into %al */
	.byte	0x8a, 0x45, 0x01

	incw	%ax		/* increment current head number */
	/* cmpw	2(%si), %ax */	/* compare to total number of heads */
	.byte	0x3b, 0x44, 0x02

	/* jne	update_heads   (8-bit)*/
	.byte	0x75; RELB(update_heads)

	/* movw	2(%di), %ax */	/* load cylinder number into %ax */
	.byte	0x8b, 0x45, 0x02

	incw	%ax		/* increment current cylinder number */
	/* cmpw	4(%si), %ax */	/* compare to total number of cylinders */
	.byte	0x3b, 0x44, 0x04

	/* je	geometry_error */	/* display error and die if greater */
	.byte	0x74; RELB(geometry_error)

	/* movw	%ax, 2(%di) */	/* store new cylinder number */
	.byte	0x89, 0x45, 0x02

	movb	$0, %al		/* for storing new head number */

update_heads:
	/* movb	%al, 1(%di) */	/* store new head number */
	.byte	0x88, 0x45, 0x01

#ifdef DEBUG
	addb	$CHAR_a, %al
	/* call message */
	.byte	0xe8; REL(message)

	/* movb	2(%di), %al */
	.byte	0x8a, 0x45, 0x02

	addb	$CHAR_a, %al
	/* call message */
	.byte	0xe8; REL(message)
#endif

	/* jump to "setup_sectors" to determine length of the new read */
	/* jmp	setup_sectors */
	.byte	0xe9; REL(setup_sectors)

/*
 * BIOS Geometry translation error (past the end of the disk geometry!).
 */
geometry_error:
	movb	$CHAR_G, %al
	/* jmp	display_error  (8-bit) */
	.byte	0xeb; RELB(display_error)

/*
 * Disk probe failure.
 */
probe_error:
	movb	$CHAR_P, %al
	/* jmp	display_error  (8-bit) */
	.byte	0xeb; RELB(display_error)

/*
 * Read error on the disk.
 */
read_error:
	movb	$CHAR_R, %al

display_error:
	/* call message */
	.byte	0xe8; REL(message)

/* go here when you need to stop the machine hard after an error condition */
stop:	/* jmp	stop  (8-bit) */
	.byte	0xeb; RELB(stop)


/*
 * message: write the character in %al to the console, %ah gets trashed.
 */
message:
	/*
	 * Use BIOS "int 10H Function 0Eh" to write character in teletype mode
	 *	%ah = 0xe	%al = character
	 *	%bh = page	%bl = foreground color (graphics modes)
	 */

	pushw	%bx
	movw	$0x0001, %bx
	movb	$0xe, %ah
	int	$0x10		/* display a byte */
	popw	%bx

	/* ret */
	.byte	0xc3
lastlist:

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
	.word 64	/* this is the number of sectors to read */
	.word 0x0800	/* this is the segment of the starting address
			   to load the data into */
#endif
firstlist:
	.byte 0xff	/* the disk to load stage2 from */
			/* 0xff means use the boot drive */
/* the above label has to be after its data!!! */

	/*
	 *  Jump here when all data loading is done.
	 */

bootit:

	/*
	 *  Tell the user we're going to the next stage...
	 */
	MSG(CHAR_J)

	popw	%dx	/* this makes sure %dl is our "boot" drive */

	/*
	 * ljmp to the second stage boot loader.
	 */

	/* ljmp	$myoffset, $myseg */
	.byte	0xea
#ifdef  STAGE1_5
	.word	0x2000, 0
#else
	.word	0x8000, 0
#endif

	/*
	 *  This is the compatibility version number.
	 *
	 *  DO NOT MOVE THIS!!!
	 */
	.byte	1, 0

	/*
	 *  This is where an MBR would go if on a hard disk.  The code
	 *  here isn't even referenced unless we're on a floppy.  Kinda
	 *  sneaky, huh?
	 */

	. = _start + PARTSTART

#ifndef BIOS_PROBE_ONLY
probe_values:
	.byte	36, 18, 15, 9, 0

floppy_probe:
/*
 *  Perform floppy probe!
 */

	movw	$ABS(probe_values-1), %si

probe_loop:
	/* reset floppy controller INT 13h AH=0 */
	xorw	%ax, %ax
	int	$0x13

	incw	%si

	/* movb	  (%si), %cl */
	.byte	0x8a, 0x0c

	/* if number of sectors is 0, display error and die */
	cmpb	$0, %cl

	/* je	probe_error   (16-bit)*/
	.byte	0x0f, 0x84; REL(probe_error)

	/* perform read */
	movw	$REALSTACK, %bx
	movw	$0x201, %ax
	movb	$0, %ch
	movb	$0, %dh
	int	$0x13

	/* if error, jump to "probe_loop" */
	/* jc probe_loop	(8-bit)*/
	.byte	0x72; RELB(probe_loop)

	/* %cl is already the correct value! */
	movb	$1, %dh
	movb	$79, %ch

	/* jmp	final_init */
	.byte	0xe9; REL(final_init)

#endif /* !BIOS_PROBE_ONLY */

	. = _start + PARTEND

/* the last 2 bytes in the sector 0 contain the signature */
	.word	SIGNATURE

