/* on x86 old clang doesn't support .code16
   newer clang supports it but creates 6-byte jumps instead of 3-byte ones
   which makes us go over boot sector size.  */

	.code16
	jmp far
	.org 4
	.space 300
far:
	.byte 0
