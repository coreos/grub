#! /bin/sh

set -ex

aclocal
autoheader
automake --add-missing
autoconf

exit 0
