/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/symbol.h>

	.file	"setjmp.S"
	
	.text
	
/*
 * int pupa_setjmp (pupa_jmp_buf env)
 */
FUNCTION(pupa_setjmp)
	movl	%ebx, 0(%eax)	/* EBX */
	movl	%esi, 4(%eax)	/* ESI */
	movl	%edi, 8(%eax)	/* EDI */
	movl	%ebp, 12(%eax)	/* EBP */
	popl	%ecx
	movl	%esp, 16(%eax)	/* ESP */
	movl	%ecx, 20(%eax)	/* EIP */
	xorl	%eax, %eax
	jmp	*%ecx

		
/*
 * int pupa_longjmp (pupa_jmp_buf env, int val)
 */
FUNCTION(pupa_longjmp)
	movl	0(%eax), %ebx
	movl	4(%eax), %esi
	movl	8(%eax), %edi
	movl	12(%eax), %ebp
	movl	16(%eax), %esp
	movl	20(%eax), %ecx

	movl	%edx, %eax
	testl	%eax, %eax
	jnz	1f
	incl	%eax
1:	jmp	*%ecx
	
