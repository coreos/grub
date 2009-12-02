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

#define BASE_ADDR 8
	
.extern __bss_start
.extern _end
	
	.globl __start, _start, start
__start:
_start:
start:	
	bal codestart
	. = _start + GRUB_KERNEL_CPU_COMPRESSED_SIZE
compressed_size:
	.long 0
	. = _start + GRUB_KERNEL_CPU_TOTAL_MODULE_SIZE
total_module_size:
	.long 0
	. = _start + GRUB_KERNEL_CPU_KERNEL_IMAGE_SIZE
kernel_image_size:	
	.long	0
codestart:
	/* Decompress the payload. */
	move $t9, $ra
	addiu $t2, $t9, GRUB_KERNEL_CPU_RAW_SIZE - BASE_ADDR
	lui $t1, %hi(compressed)
	addiu $t1, %lo(compressed)
	lw $t3, (GRUB_KERNEL_CPU_COMPRESSED_SIZE - BASE_ADDR)($t9)

	/* $t2 contains source compressed address, $t1 is destination,
	   $t3 is compressed size. FIXME: put LZMA here. Don't clober $t9
	*/
reloccont:
	lb $t4, 0($t2)
	sb $t4, 0($t1)
	addiu $t1,$t1,1
	addiu $t2,$t2,1
	addiu $t3, 0xffff
	bne $t3, $0, reloccont

	/* Move the modules out of BSS.  */
	lui $t1, %hi(_start)
	addiu $t1, %lo(_start)
	lw $t2, (GRUB_KERNEL_CPU_KERNEL_IMAGE_SIZE - BASE_ADDR)($t9)
	addu $t2, $t1, $t2
	
	lui $t1, %hi(_end)
	addiu $t1, %lo(_end)
	addiu $t1, (GRUB_MOD_ALIGN-1)
	li $t3, (GRUB_MOD_ALIGN-1)
	nor $t3, $t3, $0
	and $t1, $t1, $t3
	
	lw $t3, (GRUB_KERNEL_CPU_TOTAL_MODULE_SIZE - BASE_ADDR)($t9)

	/* Backward copy.  */
	add $t1, $t1, $t3
	add $t2, $t2, $t3
	addiu $t1, $t1, 0xffff
	addiu $t2, $t2, 0xffff

	/* $t2 is source. $t1 is destination. $t3 is size.  */
modulesmovcont:
	lb $t4, 0($t2)
	sb $t4, 0($t1)
	addiu $t1,$t1,0xffff
	addiu $t2,$t2,0xffff
	addiu $t3, 0xffff
	bne $t3, $0, modulesmovcont

	/* Clean BSS.  */
	
	lui $t1, %hi(__bss_start)
	addiu $t1, %lo(__bss_start)
	lui $t2, %hi(_end)
	addiu $t2, %lo(_end)
bsscont:
	sb $0,0($t1)
	addiu $t1,$t1,1
	sltu $t3,$t1,$t2
	bne $t3, $0, bsscont

	li $sp, GRUB_MACHINE_MEMORY_STACK_HIGH
	lui $t1, %hi(grub_main)
	addiu $t1, %lo(grub_main)

#if __mips >= 2
	sync
#endif
	jr $t1

	. = _start + GRUB_KERNEL_CPU_RAW_SIZE
compressed:	
	. = _start + GRUB_KERNEL_CPU_PREFIX

VARIABLE(grub_prefix)
	/* to be filled by grub-mkelfimage */

	/*
	 *  Leave some breathing room for the prefix.
	 */

	. = _start + GRUB_KERNEL_CPU_DATA_END
