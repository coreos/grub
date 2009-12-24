#! /bin/sh

set -ex

aclocal
autoheader

# Automake is unhappy without NEWS and README, but we don't have any
touch NEWS README
automake -a -c -f
rm -f NEWS README

autoconf

exit 0
