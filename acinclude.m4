dnl grub_CHECK_C_SYMBOL_NAME checks what name gcc will use for C symbol.
dnl Originally written by Erich Boleyn, the author of GRUB, and modified
dnl by OKUJI Yoshinori to autoconfiscate the test.

AC_DEFUN(grub_ASM_EXT_C,
[AC_PROG_CC
if test "x$GCC" != xyes; then
  AC_MSG_ERROR([GNU C compiler not found])
fi

AC_MSG_CHECKING([symbol names produced by ${CC}])
AC_CACHE_VAL(grub_cv_asm_ext_c,
[cat << EOF > conftest.c
int
func (int *list)
{
  *list = 0;
  return *list;
}
EOF

if AC_TRY_COMMAND(${CC} -S conftest.c) && test -s conftest.s; then
  true
else
  AC_MSG_ERROR([${CC} failed to produce assembly code])
fi

set dummy `grep \.globl conftest.s | grep func | sed -e s/\\.globl// | sed -e s/func/\ sym\ /`
shift
if test -z "[$]1"; then
  AC_MSG_ERROR([C symbol not found])
fi
grub_cv_asm_ext_c="[$]1"
while shift; do
  dummy=[$]1
  if test ! -z ${dummy}; then
    grub_cv_asm_ext_c="$grub_cv_asm_ext_c ## $dummy"
  fi
done
rm -f conftest*])

AC_MSG_RESULT($grub_cv_asm_ext_c)
AC_DEFINE_UNQUOTED([EXT_C(sym)], $grub_cv_asm_ext_c)
])


AC_DEFUN(grub_PROG_OBJCOPY_WORKS,
[echo "FIXME: would test objcopy"])

AC_DEFUN(grub_GAS_ADDR32,
[echo "FIXME: would test addr32"])
