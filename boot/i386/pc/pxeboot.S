/* -*-Asm-*- */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004   Free Software Foundation, Inc.
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

#include <grub/boot.h>
#include <grub/machine/boot.h>

/*
*  defines for the code go here
*/

       .file   "pxeboot.S"

       .text

       /* Tell GAS to generate 16-bit instructions so that this code works
          in real mode. */
       .code16

.globl _start; _start:
       /*
        * _start is loaded at 0x7c00 and is jumped to with CS:IP 0:0x7c00
        */
       ljmp    $0, $0x8200

       . = 0x8000 - 0x7C00 -1
       .byte 0
