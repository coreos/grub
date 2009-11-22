#include <grub/symbol.h>

	/* FIXME: This should invalidate only part of memory.  */
FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
repeat:
	cache 1, 0($a0)
	cache 0, 0($a0)
	addiu $a0, $a0, 1
	addiu $a1, $a1, 0xffff
	bne $a1, $zero, repeat
	j $31
