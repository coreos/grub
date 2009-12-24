#! /bin/sh

set -ex

aclocal
autoheader
automake -a -c -f
autoconf

exit 0
