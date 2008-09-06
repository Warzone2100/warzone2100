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
PKG_CHECK_MODULES([CAIRO_SVG], [cairo-svg])

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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
