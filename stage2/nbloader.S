/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000   Free Software Foundation, Inc.
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

#include <nbi.h>
#include <diskless_size.h>
			
	.file	"nbloader.S"
	.text

	/* Just a dummy entry */
.globl _start; _start:

	/*
	 * netboot image header
	 */

	.long	NBI_MAGIC
	.long	0x00000004
	/* load address of the first block  */
	.word	NBI_DEST_ADDR
	.word	0x0000
	/* start addr of the code (stage 2) */
	.word	NBI_DEST_ADDR + 0x0200
	.word	0x0000

	.long	0x04000004
	.long	NBI_DEST_ADDR + 0x0200
	.long	DISKLESS_SIZE
	.long	DISKLESS_SIZE

	/* This ensures that the length of this image will be 1 sector */
	. = _start + 0x200 - 1
	.byte	0
