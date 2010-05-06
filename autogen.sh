#! /bin/sh

set -e

ln -sf ../NEWS             grub-core/
ln -sf ../README           grub-core/
ln -sf ../INSTALL          grub-core/
ln -sf ../AUTHORS          grub-core/
ln -sf ../COPYING          grub-core/
ln -sf ../ABOUT-NLS        grub-core/
ln -sf ../ChangeLog        grub-core/
ln -sf ../aclocal.m4       grub-core/
ln -sf ../acinclude.m4     grub-core/
ln -sf ../config.rpath     grub-core/
ln -sf ../gentpl.py        grub-core/
ln -sf ../configure.common grub-core/

ln -sf grub-core/include .
ln -sf grub-core/gnulib .
ln -sf grub-core/lib .

python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl
autogen -T Makefile.tpl modules.def | sed -e '/^$/{N;/^\n$/D;}' > modules.am

(cd grub-core && python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl)
(cd grub-core && autogen -T Makefile.tpl modules.def | sed -e '/^$/{N;/^\n$/D;}' > modules.am)

(cd grub-core && echo timestamp > stamp-h.in)
(cd grub-core && python import_gcry.py lib/libgcrypt/ .)

echo timestamp > stamp-h.in
autoreconf -vi

exit 0
