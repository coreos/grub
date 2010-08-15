# Process this file with autoconf to produce a configure script.

# Copyright (C) 2010  Free Software Foundation, Inc.
#
# This configure.ac is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

dnl This configure script is complicated, because GRUB needs to deal
dnl with three potentially different types:
dnl
dnl   build  -- the environment for building GRUB
dnl   host   -- the environment for running utilities
dnl   target -- the environment for running GRUB
dnl
dnl In addition, GRUB needs to deal with a platform specification
dnl which specifies the system running GRUB, such as firmware.
dnl This is necessary because the target type in autoconf does not
dnl describe such a system very well.
dnl
dnl The current strategy is to build utilities using host
dnl cross-compiler and grub core and modules using target
dnl cross-compiler.  For this we use nested packages approach, where
dnl top-level package grub utilities is built with HOSTCC and nested
dnl package (in grub-core directory) builds with TARGETCC.

# NOTE: ../configure.ac must also be updated.
AC_INIT([GRUB],[1.98],[bug-grub@gnu.org])
AC_CONFIG_AUX_DIR([.])

: ${CFLAGS=""} # We don't want -g -O2

# Checks for host and target systems.
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE()
AC_PREREQ(2.60)
AC_CONFIG_SRCDIR([include/grub/dl.h])
AC_CONFIG_HEADER([config.h])

grub_coredir='.'
AC_SUBST(grub_coredir)

grub_utildir='..'
AC_SUBST(grub_utildir)

m4_include([configure.common])

CC=$TARGET_CC
CPP=$TARGET_CC
CCAS=$TARGET_CC

# Output files.
grub_CHECK_LINK_DIR
if test x"$link_dir" = xyes ; then
  AC_CONFIG_LINKS([include/grub/cpu:include/grub/$target_cpu])
  mkdir -p lib/target_cpu
  cp -r $srcdir/lib/$target_cpu/* lib/target_cpu
  if test "$platform" != emu ; then
    AC_CONFIG_LINKS([include/grub/machine:include/grub/$target_cpu/$platform])
  fi
else
  mkdir -p include/grub 2>/dev/null
  rm -rf include/grub/cpu
  cp -rp $srcdir/include/grub/$target_cpu include/grub/cpu 2>/dev/null
  cp -rp $srcdir/lib/$target_cpu lib/target_cpu 2>/dev/null
  if test "$platform" != emu ; then
    rm -rf include/grub/machine
    cp -rp $srcdir/grub-core/include/grub/$target_cpu/$platform include/grub/machine 2>/dev/null
  fi
fi

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([po/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])

AC_OUTPUT
