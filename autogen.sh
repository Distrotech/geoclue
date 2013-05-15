#!/bin/sh

set -e # exit on errors

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
mkdir -p m4

git submodule update --init --recursive
autoreconf -v --force --install
intltoolize -f

if [ -z "$NOCONFIGURE" ]; then
    "$srcdir"/configure --enable-maintainer-mode --enable-debug ${1+"$@"}
fi
