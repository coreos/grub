/* startup.S - Startup code for the MIPS.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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
#include <grub/cpu/kernel.h>
#include <grub/machine/memory.h>

.extern __bss_start
.extern _end
	
	.globl __start, _start, start
__start:
_start:
start:	
	b codestart
	. = _start + GRUB_KERNEL_CPU_PREFIX

VARIABLE(grub_prefix)
	/* to be filled by grub-mkelfimage */

	/*
	 *  Leave some breathing room for the prefix.
	 */

	. = _start + GRUB_KERNEL_CPU_DATA_END
codestart:
	lui $t1, %hi(__bss_start)
	addiu $t1, %lo(__bss_start)
	lui $t2, %hi(_end)
	addiu $t2, %lo(_end)

bsscont:
	sb $0,0($t1)
	addiu $t1,$t1,1
	sltu $t3,$t1,$t2
	bne $3, $0, bsscont

	li $sp, GRUB_MACHINE_MEMORY_STACK_HIGH
	b grub_main