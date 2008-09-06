#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([vector.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Make cross compilation options available in --help
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG

# pkg-config hooks:

if test "x$PKGCONFIG_PREFIX" != "x" ; then
	PKG_CONFIG="${PKG_CONFIG} --define-variable=prefix=${PKGCONFIG_PREFIX}"
fi

AC_ARG_ENABLE([static],
	AS_HELP_STRING([--enable-static], [Link statically [no]]),
	[ enable_static=${enableval} ], [ enable_static=no ])
if test "x$enable_static" = "xyes" ; then
	# Tell pkg-config to link against static libraries, incl. all dependencies
	PKG_CONFIG="${PKG_CONFIG} --static"
fi

# Checks for libraries.
PKG_CHECK_MODULES([SDL], [sdl >= 1.2])
PKG_CHECK_MODULES([CAIRO_SVG], [cairo-svg])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove strdup])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
