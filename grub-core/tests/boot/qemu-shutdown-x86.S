	movl $0x80000b80, %eax
	movw $0xcf8, %dx
	outl  %eax, %dx
	movl $0x1001, %eax
	movw $0xcfc, %dx
	inb  %al, %dx
	orb  $1, %al
	outb  %al, %dx

	movl $0x80000b40, %eax
	movw $0xcf8, %dx
	outl  %eax, %dx
	movl $0x1001, %eax
	movw $0xcfc, %dx
	outl  %eax, %dx
	movw $0x2000, %ax
	movw $0x1004, %dx	
	outw  %ax, %dx
