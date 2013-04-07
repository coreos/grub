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
#include <grub/dl.h>

	.file	"cache.S"
	.text
	.syntax	unified
#if !defined (__thumb2__)
	.arm
#define ARM(x...)	x
#define THUMB(x...)
#else
	.thumb
#define THUMB(x...)	x
#define ARM(x...)
#endif

	.align	2

/*
 * Simple cache maintenance functions
 */

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)	
clean_dcache_range:
	@ Clean data cache range for range to point-of-unification
	ldr	r2, dlinesz
1:	cmp	r0, r1
	bge	2f
#ifdef DEBUG
	push	{r0-r2, lr}
	mov	r1, r2
	mov	r2, r0
	ldr	r0, =dcstr
	bl	EXT_C(grub_printf)
	pop	{r0-r2, lr}
#endif
	mcr	p15, 0, r0, c7, c11, 1	@ DCCMVAU
	add	r0, r0, r2		@ Next line
	b	1b
2:	dsb
	bx	lr

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)	
invalidate_icache_range:
	@ Invalidate instruction cache for range to point-of-unification
	ldr	r2, ilinesz
1:	cmp	r0, r1
	bge	2f
#ifdef DEBUG
	push	{r0-r2, lr}
	mov	r1, r2
	mov	r2, r0
	ldr	r0, =icstr
	bl	EXT_C(grub_printf)
	pop	{r0-r2, lr}
#endif
	mcr	p15, 0, r0, c7, c5, 1	@ ICIMVAU
	add	r0, r0, r2		@ Next line
	b	1b
	@ Branch predictor invalidate all
2:	mcr	p15, 0, r0, c7,	c5, 6	@ BPIALL
	dsb
	isb
	bx	lr
	
@void __wrap___clear_cache(char *beg, char *end);
FUNCTION(__wrap___clear_cache)
	dmb
	dsb
	push	{r4-r6, lr}
	ldr	r2, probed	@ If first call, probe cache sizes
	cmp	r2, #0
	bleq	probe_caches	@ This call corrupts r3
	mov	r4, r0
	mov	r5, r1
	bl	clean_dcache_range
	mov	r0, r4
	mov	r1, r5
	bl	invalidate_icache_range
	pop	{r4-r6, pc}

probe_caches:
	push	{r4-r6, lr}
	mrc 	p15, 0, r4, c0, c0, 1	@ Read Cache Type Register
	mov	r5, #1
	ubfx	r6, r4, #16, #4		@ Extract min D-cache num word log2
	add	r6, r6, #2		@ words->bytes
	lsl	r6, r5, r6		@ Convert to num bytes
	ldr	r3, =dlinesz
	str	r6, [r3]
	and	r6, r4, #0xf		@ Extract min I-cache num word log2
	add	r6, r6, #2		@ words->bytes
	lsl	r6, r5, r6		@ Convert to num bytes
	ldr	r3, =ilinesz
	str	r6, [r3]
	ldr	r3, =probed		@ Flag cache probing done
	str	r5, [r3]
	pop	{r4-r6, pc}

#ifdef DEBUG
dcstr:	.asciz	"cleaning %d bytes of D cache @ 0x%08x\n"
icstr:	.asciz	"invalidating %d bytes of I cache @ 0x%08x\n"
#endif
	
	.align	3
probed:	.long	0
dlinesz:
	.long	0
ilinesz:
	.long	0

@void grub_arch_sync_caches (void *address, grub_size_t len)
FUNCTION(grub_arch_sync_caches)
	add	r1, r0, r1
	b	__wrap___clear_cache

	@ r0  - CLIDR
	@ r1  - LoC
	@ r2  - current level
	@ r3  - num sets
	@ r4  - num ways
	@ r5  - current set
	@ r6  - current way
	@ r7  - line size
	@ r8  - scratch
	@ r9  - scratch
	@ r10 - scratch
	@ r11 - scratch
clean_invalidate_dcache:
	push	{r4-r12, lr}
	mrc 	p15, 1, r0, c0, c0, 1	@ Read CLIDR
	ubfx	r1, r0, #24, #3		@ Extract LoC
	
	mov	r2, #0			@ First level, L1
2:	and	r8, r0, #7		@ cache type at current level
	cmp	r8, #2
	blt	5f			@ instruction only, or none, skip level

	@ set current cache level/type (for CSSIDR read)
	lsl	r8, r2, #1
	mcr	p15, 2, r8, c0, c0, 0	@ Write CSSELR (level, type: data/uni)

	@ read current cache information
	mrc	p15, 1, r8, c0, c0, 0	@ Read CSSIDR
	ubfx	r3, r8, #13, #14	@ Number of sets -1
	ubfx	r4, r8, #3, #9		@ Number of ways -1
	and	r7, r8, #7		@ log2(line size in words) - 2
	add	r7, r7, #2		@  adjust
	mov	r8, #1
	lsl	r7, r8, r7		@  -> line size in words
	lsl	r7, r7, #2		@  -> bytes

	@ set loop
	mov	r5, #0			@ current set = 0
3:	lsl	r8, r2, #1		@ insert level
	clz	r9, r7			@ calculate set field offset
	mov	r10, #31
	sub	r9, r10, r9
	lsl	r10, r5, r9
	orr	r8, r8, r10		@ insert set field

	@ way loop
	@ calculate way field offset
	mov	r6, #0			@ current way = 0
	add	r10, r4, #1
	clz	r9, r10			@ r9 = way field offset
	add	r9, r9, #1
4:	lsl	r10, r6, r9
	orr	r11, r8, r10		@ insert way field	
	
	@ clean line by set/way
	mcr	p15, 0, r11, c7, c14, 2	@ DCCISW
	
	@ next way
	add	r6, r6, #1
	cmp	r6, r4
	ble	4b

	@ next set
	add	r5, r5, #1
	cmp	r5, r3
	ble	3b
	
	@ next level
5:	lsr	r0, r0, #3		@ align next level CLIDR 'type' field
	add	r2, r2, #1		@ increment cache level counter
	cmp	r2, r1
	blt	2b			@ outer loop

	@ return
6:	dsb
	isb
	pop	{r4-r12, pc}

FUNCTION(grub_arm_disable_caches_mmu)
	push	{r4, lr}

	@ disable D-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0
	dsb
	isb

	@ clean/invalidate D-cache
	bl	clean_invalidate_dcache

	@ disable I-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0
	dsb
	isb

	@ invalidate I-cache (also invalidates branch predictors)
	mcr	p15, 0, r0, c7, c5, 0
	dsb
	isb
	
	@ clear SCTLR M bit
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 0)
	mcr	p15, 0, r0, c1, c0, 0

	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLB
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate branch predictor
	dsb
	isb

	pop	{r4, pc}

