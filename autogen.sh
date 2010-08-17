#! /bin/sh

set -e

autogen --version >/dev/null || (echo autogen missing; exit 1)

echo "Creating Makefile.tpl..."
python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl

echo "Running autogen..."
autogen -T Makefile.tpl modules.def | sed -e '/^$/{N;/^\n$/D;}' > modules.am
autogen -T Makefile.tpl grub-core/modules.def | sed -e '/^$/{N;/^\n$/D;}' > grub-core/modules.am

echo "Importing unicode..."
python util/import_unicode.py unicode/UnicodeData.txt unicode/BidiMirroring.txt unicode/ArabicShaping.txt grub-core/unidata.c

echo "Importing libgcrypt..."
python util/import_gcry.py grub-core/lib/libgcrypt/ grub-core

echo "Saving timestamps..."
echo timestamp > stamp-h.in

echo "Running autoreconf..."
autoreconf -vi
exit 0
