# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT([Warzone 2100],[2.0.4_rc2],[http://wz.rootzilla.de/])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([lib/gamelib/anim.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

AC_PROG_YACC
if test "x$YACC" != "xbison -y" ; then
	AC_MSG_ERROR([Bison is not installed])
fi

AM_PROG_LEX
if test "x$LEX" != "xflex" ; then
	AC_MSG_ERROR([Flex is not installed])
fi


# Commandline options:

AC_ARG_ENABLE(make_data,
[  --disable-make-data     do not recurse make into data directories
                          (ok to disable unless you will 'make install')],
[ make_data="$enableval" ], [make_data=yes])
AC_MSG_CHECKING([whether to make data])
AM_CONDITIONAL(MAKE_DATA, test "x$make_data" = "xyes" )
if test "x$make_data" = "xyes" ; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi


AC_ARG_ENABLE(debug,
[  --enable-debug          run in debug mode],
[ debug=$enableval ], [ debug=no ])
AC_MSG_CHECKING([whether to compile in debug mode])
if test "x$debug" = "xyes" ; then
  # -O0 turns off all optimizations; this is necessary for accurate debugging
  WZ_CFLAGS="${WZ_CFLAGS} -g3 -O0 -Wall -DDEBUG"
  AC_MSG_RESULT([yes])
else
  WZ_CFLAGS="${WZ_CFLAGS} -DNDEBUG"
  AC_MSG_RESULT([no])
fi

#   --disable-ogg will build without Ogg Vorbis support
AC_ARG_ENABLE(ogg,
[AC_HELP_STRING([--disable-ogg],[Do not built support for Ogg files])],
[ ogg=$enableval ], [ ogg=maybe ])

#   --disable-mp3 will build without Mp3 support
AC_ARG_ENABLE(mp3,
[AC_HELP_STRING([--disable-mp3],[Do not built support for Mp3 files])],
[ mp3=$enableval ], [ mp3=maybe ])

# windows conditional
case $host_os in
  *mingw32* )
        AC_DEFINE(WIN32, 1, [Windows build])
        WZ_LIBS="${WZ_LIBS} -lmingw32"
        ;;
          * )
        ;;
esac

dnl WZ_EXPAND_DIR(VARNAME, DIR)
dnl expands occurrences of ${prefix} and ${exec_prefix} in the given DIR,
dnl and assigns the resulting string to VARNAME
dnl example: WZ_EXPAND_DIR(LOCALEDIR, "$datadir/locale")
dnl eg, then: AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR")
dnl by Alexandre Oliva
dnl from http://www.cygnus.com/ml/automake/1998-Aug/0040.html
AC_DEFUN([WZ_EXPAND_DIR], [
        $1=$2
        $1=`(
            test "x$prefix" = "xNONE" && prefix="${ac_default_prefix}"
            test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"
            eval echo \""[$]$1"\"
        )`
])
dnl export where the datadir is going to be installed; we will look for
dnl $datadir/warzone2100/warzone.wz
WZ_EXPAND_DIR(datadir, "$datadir/warzone2100")
WZ_CFLAGS="${WZ_CFLAGS} -DDEFAULT_DATADIR=\\\"${datadir}\\\""


# Checks for libraries:

AC_CHECK_LIB(m, pow)

AC_CHECK_LIB(z, gzgets, , AC_MSG_ERROR([zlib not found.]))
AC_CHECK_HEADER(zlib.h, , AC_MSG_ERROR([zlib header not found.]))


# Look for image libs
AC_CHECK_LIB(jpeg, main, , AC_MSG_ERROR([libjpeg not found.]))
AC_CHECK_HEADER(jpeglib.h, , AC_MSG_ERROR([jpeg header not found.]))
AC_CHECK_LIB(png, main, , AC_MSG_ERROR([libpng not found.]))
AC_CHECK_HEADER(png.h, , AC_MSG_ERROR([png header not found.]))


# Look for PhysicsFS
AC_CHECK_LIB(physfs, PHYSFS_init, , AC_MSG_ERROR([PhysicsFS not found]))
AC_CHECK_HEADER(physfs.h, , AC_MSG_ERROR([PhysicsFS header not found]))


