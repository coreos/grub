/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <multiboot.h>
#include <multiboot2.h>

	.p2align	2	/* force 4-byte alignment */

/*
 * This starts the multiboot 2 kernel.
 */

FUNCTION(grub_multiboot2_real_boot)
        /* Push the entry address on the stack.  */
        pushl   %eax
        /* Move the address of the multiboot information structure to ebx.  */
        movl    %edx,%ebx

        /* Interrupts should be disabled.  */
        cli

        /* Move the magic value into eax and jump to the kernel.  */
        movl    $MULTIBOOT2_BOOTLOADER_MAGIC,%eax
        popl    %ecx

        cld
        jmp     *%ecx
