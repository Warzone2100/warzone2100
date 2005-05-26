# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([warzone], [1.3], [http://developer.berlios.de/projects/warzone/])
AC_CONFIG_SRCDIR([lib/gamelib/parser.h])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h malloc.h memory.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostname memmove memset sqrt strchr strdup strncasecmp strrchr strstr])
AC_HEADER_STDBOOL
AC_HEADER_TIME

# Force or skip build of OpenGL?
#   --with-opengl will force an opengl build (skip tests)
#   --without-opengl will build without opengl
AC_ARG_WITH(opengl,
[  --with-opengl           use OpenGL to accelerate graphics (default)],
[ opengl=$withval ], [ opengl=maybe ])

AM_PATH_SDL([1.1.4], [sdl_found="yes"], [sdl_found="no"])
if test "$sdl_found" = yes; then
  CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
  CFLAGS="$CFLAGS $SDL_CFLAGS"
  LIBS="$LIBS $SDL_LIBS"
else
  AC_MSG_ERROR([You need to install SDL (http://www.libsdl.org/).])
fi

if test "x$opengl" = xmaybe; then
  AC_CHECK_HEADER(GL/gl.h, GL_h=yes, GL_h=no)
  AC_CHECK_LIB(GL, glGetString, GL_lib=yes, GL_lib=no)
  AC_MSG_CHECKING([OpenGL])
  if test "x$GL_h" = "xno"; then
    if test "x$opengl" = xyes; then
      AC_MSG_ERROR([You need to install OpenGL header files (usually a -dev package).])
    else
      opengl=no
    fi
  else
    opengl=yes
  fi
  if test "x$GL_lib" = "xno"; then
    if test "x$opengl" = xyes; then
      AC_MSG_ERROR([You need to install OpenGL.])
    else
      opengl=no
    fi
  else
    opengl=yes
  fi
fi
if test "x$opengl" = xyes; then
  AC_DEFINE(OPENGL, 1, [OpenGL acceleration])
  LIBS="$LIBS -lGL -ljpeg"
  AC_SUBST(RENDERER, "ivis_opengl")
else
  AC_SUBST(RENDERER, "ivis02")
fi

AC_CHECK_HEADER(AL/al.h, AL_h=yes, AL_h=no)
AC_CHECK_LIB(openal, alGetString, AL_lib=yes, AL_lib=no)
AC_MSG_CHECKING([OpenAL])
if test "x$AL_h" = "xno"; then
  AC_MSG_ERROR([You need to install OpenAL header files (usually a -dev package).])
fi
if test "x$AL_lib" = "xno"; then
  AC_MSG_ERROR([You need to install OpenAL.])
fi
LIBS="$LIBS -lopenal"

# add some required C flags here
# -DYY_STATIC is required by flex
# -m32 forces 32-bit compile, since code is not clean enough for 64-bit yet
CFLAGS="$CFLAGS -Wall -DYY_STATIC -m32"

# windows conditional
case $host_os in
  *mingw32* ) MINGW32=yes;;
          * ) MINGW32=no;;
esac
if test x"$MINGW32" = "xyes"; then
  AC_DEFINE(WIN32, 1, [Windows build])
fi

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/framework/Makefile
                 lib/gamelib/Makefile
                 lib/ivis_opengl/Makefile
                 lib/ivis02/Makefile
                 lib/netplay/Makefile
                 lib/script/Makefile
                 lib/sequence/Makefile
                 lib/sound/Makefile
                 lib/widget/Makefile
                 src/Makefile])
AC_OUTPUT

echo ""
echo "Warzone will compile with the following components:"
echo ""
echo " OpenGL acceleration:   $opengl"
echo " OpenAL sound system:   yes"
echo ""