AM_PATH_SDL([1.1.4],[
  WZ_CFLAGS="${WZ_CFLAGS} ${SDL_CFLAGS}" WZ_LIBS="${WZ_LIBS} ${SDL_LIBS}"],
  AC_MSG_ERROR([SDL is not installed]))

AC_PATH_SDLNET([
  WZ_CFLAGS="${WZ_CFLAGS} ${SDLNETINC}" WZ_LIBS="${WZ_LIBS} ${SDLNETLIB}"],
  AC_MSG_ERROR([SDL_net is not installed]))


if test "x$opengl" != "xno" ; then
  if test `uname` = "Darwin"; then
    AC_MSG_CHECKING([for OpenGL framework])
    wz_ac_check_opengl_save_LIBS=$LIBS
    LIBS="-framework OpenGL $LIBS"
    AC_LINK_IFELSE([main() { }],[GL_h=mac ; GLU_h=mac ; GL_lib=yes ; GLU_lib=yes ; GLLIB="-framework OpenGL" ; GLULIB="" ; AC_MSG_RESULT([yes])],LIBS=$wz_ac_check_opengl_save_LIBS ; [AC_MSG_RESULT([no])])
  fi

  if test "x$GL_lib" != "xyes" ; then
    AC_CHECK_HEADER(SDL/SDL_opengl.h, GL_h=yes, GL_h=no)
    AC_CHECK_LIB(GL, main, GLLIB=-lGL GL_lib=yes,
      [AC_CHECK_LIB(opengl32, main, GLLIB=-lopengl32 GL_lib=yes, GL_lib=no)])
    AC_CHECK_LIB(GLU, main, GLULIB=-lGLU GLU_lib=yes,
      [AC_CHECK_LIB(glu32, main, GLULIB=-lglu32 GLU_lib=yes, GLU_lib=no)])
  fi

  if test "x$GL_lib" = "xno" -o "x$GLU_lib" = "xno" ; then
    if test "x$opengl" = "xyes" ; then
      AC_MSG_ERROR([You need to install OpenGL and GLU.])
    else
      opengl=no
    fi
  elif test "x$GL_h" = "xno" -o "x$GLU_h" = "xno" ; then
    if test "x$opengl" = "xyes" ; then
      AC_MSG_ERROR([You need to install OpenGL and GLU header files (usually a -dev package).])
    else
      opengl=no
    fi
  else
    opengl=yes
  fi
fi

AC_MSG_CHECKING([OpenGL])
if test "x$opengl" = "xyes" ; then
	AC_DEFINE(WZ_OPENGL, 1, [OpenGL acceleration])
	WZ_LIBS="${WZ_LIBS} ${GLLIB} ${GLULIB}"
	AC_SUBST(RENDERER, "ivis_opengl")
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([OpenGL is currently mandatory])
fi

if test "x$openal" != "xno" ; then
  if test `uname` = "Darwin"; then
    AC_MSG_CHECKING([for OpenAL framework])
    wz_ac_check_openal_save_LIBS=$LIBS
    LIBS="-framework OpenAL $LIBS"
    AC_LINK_IFELSE([main() { }],[AL_h=mac ; AL_lib=yes ; ALLIB="-framework OpenAL" ; AC_MSG_RESULT([yes])],[LIBS=$wz_ac_check_openal_save_LIBS ; AC_MSG_RESULT([no])])
  fi

  if test "x$AL_lib" != "xyes" ; then
    AC_CHECK_HEADER(AL/al.h, AL_h=standard,
                 [AC_CHECK_HEADER(openal/al.h, AL_h=mac, AL_h=no)])
    AC_CHECK_LIB(openal, main, ALLIB=-lopenal AL_lib=yes,
                 [AC_CHECK_LIB(openal32, main, ALLIB=-lopenal32 AL_lib=yes, AL_lib=no)])
  fi

  if test "x$AL_lib" = "xno" ; then
    if test "x$openal" = "xyes" ; then
      AC_MSG_ERROR([You need to install OpenAL.])
    else
      openal=no
    fi
  elif test "x$AL_h" = "xno" ; then
      if test "x$openal" = "xyes" ; then
        AC_MSG_ERROR([You need to install OpenAL header files (usually a -dev package).])
      else
        openal=no
      fi
  else
    openal=yes
  fi
