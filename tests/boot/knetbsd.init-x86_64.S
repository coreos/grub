/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#define MODE_RDRW 2
#define FLAGS_NONE 0
#define SYSCALL_OPEN 5
#define SYSCALL_WRITE 4
#define SYSCALL_RESET 208
#define SYSCALL_EXIT 1
#define SYSCALL_MKNOD 14
#define SYSCALL_MOUNT 410
#define SYSCALL_INT 0x80

#define RESET_NOSYNC 0x4
#define RESET_HALT 0x8
#define RESET_POWEROFF 0x800

	.section ".init", "ax"
	.global start,_start
start:
_start:
	/* mount.  */
	movq $SYSCALL_MOUNT, %rax
	movq $devfstype, %rdi
	movq $devfsdir, %rsi
	movq $0, %rdx
	movq $tmpfs_args, %r10
	movq $(tmpfs_args_end - tmpfs_args), %r8
	syscall

	/* mknod.  */
	movq $SYSCALL_MKNOD, %rax
	leaq device, %rdi
	movq $0x2140, %rsi
	movq $0, %rdx
	syscall

	/* open.  */
	movq $SYSCALL_OPEN, %rax
	leaq device, %rdi
	movq $MODE_RDRW, %rsi
	movq $FLAGS_NONE, %rdx
	syscall
	movq %rax, %rdi

	/* write.  */
	movq $SYSCALL_WRITE, %rax
	movq $(messageend-message), %rdx
	leaq message, %rsi
	syscall

	/* shutdown.  */
	movq $SYSCALL_RESET, %rax
	movq $(RESET_POWEROFF|RESET_HALT|RESET_NOSYNC), %rdi
	movq $0, %rsi
	syscall

	/* exit (1). Shouldn't be reached.  */
	movq $SYSCALL_EXIT, %rax
	movq $1, %rdi
	syscall
	.section ".fini", "ax"
1:	jmp 1b
	.section ".text", "ax"
1:	jmp 1b
	/* This section is needed for NetBSD to identify the binary.  */
	.section ".note.netbsd.ident", "a"
	.long 0x7
	.long 0x4
	.long 0x1
	.ascii "NetBSD"
	.byte 0
	.data
device:
	.ascii "/dev/console"
	.byte 0
message:
	.ascii "Boot Test Passed Successfully\n" SUCCESSFUL_BOOT_STRING "\n"
messageend:
devfstype:
	.ascii "tmpfs"
	.byte 0
devfsdir:
	.ascii "/dev"
	.byte 0
tmpfs_args:
	/* Version.  */
	.long 1

	/* Alignment long.  */
	.long 0

	/* Maximum inodes.  */
	.quad 	0
	/* Maximum size.  */
	.quad   0

	/* UID */
	.long		0
	/* GID */
	.long 		0
	/* Mode */
	.long		0777
	/* Alignment long.  */
	.long 0
tmpfs_args_end:	