# Process this file with autoconf to produce a configure script.

# Copyright (C) 2002,2003  Yoshinori K. Okuji <okuji@enbug.org>
#
# This configure.ac is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

AC_INIT(PUPA, 0.6, [okuji@enbug.org])
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR([include/pupa/dl.h])
AC_CONFIG_HEADER([config.h])

# Checks for build and host systems.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

case "$host_cpu" in
  i[[3456]]86) host_cpu=i386 ;;
  *) AC_MSG_ERROR([unsupported CPU type]) ;;
esac

case "$host_cpu"-"$host_vendor" in
  i386-*) host_vendor=pc ;;
  *) AC_MSG_ERROR([unsupported machine type]) ;;
esac

AC_SUBST(host_cpu)
AC_SUBST(host_vendor)

# Checks for programs.
if test "x$CFLAGS" = x; then
  default_CFLAGS=yes
fi

AC_PROG_CC

# Must be GCC.
test "x$GCC" = xyes || AC_MSG_ERROR([GCC is required])

if test "x$default_CFLAGS" = xyes; then
  # debug flags.
  tmp_CFLAGS="-Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes \
		-Wundef -Wstrict-prototypes -g"

  # optimization flags.
  AC_CACHE_CHECK([whether optimization for size works], size_flag, [
    CFLAGS=-Os
    AC_TRY_COMPILE(, , size_flag=yes, size_flag=no)
  ])
  if test "x$size_flag" = xyes; then
    tmp_CFLAGS="$tmp_CFLAGS -Os"
  else
    tmp_CFLAGS="$tmp_CFLAGS -O2 -fno-strength-reduce -fno-unroll-loops"
  fi

  # Force no alignment to save space on i386.
  if test "x$host_cpu" = xi386; then
    AC_CACHE_CHECK([whether -falign-loops works], [falign_loop_flag], [
      CFLAGS="-falign-loops=1"
      AC_TRY_COMPILE(, , [falign_loop_flag=yes], [falign_loop_flag=no])
    ])

    if test "x$falign_loop_flag" = xyes; then
      tmp_CFLAGS="$tmp_CFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
    else
      tmp_CFLAGS="$tmp_CFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
    fi
  fi

  CFLAGS="$tmp_CFLAGS"
fi
AC_SUBST(CFLAGS)

# Defined in aclocal.m4.
pupa_ASM_USCORE
pupa_CHECK_START_SYMBOL
pupa_CHECK_BSS_START_SYMBOL
pupa_CHECK_END_SYMBOL

if test "x$host_cpu" = xi386; then
  pupa_I386_ASM_PREFIX_REQUIREMENT
  pupa_I386_ASM_ADDR32
  pupa_I386_ASM_ABSOLUTE_WITHOUT_ASTERISK
fi


AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_TOOL(OBJCOPY, objcopy)
pupa_PROG_OBJCOPY_ABSOLUTE
AC_CHECK_TOOL(STRIP, strip)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(LD, ld)

# This is not a "must".
AC_PATH_PROG(RUBY, ruby)

# For cross-compiling.
if test "x$build" = "x$host"; then
  BUILD_CC="$CC"
  AC_SUBST(BUILD_CC)
else
  AC_CHECK_PROGS(BUILD_CC, [gcc egcs cc],
		 [AC_MSG_ERROR([none of gcc, egcs and cc is found. set BUILD_CC manually.])])
fi

# Test the C compiler for the build environment.
tmp_CC="$CC"
tmp_CFLAGS="$CFLAGS"
tmp_CPPFLAGS="$CPPFLAGS"
CC="$BUILD_CC"
CFLAGS="$BUILD_CFLAGS"
CPPFLAGS="$BUILD_CPPFLAGS"

# Identify characteristics of the build architecture.
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)

# Check LZO.
AC_CHECK_LIB(lzo, __lzo_init2, ,
	     AC_MSG_ERROR([LZO library version 1.02 or later is required]))
AC_CHECK_FUNC(lzo1x_999_compress, , 
	      [AC_MSG_ERROR([LZO1X-999 must be enabled])])

CC="$tmp_CC"
CFLAGS="$tmp_CFLAGS"
CPPFLAGS="$tmp_CPPFLAGS"

# Output files.
AC_CONFIG_LINKS([include/pupa/cpu:include/pupa/$host_cpu
	include/pupa/machine:include/pupa/$host_cpu/$host_vendor])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
