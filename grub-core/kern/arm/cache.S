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

	.file	"cache.S"
	.text
	.syntax	unified
#if !defined (__thumb2__) || !defined (ARMV7)
	.arm
#else
	.thumb
#endif

#if !defined (ARMV6) && !defined (ARMV7)
# error Unsupported architecture version!
#endif

	.align	2

/*
 * Simple cache maintenance functions
 */

dlinesz_addr:
	.long EXT_C(grub_arch_cache_dlinesz)
ilinesz_addr:
	.long EXT_C(grub_arch_cache_ilinesz)

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)
clean_dcache_range:
	@ Clean data cache for range to point-of-unification
	ldr	r2, dlinesz_addr
	ldr	r2, [r2]
	sub	r3, r2, #1		@ align "beg" to start of line
	mvn	r3, r3
	and	r0, r0, r3
1:	cmp	r0, r1
	bge	2f
#ifdef ARMV6
	mcr	p15, 0, r0, c7, c10, 1	@ Clean data cache line by MVA
#else
	mcr	p15, 0, r0, c7, c11, 1	@ DCCMVAU
#endif
	add	r0, r0, r2		@ Next line
	b	1b
2:	DSB
	bx	lr

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)
invalidate_icache_range:
	@ Invalidate instruction cache for range to point-of-unification
	ldr	r2, ilinesz_addr
	ldr	r2, [r2]
	sub	r3, r2, #1		@ align "beg" to start of line
	mvn	r3, r3
	and	r0, r0, r3
1:	cmp	r0, r1
	bge	2f
	mcr	p15, 0, r0, c7, c5, 1	@ ICIMVAU
	add	r0, r0, r2		@ Next line
	b	1b
	@ Branch predictor invalidate all
2:	mcr	p15, 0, r0, c7,	c5, 6	@ BPIALL
	DSB
	ISB
	bx	lr

@void grub_arch_sync_caches (void *address, grub_size_t len)
#ifdef ARMV6
FUNCTION(grub_arch_sync_caches_armv6)
#else
FUNCTION(grub_arch_sync_caches_armv7)
#endif
	DSB
	add	r1, r0, r1
	push	{r0-r2, lr}
	bl	clean_dcache_range
	pop	{r0, r1}
	bl	invalidate_icache_range
	pop	{r2, pc}

#ifdef ARMV6
FUNCTION(grub_arm_disable_caches_mmu_armv6)
#else
FUNCTION(grub_arm_disable_caches_mmu_armv7)
#endif

	push	{r4, lr}

	@ disable D-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0
	DSB
	ISB

	@ clean/invalidate D-cache
	bl	clean_invalidate_dcache

	@ disable I-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0
	DSB
	ISB

	@ invalidate I-cache (also invalidates branch predictors)
	mcr	p15, 0, r0, c7, c5, 0
	DSB
	ISB

	@ clear SCTLR M bit
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 0)
	mcr	p15, 0, r0, c1, c0, 0

	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLB
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate branch predictor
	DSB
	ISB

	pop	{r4, pc}

