#include <grub/symbol.h>

FUNCTION (grub_arch_sync_caches)
FUNCTION (_flush_cache)
	j $31
