# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT([warzone],[2.0.3],[http://gna.org/projects/warzone])
AC_CONFIG_SRCDIR([lib/gamelib/anim.h])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

AC_PROG_YACC
if test "x$YACC" != "xbison -y" ; then
	AC_MSG_ERROR([Bison is not installed])
fi

AC_PROG_LEX
if test "x$LEX" != "flex" ; then
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
  WZ_CFLAGS="${WZ_CFLAGS} -O0 -g3 -Wall"
  AC_DEFINE(DEBUG, 1, [Define if you want extra debugging.])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

#   --without-opengl will build without opengl
AC_ARG_WITH(opengl,
[  --with-opengl           use OpenGL to accelerate graphics [[auto]]],
[ opengl=$withval ], [ opengl=maybe ])

#   --without-openal will build without OpenAL
# Commented out as OpenAL is not curently an option,
# but it will need to become optional someday.
#AC_ARG_WITH(openal,
#[  --with-openal           use OpenAL for sound [[auto]]],
#[ openal=$withval ], [ openal=maybe ])

#   --without-ogg will build without Ogg vorbis support
AC_ARG_WITH(ogg,
[  --with-ogg              built support for Ogg files [[auto]]],
[ ogg=$withval ], [ ogg=maybe ])

#   --without-mp3 will build without MP3 support
AC_ARG_WITH(mp3,
[  --with-mp3              built support for Mp3 files [[auto]]],
[ mp3=$withval ], [ mp3=maybe ])

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
            test "x$prefix" = xNONE && prefix="$ac_default_prefix"
            test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
            eval echo \""[$]$1"\"
        )`
])
dnl export where the datadir is going to be installed; we will look for
dnl $datadir/warzone/warzone.wz
WZ_EXPAND_DIR(datadir, "$datadir/warzone")
AC_DEFINE_UNQUOTED(DEFAULT_DATA_DIR, "$datadir", [Warzone data path])


# Checks for libraries:

AC_CHECK_LIB(m, pow)

AC_CHECK_LIB(z, gzgets, , AC_MSG_ERROR([zlib not found.]))
AC_CHECK_HEADER(zlib.h, , AC_MSG_ERROR([zlib header not found.]))


# Look for image libs
AC_CHECK_LIB(jpeg, main, , [AC_MSG_ERROR([You need to install the jpeg library.])])
AC_CHECK_LIB(png, main, , [AC_MSG_ERROR([You need to install the png library.])])


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
  AC_CHECK_HEADER(GL/gl.h, GL_h=yes, GL_h=no)
  AC_CHECK_HEADER(GL/glu.h, GLU_h=yes, GLU_h=no)
  AC_CHECK_LIB(GL, main, GLLIB=GL GL_lib=yes,
    [AC_CHECK_LIB(opengl32, main, GLLIB=opengl32 GL_lib=yes, GL_lib=no)])
  AC_CHECK_LIB(GLU, main, GLULIB=GLU GLU_lib=yes,
    [AC_CHECK_LIB(glu32, main, GLULIB=glu32 GLU_lib=yes, GLU_lib=no)])
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
	WZ_LIBS="${WZ_LIBS} -l${GLLIB} -l${GLULIB}"
	AC_SUBST(RENDERER, "ivis_opengl")
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_SUBST(RENDERER, "ivis02")
fi


if test "x$openal" != "xno" ; then
  AC_CHECK_HEADER(AL/al.h, AL_h=yes, AL_h=no)
  AC_CHECK_LIB(openal, main, ALLIB=openal AL_lib=yes,
               [AC_CHECK_LIB(openal32, main, ALLIB=openal32 AL_lib=yes, AL_lib=no)])
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
	WZ_LIBS="${WZ_LIBS} -l${ALLIB}"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([OpenAL is currently mandatory])
fi


if test "x$ogg" != "xno" ; then
  XIPH_PATH_OGG(have_ogg=yes, have_ogg=no)
  XIPH_PATH_VORBIS(have_vorbis=yes, have_vorbis=no)
  if test "x$have_ogg" = "xno" -o "x$have_vorbis" = "xno" ; then
    if "x$ogg" = "xyes" ; then
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


# add some required C flags here
# -DYY_STATIC is required by flex
# -m32 forces 32-bit compile, since code is not clean enough for 64-bit yet
WZ_CFLAGS="${WZ_CFLAGS} -DYY_STATIC -m32 -fno-strict-aliasing"

# Set flex flags. The -olex.yy.c kludge is to ensure that flex
# generates the kind of filename that automake expects.
AC_SUBST(FLEX_FLAGS, "-olex.yy.c")

# only now set the LIBS/CFLAGS/CPPFLAGS lines, if set earlier it will give
# strange error messages when something is wrong
LIBS="${LIBS} ${WZ_LIBS}"
CFLAGS="${CFLAGS} ${WZ_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${WZ_CPPFLAGS}"

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/framework/Makefile
                 lib/gamelib/Makefile
                 lib/ivis_opengl/Makefile
                 lib/ivis02/Makefile
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
echo "Warzone will compile with the following components:"
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
