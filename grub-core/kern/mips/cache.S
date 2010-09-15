
#include <grub/symbol.h>

FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
#include "cache_flush.S"
	j $ra
