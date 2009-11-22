#include <grub/symbol.h>

	/* FIXME: This should invalidate only part of memory.  */
FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
	move $t2, $a0
	addu $t3, $a0, $a1
	srl $t2, $t2, 5
	sll $t2, $t2, 5
	addu $t3, $t3, 0x1f
	srl $t3, $t3, 5
	sll $t3, $t3, 5
	move $t0, $t2
	subu $t1, $t3, $t2
r1:
	cache 1, 0($t0)
	addiu $t0, $t0, 0x1
	addiu $t1, $t1, 0xffff
	bne $t1, $zero, r1
	sync
	move $t0, $t2
	subu $t1, $t3, $t2
r2:
	cache 0, 0($t0)
	addiu $t0, $t0, 0x1
	addiu $t1, $t1, 0xffff
	bne $t1, $zero, r2
	sync
	j $ra
