/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

	.file	"cache_flush.S"
	.text

/*
 * Simple cache maintenance functions
 */

// r0 - *beg (inclusive)
// r1 - *end (exclusive)
clean_dcache_range:
	// Clean data cache for range to point-of-unification
	ldr	x2, =EXT_C(grub_arch_cache_dlinesz)
	ldr	x2, [x2]
1:	cmp	x0, x1
	bge	2f
	dc	cvau, x0		// Clean Virtual Address to PoU
	add	x0, x0, x2		// Next line
	b	1b
2:	dsb	ish
	ret

// r0 - *beg (inclusive)
// r1 - *end (exclusive)
invalidate_icache_range:
	// Invalidate instruction cache for range to point-of-unification
	ldr	x2, =EXT_C(grub_arch_cache_ilinesz)
	ldr	x2, [x2]
1:	cmp	x0, x1
	bge	2f
	ic	ivau, x0		// Invalidate Virtual Address to PoU
	add	x0, x0, x2		// Next line
	b	1b
	// Branch predictor invalidation not needed on AArch64
2:	dsb	ish
	isb
	ret

// void grub_arch_sync_caches_real (void *address, grub_size_t len)
FUNCTION(grub_arch_sync_caches_real)
	dsb	ish
	stp	x0, x30, [sp, #-16]!
	stp	x0, x1, [sp, #-16]!
	bl	clean_dcache_range
	ldp	x0, x1, [sp], #16
	bl	invalidate_icache_range
	ldp	x0, x30, [sp], #16
	ret
