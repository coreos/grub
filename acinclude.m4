dnl grub_ASM_EXT_C checks what name gcc will use for C symbol.
dnl Originally written by Erich Boleyn, the author of GRUB, and modified
dnl by OKUJI Yoshinori to autoconfiscate the test.
AC_DEFUN(grub_ASM_EXT_C,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([symbol names produced by ${CC-cc}])
AC_CACHE_VAL(grub_cv_asm_ext_c,
[cat > conftest.c <<\EOF
int
func (int *list)
{
  *list = 0;
  return *list;
}
EOF

if AC_TRY_COMMAND([${CC-cc} -S conftest.c]) && test -s conftest.s; then :
else
  AC_MSG_ERROR([${CC-cc} failed to produce assembly code])
fi

set dummy `grep \.globl conftest.s | grep func | sed -e s/\\.globl// | sed -e s/func/\ sym\ /`
shift
if test -z "[$]1"; then
  AC_MSG_ERROR([C symbol not found])
fi
grub_cv_asm_ext_c="[$]1"
while test [$]# != 0; do
  shift
  dummy=[$]1
  if test ! -z ${dummy}; then
    grub_cv_asm_ext_c="$grub_cv_asm_ext_c ## $dummy"
  fi
done
rm -f conftest*])

AC_MSG_RESULT([$grub_cv_asm_ext_c])
AC_DEFINE_UNQUOTED([EXT_C(sym)], $grub_cv_asm_ext_c)
])


dnl Some versions of `objcopy -O binary' vary their output depending
dnl on the link address.
AC_DEFUN(grub_PROG_OBJCOPY_ABSOLUTE,
[AC_MSG_CHECKING([whether ${OBJCOPY} works for absolute addresses])
AC_CACHE_VAL(grub_cv_prog_objcopy_absolute,
[cat > conftest.c <<\EOF
void
blah (void)
{
   *((int *) 0x1000) = 2;
}
EOF

if AC_TRY_EVAL(ac_compile) && test -s conftest.o; then :
else
  AC_MSG_ERROR([${CC-cc} cannot compile C source code])
fi
grub_cv_prog_objcopy_absolute=yes
for link_addr in 2000 8000 7C00; do
  if AC_TRY_COMMAND([${LD-ld} -N -Ttext $link_addr conftest.o -o conftest.exec]); then :
  else
    AC_MSG_ERROR([${LD-ld} cannot link at address $link_addr])
  fi
  if AC_TRY_COMMAND([${OBJCOPY-objcopy} -O binary conftest.exec conftest]); then :
  else
    AC_MSG_ERROR([${OBJCOPY-objcopy} cannot create binary files])
  fi
  if test ! -f conftest.old || AC_TRY_COMMAND([cmp -s conftest.old conftest]); then
    mv -f conftest conftest.old
  else
    grub_cv_prog_objcopy_absolute=no
    break
  fi
done
rm -f conftest*])
AC_MSG_RESULT([$grub_cv_prog_objcopy_absolute])])

dnl Mass confusion!
dnl Older versions of GAS interpret `.code16' to mean ``generate 32-bit
dnl instructions, but implicitly insert addr32 and data32 bytes so
dnl that the code works in real mode''.
dnl
dnl Newer versions of GAS interpret `.code16' to mean ``generate 16-bit
dnl instructions,'' which seems right.  This requires the programmer
dnl to explicitly insert addr32 and data32 instructions when they want
dnl them.
dnl
dnl We only support the newer versions, because the old versions cause
dnl major pain, by requiring manual assembly to get 16-bit instructions into
dnl stage1/stage1.S.
AC_DEFUN(grub_ASM_ADDR32,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([for .code16 addr32 assembler support])
AC_CACHE_VAL(grub_cv_asm_addr32,
[cat > conftest.s <<\EOF
	.code16
l1:	addr32
	movb	%al, l1
EOF

if AC_TRY_COMMAND([${CC-cc} -c conftest.s]) && test -s conftest.o; then
  grub_cv_asm_addr32=yes
else
  grub_cv_asm_addr32=no
fi
rm -f conftest*])
AC_MSG_RESULT([$grub_cv_asm_addr32])])
