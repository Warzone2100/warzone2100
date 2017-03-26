#!/bin/sh
# Run this to generate all the initial makefiles, etc.

DIE=0
SRCDIR=`dirname $0`
BUILDDIR=`pwd`
srcfile=src/action.cpp
package=Warzone2100

debug ()
# print out a debug message if DEBUG is a defined variable
{
  if [ ! -z "$DEBUG" ]; then
    echo "DEBUG: $1"
  fi
}

version_check ()
# check the version of a package
# first argument : complain ('1') or not ('0')
# second argument : package name (executable)
# third argument : source download url
# rest of arguments : major, minor, micro version
{
  COMPLAIN=$1
  PACKAGE=$2
  URL=$3
  MAJOR=$4
  MINOR=$5
  MICRO=$6

  WRONG=

  debug "major $MAJOR minor $MINOR micro $MICRO"
  VERSION=$MAJOR
  if [ ! -z "$MINOR" ]; then VERSION=$VERSION.$MINOR; else MINOR=0; fi
  if [ ! -z "$MICRO" ]; then VERSION=$VERSION.$MICRO; else MICRO=0; fi

  debug "version $VERSION"
  echo "+ checking for $PACKAGE >= $VERSION ... " | tr -d '\n'

  ($PACKAGE --version) < /dev/null > /dev/null 2>&1 ||
  {
    echo
    echo "You must have $PACKAGE installed to compile $package."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at $URL"
    return 1
  }
  # the following line is carefully crafted sed magic
  pkg_version=`$PACKAGE --version|head -n 1|sed 's/([^)]*)//g;s/^[a-zA-Z\.\ \-\/]*//;s/ .*$//'`
  debug "pkg_version $pkg_version"
  pkg_major=`echo $pkg_version | cut -d. -f1`
  pkg_minor=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
  pkg_micro=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
  [ -z "$pkg_minor" ] && pkg_minor=0
  [ -z "$pkg_micro" ] && pkg_micro=0

  debug "found major $pkg_major minor $pkg_minor micro $pkg_micro"

  #start checking the version
  if [ "$pkg_major" -lt "$MAJOR" ]; then
    WRONG=1
  elif [ "$pkg_major" -eq "$MAJOR" ]; then
    if [ "$pkg_minor" -lt "$MINOR" ]; then
      WRONG=1
    elif [ "$pkg_minor" -eq "$MINOR" -a "$pkg_micro" -lt "$MICRO" ]; then
      WRONG=1
    fi
  fi

  if [ ! -z "$WRONG" ]; then
   echo "found $pkg_version, not ok !"
   if [ "$COMPLAIN" -eq "1" ]; then
     echo
     echo "You must have $PACKAGE $VERSION or greater to compile $package."
     echo "Get the latest version from <$URL>."
     echo
   fi
   return 1
  else
    echo "found $pkg_version, ok."
  fi
}

not_version ()
# check the version of a package
# first argument : package name (executable)
# second argument : source download url
# rest of arguments : major, minor, micro version
{
  PACKAGE=$1
  URL=$2
  MAJOR=$3
  MINOR=$4
  MICRO=$5

  WRONG=

  debug "major $MAJOR minor $MINOR micro $MICRO"
  VERSION=$MAJOR
  if [ ! -z "$MINOR" ]; then VERSION=$VERSION.$MINOR; else MINOR=0; fi
  if [ ! -z "$MICRO" ]; then VERSION=$VERSION.$MICRO; else MICRO=0; fi

  debug "version $VERSION"
  echo "+ checking for $PACKAGE != $VERSION ... " | tr -d '\n'

  ($PACKAGE --version) < /dev/null > /dev/null 2>&1 ||
  {
    echo
    echo "You must have $PACKAGE installed to compile $package."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at $URL"
    return 1
  }
  # the following line is carefully crafted sed magic
  pkg_version=`$PACKAGE --version|head -n 1|sed 's/([^)]*)//g;s/^[a-zA-Z\.\ \-\/]*//;s/ .*$//'`
  debug "pkg_version $pkg_version"
  pkg_major=`echo $pkg_version | cut -d. -f1`
  pkg_minor=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
  pkg_micro=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
  [ -z "$pkg_minor" ] && pkg_minor=0
  [ -z "$pkg_micro" ] && pkg_micro=0

  debug "found major $pkg_major minor $pkg_minor micro $pkg_micro"

  #start checking the version
  if [ "$pkg_major" -eq "$MAJOR" ]; then
   if [ "$pkg_minor" -eq "$MINOR" ]; then
    if [ "$pkg_micro" -eq "$MICRO" ]; then
      WRONG=1
    fi
   fi
  fi

  if [ ! -z "$WRONG" ]; then
   echo "found $pkg_version, not ok !"
   echo
   echo "Version $PACKAGE $VERSION is known not to work well with $package."
   echo "Get another (preferably latest) version from <$URL>."
   echo
   return 1
  else
    echo "not found, good."
  fi
}

# Chdir to the srcdir, then run auto* tools.
cd "$SRCDIR"

version_check 1 "autoconf" "ftp://ftp.gnu.org/pub/gnu/autoconf/" 2 56 || DIE=1
version_check 1 "automake" "ftp://ftp.gnu.org/pub/gnu/automake/" 1 11 || DIE=1
if [ "$DIE" -eq 1 ]; then
  exit 1
fi

[ -f "$srcfile" ] || {
  echo "Are you sure $SRCDIR is a valid source directory?"
  exit 1
}

echo "+ running intltoolize ..."
intltoolize --force || {
  echo
  echo "intltoolize failed - check that all needed development files are present on system"
  exit 1
}
echo "+ running aclocal ..."
aclocal -I m4 || {
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
cd "$BUILDDIR"

# now remove the cache, because it can be considered dangerous in this case
echo "+ removing config.cache ... "
rm -f config.cache

echo
echo "Now type '$SRCDIR/configure && make' to compile."

exit 0
