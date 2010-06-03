#! /bin/sh

set -e

echo "Creating symlinks..."
ln -svf ../NEWS grub-core/
ln -svf ../TODO grub-core/
ln -svf ../THANKS grub-core/
ln -svf ../README grub-core/
ln -svf ../INSTALL grub-core/
ln -svf ../AUTHORS grub-core/
ln -svf ../COPYING grub-core/
ln -svf ../ABOUT-NLS grub-core/
ln -svf ../ChangeLog grub-core/
ln -svf ../aclocal.m4 grub-core/
ln -svf ../acinclude.m4 grub-core/
ln -svf ../config.rpath grub-core/
ln -svf ../gentpl.py grub-core/
ln -svf ../configure.common grub-core/

echo "Creating Makefile.tpl..."
python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl
echo "Running autogen..."
autogen -T Makefile.tpl modules.def | sed -e '/^$/{N;/^\n$/D;}' > modules.am

echo "Creating grub-core/Makefile.tpl..."
(cd grub-core && python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl)
echo "Running autogen..."
(cd grub-core && autogen -T Makefile.tpl modules.def | sed -e '/^$/{N;/^\n$/D;}' > modules.am)

echo "Importing libgcrypt..."
(cd grub-core && python import_gcry.py lib/libgcrypt/ .)

echo "Saving timestamps..."
echo timestamp > stamp-h.in
(cd grub-core && echo timestamp > stamp-h.in)

echo "Running autoreconf..."
autoreconf -vi
exit 0
