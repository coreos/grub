#include <grub/symbol.h>

	/* FIXME: This should invalidate only part of memory.  */
FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
#if __mips >= 2
	sync
#endif
	j $31
