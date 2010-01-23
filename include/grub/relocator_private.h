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

#ifndef GRUB_RELOCATOR_PRIVATE_HEADER
#define GRUB_RELOCATOR_PRIVATE_HEADER	1

#include <grub/types.h>
#include <grub/err.h>

extern grub_size_t grub_relocator_align;
extern grub_size_t grub_relocator_forward_size;
extern grub_size_t grub_relocator_backward_size;
extern grub_size_t grub_relocator_jumper_size;

void
grub_cpu_relocator_init (void);
grub_err_t
grub_relocator_prepare_relocs (struct grub_relocator *rel, grub_addr_t addr,
			       grub_addr_t *relstart, grub_size_t *relsize);
void grub_cpu_relocator_forward (void *rels, void *src, void *tgt,
				 grub_size_t size);
void grub_cpu_relocator_backward (void *rels, void *src, void *tgt,
				 grub_size_t size);
void grub_cpu_relocator_jumper (void *rels, grub_addr_t addr);

#endif
