#include <grub/symbol.h>

	.text
	.p2align 4

FUNCTION(grub_int_handler)
	/* FIXME: push this only when CPU pushes no error on stack.  */
	push $0
	pusha
	movl 36(%esp), %eax
	movl %ebp, %edx
	call EXT_C(grub_interrupt_handler_real)
