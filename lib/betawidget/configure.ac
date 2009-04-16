#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(betawidget, VERSION)

AC_CONFIG_MACRO_DIR([m4])

# libtool shared library version

# Increment if the interface has additions, changes, removals.
LIB_CURRENT=0

# Increment any time the source changes; set to
# 0 if you increment CURRENT
LIB_REVISION=0

# Increment if any interfaces have been added; set to 0
# if any interfaces have been removed. removal has
# precedence over adding, so set to 0 if both happened.
LIB_AGE=0

AC_SUBST([LIBTOOL_VERSION], ["$LIB_CURRENT:$LIB_REVISION:$LIB_AGE"])

AC_CONFIG_SRCDIR([src/vector.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Make cross compilation options available in --help
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_SWIG
SWIG_ENABLE_CXX
SWIG_LUA
# Objective C is used when compiling for OSX (platform/sdl/clipboardOSX.m)
AC_PROG_OBJC

# By default don't build shared libraries
AC_DISABLE_SHARED

AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

PKG_PROG_PKG_CONFIG

# pkg-config hooks:

if test "x$PKGCONFIG_PREFIX" != "x" ; then
	PKG_CONFIG="${PKG_CONFIG} --define-variable=prefix=${PKGCONFIG_PREFIX}"
fi

if test "x$enable_static" = "xyes" ; then
	# Tell pkg-config to link against static libraries, incl. all dependencies
	PKG_CONFIG="${PKG_CONFIG} --static"
fi

# Check for operating system
case ${host_os} in
	*mingw32*)
		host_os_mingw32=yes
		;;
	*darwin*)
		host_os_macosx=yes
		;;
	*)
		;;
esac
AM_CONDITIONAL([MINGW32], test "x$host_os_mingw32" = "xyes")
AM_CONDITIONAL([MACOSX], test "x$host_os_macosx" = "xyes")

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

# Checks for libraries.
PKG_CHECK_MODULES([SDL], [sdl >= 1.2])
AC_SUBST([PLATFORM_REQUIRES], [sdl])

PKG_CHECK_MODULES([CAIRO], [cairo])
PKG_CHECK_MODULES([SVG_CAIRO], [libsvg-cairo])
PKG_CHECK_MODULES([PANGO_CAIRO], [pangocairo])

# First check the pkg-config lua5.1 package
PKG_CHECK_MODULES([LUA], [lua5.1 >= 5.1], [
	AC_SUBST([LUA_REQUIRES], [lua5.1])
],[
	# If that fails, check the "lua" pkg-config package
	PKG_CHECK_MODULES([LUA], [lua >= 5.1], [
		AC_SUBST([LUA_REQUIRES], [lua])
	])
])

# Look for OpenGL
AC_CHECK_HEADER(SDL/SDL_opengl.h, ,
	AC_MSG_ERROR([SDL OpenGL header not found. Please install SDL with OpenGL support.]))
AC_CHECK_LIB(GL, main,
	OPENGL_LIBS="-lGL",
	AC_CHECK_LIB(opengl32, main,
		OPENGL_LIBS="-lopengl32",
		AC_MSG_ERROR([OpenGL library not found.])))
AC_CHECK_LIB(GLU, main,
	OPENGL_LIBS="${OPENGL_LIBS} -lGLU",
	AC_CHECK_LIB(glu32, main,
		OPENGL_LIBS="${OPENGL_LIBS} -lglu32",
		AC_MSG_ERROR([GLU library not found.])))
AC_SUBST([OPENGL_LIBS], [${OPENGL_LIBS}])

CFLAGS="${CFLAGS} -Werror-implicit-function-declaration"
AC_SUBST([CFLAGS])

AC_CONFIG_FILES([Makefile
                 src/betawidget.pc
                 src/Makefile
                 src/platform/Makefile
                 src/platform/sdl/Makefile
                 tools/Makefile])
AC_OUTPUT