fi

AC_MSG_CHECKING([OpenAL])
if test "x$openal" = "xyes" ; then
	AC_DEFINE(WZ_OPENAL, 1, [OpenAL support])
	WZ_LIBS="${WZ_LIBS} ${ALLIB}"
	AC_MSG_RESULT([yes])
	if test "x$AL_h" = "xmac"; then
		AC_DEFINE(WZ_OPENAL_MAC_H, 1, [OpenAL MacOSX header support])
	fi
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([OpenAL is currently mandatory])
fi


if test "x$ogg" != "xno" ; then
  XIPH_PATH_OGG(have_ogg=yes, have_ogg=no)
  XIPH_PATH_VORBIS(have_vorbis=yes, have_vorbis=no)
  if test "x$have_ogg" = "xno" -o "x$have_vorbis" = "xno" ; then
    if test "x$ogg" = "xyes" ; then
      AC_MSG_ERROR([You need to install OGG/Vorbis.])
    else
      ogg=no
      AC_DEFINE(WZ_NOOGG, 1, [Disable ogg/vorbis support])
    fi
  else
    ogg=yes
    WZ_LIBS="${WZ_LIBS} -lvorbisfile -lvorbis -logg"
  fi
else
  AC_DEFINE(WZ_NOOGG, 1, [Disable ogg/vorbis support])
fi


if test "x$mp3" != "xno" ; then
  AC_CHECK_LIB(mad, mad_stream_init,
    [AC_CHECK_HEADER(mad.h, have_mad=yes, have_mad=no)],
    [have_mad=no])
  if test "x$have_mad" = "xno" ; then
    if test "x$mp3" = "xyes" ; then
      AC_MSG_ERROR([You need to install libMAD.])
    else
      mp3=no
	  AC_DEFINE(WZ_NOMP3, 1, [Disable mp3 support])
    fi
  else
    mp3=yes
	WZ_LIBS="${WZ_LIBS} -lmad"
  fi
else
  AC_DEFINE(WZ_NOMP3, 1, [Disable mp3 support])
fi


# Set flex flags. The -olex.yy.c kludge is to ensure that flex
# generates the kind of filename that automake expects.
AC_SUBST(LFLAGS, "-olex.yy.c")

# Set bison flags. The -oy.tab.c kludge is to ensure that bison
# generates the kind of filename that automake expects.
AC_SUBST(YFLAGS, "-oy.tab.c")

# add some required C flags here
# -DYY_STATIC is required by flex
# -m32 forces 32-bit compile, since code is not clean enough for 64-bit yet
WZ_CFLAGS="${WZ_CFLAGS} -DYY_STATIC -m32"

# only now set the LIBS/CFLAGS/CPPFLAGS lines, if set earlier it will give
# strange error messages when something is wrong
LIBS="${LIBS} ${WZ_LIBS}"
CFLAGS="${CFLAGS} ${WZ_CFLAGS}"

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/framework/Makefile
                 lib/gamelib/Makefile
                 lib/ivis_opengl/Makefile
                 lib/ivis_common/Makefile
                 lib/netplay/Makefile
                 lib/script/Makefile
                 lib/sequence/Makefile
                 lib/sound/Makefile
                 lib/widget/Makefile
                 data/Makefile
                 src/Makefile])
AC_OUTPUT

echo ""
echo "Warzone 2100 will compile with the following components:"
echo ""
echo " OpenGL acceleration:   $opengl"
echo " OpenAL sound system:   $openal"
echo " MP3 music support:     $mp3"
echo " OGG music support:     $ogg"
echo ""

if test "x$debug" = xyes; then
  echo " *** Running in debug mode! ***"
  echo ""
fi
