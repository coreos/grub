#! /usr/bin/env bash

set -e

autogen --version >/dev/null || exit 1

echo "Importing unicode..."
python util/import_unicode.py unicode/UnicodeData.txt unicode/BidiMirroring.txt unicode/ArabicShaping.txt grub-core/unidata.c

echo "Importing libgcrypt..."
python util/import_gcry.py grub-core/lib/libgcrypt/ grub-core

echo "Creating Makefile.tpl..."
python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl

echo "Running autogen..."
autogen -T Makefile.tpl Makefile.util.def | sed -e '/^$/{N;/^\n$/D;}' > Makefile.util.am
autogen -T Makefile.tpl grub-core/Makefile.core.def | sed -e '/^$/{N;/^\n$/D;}' > grub-core/Makefile.core.am
autogen -T Makefile.tpl grub-core/Makefile.gcry.def | sed -e '/^$/{N;/^\n$/D;}' > grub-core/Makefile.gcry.am

echo "Saving timestamps..."
echo timestamp > stamp-h.in

echo "Running autoreconf..."
autoreconf -vi
exit 0
