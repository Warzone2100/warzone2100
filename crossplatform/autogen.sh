#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# This is a kludge to make Gentoo behave and select the
# correct version of automake to use.
WANT_AUTOMAKE=1.8
export WANT_AUTOMAKE

SRCDIR=`dirname $0`
BUILDDIR=`pwd`
srcfile=src/action.c

# Chdir to the srcdir, then run auto* tools.
cd $SRCDIR

[ -f $srcfile ] || {
  echo "Are you sure $SRCDIR is a valid source directory?"
  exit 1
}

echo "+ creating acinclude.m4"
cat m4/*.m4 > acinclude.m4

echo "+ running aclocal ..."
aclocal $ACLOCAL_FLAGS || {
  echo
  echo "aclocal failed - check that all needed development files are present on system"
  exit 1
}
echo "+ running autoheader ... "
autoheader || {
  echo
  echo "autoheader failed"
  exit 1
}
echo "+ running autoconf ... "
autoconf || {
  echo
  echo "autoconf failed"
  exit 1
}
echo "+ running automake ... "
automake -a -c --foreign || {
  echo
  echo "automake failed"
  exit 1
}

# Chdir back to the builddir before the configure step.
cd $BUILDDIR

# now remove the cache, because it can be considered dangerous in this case
echo "+ removing config.cache ... "
rm -f config.cache

echo
echo "Now type './configure && make' to compile."

exit 0

