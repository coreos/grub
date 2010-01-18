/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
	.text
	.psr abi64
	.psr lsb
	.lsb
	
	.proc __ia64_trampoline
	.global __ia64_trampoline
__ia64_trampoline:
	/* Read address of the real descriptor.  */
	ld8 r2=[r1],8
	;;
	/* Read chain.  */
	ld8 r15=[r1]
	/* Read pc.  */
	ld8 r3=[r2],8
	;;
	/* Read gp.  */
	ld8 r1=[r2]
	mov b6=r3
	br.many b6
	.endp __ia64_trampoline
