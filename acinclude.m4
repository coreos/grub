dnl grub_ASM_USCORE checks if C symbols get an underscore after
dnl compiling to assembler.
dnl Written by Pavel Roskin. Based on grub_ASM_EXT_C written by
dnl Erich Boleyn and modified by OKUJI Yoshinori
AC_DEFUN(grub_ASM_USCORE,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if C symbols get an underscore after compilation])
AC_CACHE_VAL(grub_cv_asm_uscore,
[cat > conftest.c <<\EOF
int
func (int *list)
{
  *list = 0;
  return *list;
}
EOF

if AC_TRY_COMMAND([${CC-cc} -S conftest.c]) && test -s conftest.s; then
  true
else
  AC_MSG_ERROR([${CC-cc} failed to produce assembly code])
fi

if grep _func conftest.s >/dev/null 2>&1; then
  grub_cv_asm_uscore=yes
  AC_DEFINE_UNQUOTED([HAVE_ASM_USCORE], $grub_cv_asm_uscore,
    [Define if C symbols get an underscore after compilation])
else
  grub_cv_asm_uscore=no
fi

rm -f conftest*])

AC_MSG_RESULT([$grub_cv_asm_uscore])
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
AC_REQUIRE([grub_ASM_PREFIX_REQUIREMENT])
AC_MSG_CHECKING([for .code16 addr32 assembler support])
AC_CACHE_VAL(grub_cv_asm_addr32,
[cat > conftest.s <<\EOF
	.code16
l1:	ADDR32	movb	%al, l1
EOF

if AC_TRY_COMMAND([${CC-cc} -c conftest.s]) && test -s conftest.o; then
  grub_cv_asm_addr32=yes
else
  grub_cv_asm_addr32=no
fi
rm -f conftest*])
AC_MSG_RESULT([$grub_cv_asm_addr32])])

dnl
dnl Later versions of GAS requires that addr32 and data32 prefixes
dnl appear in the same lines as the instructions they modify, while
dnl earlier versions requires that they appear in separate lines.
AC_DEFUN(grub_ASM_PREFIX_REQUIREMENT,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(dnl
[whether addr32 must be in the same line as the instruction])
AC_CACHE_VAL(grub_cv_asm_prefix_requirement,
[cat > conftest.s <<\EOF
	.code16
l1:	addr32	movb	%al, l1
EOF

if AC_TRY_COMMAND([${CC-cc} -c conftest.s]) && test -s conftest.o; then
  grub_cv_asm_prefix_requirement=yes
else
  grub_cv_asm_prefix_requirement=no
fi

rm -f conftest*])

if test "x$grub_cv_asm_prefix_requirement" = xyes; then
  grub_tmp_addr32="addr32"
  grub_tmp_data32="data32"
else
  grub_tmp_addr32="addr32;"
  grub_tmp_data32="data32;"
fi

AC_DEFINE_UNQUOTED([ADDR32], $grub_tmp_addr32,
  [Define it to \"addr32\" or \"addr32;\" to make GAS happy])
AC_DEFINE_UNQUOTED([DATA32], $grub_tmp_data32,
  [Define it to \"data32\" or \"data32;\" to make GAS happy])

AC_MSG_RESULT([$grub_cv_asm_prefix_requirement])])

dnl
dnl grub_CHECK_START_SYMBOL checks if start is automatically defined by
dnl the compiler.
dnl Written by OKUJI Yoshinori
AC_DEFUN(grub_CHECK_START_SYMBOL,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if start is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_start_symbol,
[cat > conftest.c <<\EOF
int
main (void)
{
  asm ("incl start");
  return 0;
}
EOF

if AC_TRY_COMMAND([${CC-cc} conftest.c -o conftest]) && test -s conftest; then
  grub_cv_check_start_symbol=yes
else
  grub_cv_check_start_symbol=no
fi

rm -f conftest*])

if test "x$grub_cv_check_start_symbol" = xyes; then
  AC_DEFINE([HAVE_START_SYMBOL])
fi

AC_MSG_RESULT([$grub_cv_check_start_symbol])
])

dnl
dnl grub_CHECK_USCORE_START_SYMBOL checks if _start is automatically
dnl defined by the compiler.
dnl Written by OKUJI Yoshinori
AC_DEFUN(grub_CHECK_USCORE_START_SYMBOL,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if _start is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_uscore_start_symbol,
[cat > conftest.c <<\EOF
int
main (void)
{
  asm ("incl _start");
  return 0;
}
EOF

if AC_TRY_COMMAND([${CC-cc} conftest.c -o conftest]) && test -s conftest; then
  grub_cv_check_uscore_start_symbol=yes
else
  grub_cv_check_uscore_start_symbol=no
fi

rm -f conftest*])

if test "x$grub_cv_check_uscore_start_symbol" = xyes; then
  AC_DEFINE([HAVE_USCORE_START_SYMBOL])
fi

AC_MSG_RESULT([$grub_cv_check_uscore_start_symbol])
])

dnl
dnl grub_CHECK_END_SYMBOL checks if end is automatically defined by the
dnl compiler.
dnl Written by OKUJI Yoshinori
AC_DEFUN(grub_CHECK_END_SYMBOL,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if end is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_end_symbol,
[cat > conftest.c <<\EOF
int
main (void)
{
  asm ("incl end);
  return 0;
}
EOF

if AC_TRY_COMMAND([${CC-cc} conftest.c -o conftest]) && test -s conftest; then
  grub_cv_check_end_symbol=yes
else
  grub_cv_check_end_symbol=no
fi

rm -f conftest*])

if test "x$grub_cv_check_end_symbol" = xyes; then
  AC_DEFINE([HAVE_END_SYMBOL])
fi

AC_MSG_RESULT([$grub_cv_check_end_symbol])
])

dnl
dnl grub_CHECK_USCORE_END_SYMBOL checks if _end is automatically defined
dnl by the compiler.
dnl Written by OKUJI Yoshinori
AC_DEFUN(grub_CHECK_USCORE_END_SYMBOL,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if _end is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_uscore_end_symbol,
[cat > conftest.c <<\EOF
int
main (void)
{
  asm ("incl _end");
  return 0;
}
EOF

if AC_TRY_COMMAND([${CC-cc} conftest.c -o conftest]) && test -s conftest; then
  grub_cv_check_uscore_end_symbol=yes
else
  grub_cv_check_uscore_end_symbol=no
fi

if test "x$grub_cv_check_uscore_end_symbol" = xyes; then
  AC_DEFINE([HAVE_USCORE_END_SYMBOL])
fi

rm -f conftest*])

AC_MSG_RESULT([$grub_cv_check_uscore_end_symbol])
])
