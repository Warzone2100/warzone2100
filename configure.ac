# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT([Warzone 2100],[TRUNK],[http://wz2100.net/],[warzone2100])
AM_INIT_AUTOMAKE([1.8])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/framework/frame.c])

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


# Check for Windows.
AC_CANONICAL_HOST
AM_CONDITIONAL([MINGW32], [test x$host_os = xmingw32])
if test x$host_os = xmingw32 ; then
		AC_DEFINE([WIN32], 1, [Windows build])
		AC_CHECK_TOOL([WINDRES], [windres], [no])
		if test x$WINDRES = xno ; then
			AC_MSG_ERROR([windres not found])
		fi
		WIN32_LIBS="-L\$(srcdir) -ldbghelp -lshfolder -lwinmm -lwsock32"
fi


# Installation directories

AC_ARG_WITH(icondir,
[AS_HELP_STRING([--with-icondir=DIR],[icon files [DATADIR/icons]])],
[ icondir=$withval ], [ icondir=$datadir/icons ])


AC_ARG_WITH(applicationdir,
[AS_HELP_STRING([--with-applicationdir=DIR],[application files [DATADIR/applications]])],
[ applicationdir=$withval ], [ applicationdir=$datadir/applications ])

AC_SUBST(icondir, $icondir)
AC_SUBST(applicationdir, $applicationdir)
if test "x$docdir" = "x" ; then
	AC_SUBST(docdir, $datarootdir/doc/$AC_PACKAGE_TARNAME)
fi


# Commandline options:

AC_ARG_WITH(makensis,
[AS_HELP_STRING([--with-makensis],[Path to makensis])],
[ makensis=$withval ], [ makensis=auto ])


AC_ARG_WITH(installer_libdir,
[AS_HELP_STRING([--with-installer-libdir],[Path to libraries for NSIS script])],
[ installer_libdir=$withval ], [ installer_libdir=auto ])


AC_ARG_WITH(installer_version,
[AS_HELP_STRING([--with-installer-version],[Version of the installer. Workaround till we have some automagic.])],
[ installer_version=$withval ], [ installer_version=auto ])


AC_ARG_ENABLE(data,
[AS_HELP_STRING([--disable-data],[Do not build data packages])],
[ data="$enableval" ], [ data=no ])
AM_CONDITIONAL(DATA, test "x$data" = "xyes" )
AC_MSG_CHECKING([whether to build data packages])
AC_MSG_RESULT([$data])


AC_ARG_ENABLE(installer,
[AS_HELP_STRING([--enable-installer],[Build NSIS installer])],
[ installer=$enableval ], [ installer=no ])
AM_CONDITIONAL(INSTALLER, test "x$installer" = "xyes" )
AC_MSG_CHECKING([whether to build NSIS installer])
AC_MSG_RESULT([$installer])

if test "x$installer" = "xyes" ; then
	if test "x$makensis" = "xauto" ; then
		AC_CHECK_TOOL([MAKENSIS], [makensis], [no])
		if test "x$MAKENSIS" = "xno" ; then
			AC_MSG_ERROR([makensis not found])
		fi
	else
		AC_MSG_CHECKING([for makensis])
		AC_MSG_RESULT([$makensis])
		AC_SUBST(MAKENSIS, $makensis)
	fi
	if test "x$installer_version" = "xauto" ; then
		AC_MSG_ERROR([installer version invalid])
	else
		AC_MSG_CHECKING([for installer version])
		AC_MSG_RESULT([$installer_version])
		AC_SUBST(INSTALLER_VERSION, $installer_version)
	fi
	if test "x$installer_libdir" = "xauto" ; then
		AC_MSG_ERROR([installer libdir not found])
	else
		AC_MSG_CHECKING([for installer libdir])
		AC_MSG_RESULT([$installer_libdir])
		AC_SUBST(INSTALLER_LIBDIR, $installer_libdir)
	fi
fi


# -O0 turns off all optimizations; this is necessary for accurate debugging
# -Wno-unused-label is necessary because flex produces unused labels that we cannot fix
# Add later for stricter checking: -Wextra -Wmissing-declarations -Wstrict-prototypes
AC_ARG_ENABLE(debug,
[AS_HELP_STRING([--enable-debug[=strict/yes/relaxed/no]],[Compile debug version [no]])],
[ debug=$enableval ], [ debug=no ])
AC_MSG_CHECKING([whether to compile in debug mode])
AC_MSG_RESULT([$debug])
if test "x$debug" = "xyes" ; then
  WZ_CFLAGS="${WZ_CFLAGS} -g -O0 -DDEBUG -Wall -Wno-unused-label"
elif test "x$debug" = "xstrict" ; then
  WZ_CFLAGS="${WZ_CFLAGS} -g -O0 -DDEBUG -Wall -Wno-unused-label -Werror"
elif test "x$debug" = "xrelaxed" ; then
  WZ_CFLAGS="${WZ_CFLAGS} -g -O2 -DDEBUG -Wall -Wno-unused-label -Wextra"
else
  WZ_CFLAGS="${WZ_CFLAGS} -g -O2 -DNDEBUG"
fi


#   --disable-ogg will build without Ogg Vorbis support
AC_ARG_ENABLE(ogg,
[AS_HELP_STRING([--disable-ogg],[Do not built support for Ogg files])],
[ ogg=$enableval ], [ ogg=maybe ])

#   --disable-mp3 will build without Mp3 support
AC_ARG_ENABLE(mp3,
[AS_HELP_STRING([--disable-mp3],[Do not built support for Mp3 files])],
[ mp3=$enableval ], [ mp3=maybe ])



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

# add some required C flags here
# -DYY_STATIC is required by flex
WZ_CFLAGS="${WZ_CFLAGS} -DYY_STATIC -DDEFAULT_DATADIR=\\\"${datadir}/warzone2100\\\""

# only now set the LIBS/CFLAGS/CPPFLAGS lines, if set earlier it will give
# strange error messages when something is wrong
LIBS="${LIBS} ${WZ_LIBS} ${WIN32_LIBS}"
CFLAGS="${CFLAGS} ${WZ_CFLAGS}"

AC_CONFIG_HEADER([lib/framework/config.h])
AC_CONFIG_FILES([Makefile
		data/Makefile
		nsis/Makefile
		win32/Makefile
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
