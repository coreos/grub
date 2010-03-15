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
base:	
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
	/* Save our base.  */
	move $s0, $ra

	/* Parse arguments. Has to be done before relocation.
	So need to do it in asm.  */
#ifdef GRUB_MACHINE_MIPS_YEELOONG
	/* $a2 has the environment.  */
	move $t0, $a2
argcont:	
	lw $t1, 0($t0)
	beq $t1, $zero, argdone
#define DO_PARSE(str, reg) \
	addiu $t2, $s0, (str-base);\
	bal parsestr;\
	beq $v0, $zero, 1f;\
	move reg, $v0;\
	b 2f;\
1:	
	DO_PARSE (busclockstr, $s2)
	DO_PARSE (cpuclockstr, $s3)
	DO_PARSE (memsizestr, $s4)
	DO_PARSE (highmemsizestr, $s5)
2:	
	addiu $t0, $t0, 4
	b argcont
parsestr:
	move $v0, $zero
	move $t3, $t1
3:	
	lb $t4, 0($t2)
	lb $t5, 0($t3)
	addiu $t2, $t2, 1
	addiu $t3, $t3, 1
	beq $t5, $zero, 1f
	beq $t5, $t4, 3b
	bne $t4, $zero, 1f

	addiu $t3, $t3, 0xffff
digcont:	
	lb $t5, 0($t3)
	/* Substract '0' from digit.  */
	addiu $t5, $t5, 0xffd0
	bltz $t5, 1f
	addiu $t4, $t5, 0xfff7
	bgtz $t4, 1f
	/* Multiply $v0 by 10 with bitshifts. */
	sll $v0, $v0, 1
	sll $t4, $v0, 2
	addu $v0, $v0, $t4
	addu $v0, $v0, $t5
	addiu $t3, $t3, 1
	b digcont
1:
	jr $ra
busclockstr:	.asciiz "busclock="
cpuclockstr:	.asciiz "cpuclock="
memsizestr:	.asciiz "memsize="
highmemsizestr:	.asciiz "highmemsize="
	.p2align 2
argdone:
#endif

	/* Decompress the payload. */
	addiu $a0, $s0, GRUB_KERNEL_CPU_RAW_SIZE - BASE_ADDR
	lui $a1, %hi(compressed)
	addiu $a1, %lo(compressed)
	lw $a2, (GRUB_KERNEL_CPU_COMPRESSED_SIZE - BASE_ADDR)($s0)
	move $s1, $a1

	/* $a0 contains source compressed address, $a1 is destination,
	   $a2 is compressed size. FIXME: put LZMA here. Don't clober $s0,
	   $s1, $s2, $s3, $s4 and $s5.
	   On return $v0 contains uncompressed size.
	*/
	move $v0, $a2
reloccont:
	lb $t4, 0($a0)
	sb $t4, 0($a1)
	addiu $a1,$a1,1
	addiu $a0,$a0,1
	addiu $a2, 0xffff
	bne $a2, $0, reloccont

	move $a0, $s1
	move $a1, $v0

#include "cache_flush.S"

	lui $t1, %hi(cont)
	addiu $t1, %lo(cont)

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
#ifdef GRUB_MACHINE_MIPS_YEELOONG
VARIABLE (grub_arch_busclock)
	.long 0
VARIABLE (grub_arch_cpuclock)
	.long 0
VARIABLE (grub_arch_memsize)
	.long 0
VARIABLE (grub_arch_highmemsize)
	.long 0
#endif
cont:

#ifdef GRUB_MACHINE_MIPS_YEELOONG
	lui $t1, %hi(grub_arch_busclock)
	addiu $t1, %lo(grub_arch_busclock)
	sw $s2, 0($t1)
	sw $s3, 4($t1)
	sw $s4, 8($t1)
	sw $s5, 12($t1)
#endif

	/* Move the modules out of BSS.  */
	lui $t1, %hi(_start)
	addiu $t1, %lo(_start)
	lw $t2, (GRUB_KERNEL_CPU_KERNEL_IMAGE_SIZE - BASE_ADDR)($s0)
	addu $t2, $t1, $t2
	
	lui $t1, %hi(_end)
	addiu $t1, %lo(_end)
	addiu $t1, (GRUB_MOD_ALIGN-1)
	li $t3, (GRUB_MOD_ALIGN-1)
	nor $t3, $t3, $0
	and $t1, $t1, $t3
	
	lw $t3, (GRUB_KERNEL_CPU_TOTAL_MODULE_SIZE - BASE_ADDR)($s0)

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

	jr $t1

