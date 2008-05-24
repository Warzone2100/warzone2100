# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.56])
AC_INIT([Warzone 2100],[TRUNK],[http://wz2100.net/],[warzone2100])
AM_INIT_AUTOMAKE([1.10])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/framework/frame.c])


# HACK
# Workaround stupid autoconf CFLAGS default
# (Bash considers set but empty variables different from null for the ${VAR:+alt} operator)
CFLAGS="${CFLAGS}"


# Checks for programs.
AC_PROG_CC_STDC
AC_PROG_RANLIB

# We need a C++ compiler for build_tools/autorevision
AC_PROG_CXX

AC_PROG_YACC
if test "x$YACC" != "xbison -y" ; then
	AC_MSG_ERROR([Bison is not installed])
fi

AC_PROG_LEX
if test "x$LEX" != "xflex" ; then
	AC_MSG_ERROR([Flex is not installed])
fi

# Enable Compiler/LibC GNU extensions
AC_GNU_SOURCE

# Check for gettext
AM_GNU_GETTEXT([external])

# Check for pkg-config
PKG_PROG_PKG_CONFIG


# Check for compiler
case ${CC} in
	icc)
		cc_icc=yes
		;;
esac


# Check for operating system
case ${host_os} in
	*mingw32*)
		host_os_mingw32=yes
		AC_CHECK_TOOL([WINDRES], [windres], AC_MSG_ERROR([windres not found]))
		WIN32_LIBS='-ldbghelp -lshfolder -lwinmm -lwsock32'
		AC_SUBST([WIN32_LIBS], [${WIN32_LIBS}])
		;;
	*)
		AC_DEFINE([_XOPEN_SOURCE], 600, [Enable POSIX extensions if present])
		;;
esac
AM_CONDITIONAL([MINGW32], test "x$host_os_mingw32" = "xyes")


# Set compilers to use when building for the build system in a cross compile build
# Fallback to the system compilers
if test "x$CXX_FOR_BUILD" = "x" ; then
    CXX_FOR_BUILD="${CXX}"
fi
AC_SUBST([CXX_FOR_BUILD])

if test "x$CC_FOR_BUILD" = "x" ; then
    CC_FOR_BUILD="${CC}"
fi
AC_SUBST([CC_FOR_BUILD])

if test "x$CPPFLAGS_FOR_BUILD" = "x" ; then
    CPPFLAGS_FOR_BUILD="${CPPFLAGS}"
fi
AC_SUBST([CPPFLAGS_FOR_BUILD])


# pkg-config hooks:

if test "x$PKGCONFIG_PREFIX" != "x" ; then
	PKG_CONFIG="${PKG_CONFIG} --define-variable=prefix=${PKGCONFIG_PREFIX}"
fi


# Variable fixes for older autoconf versions:

if test "x$docdir" = "x" ; then
	AC_SUBST([docdir], [${datarootdir}/doc/${AC_PACKAGE_TARNAME}])
fi
if test "x$localedir" = "x" ; then
	AC_SUBST([localedir], [${datadir}/locale])
fi


# Installation directories

AC_ARG_WITH(icondir,
	AS_HELP_STRING([--with-icondir=DIR],[icon files [DATADIR/icons]]),
	[ with_icondir=${withval} ], [ with_icondir=${datadir}/icons ])
AC_SUBST([icondir], [${with_icondir}])

AC_ARG_WITH(applicationdir,
	AS_HELP_STRING([--with-applicationdir=DIR],[application files [DATADIR/applications]]),
	[ with_applicationdir=${withval} ], [ with_applicationdir=${datadir}/applications ])
AC_SUBST([applicationdir], [${with_applicationdir}])


# Commandline options - enable:

AC_ARG_ENABLE([installer],
	AS_HELP_STRING([--enable-installer], [Build NSIS installer [no]]),
	[ enable_installer=${enableval} ], [ enable_installer=no ])
AM_CONDITIONAL(INSTALLER, test "x$enable_installer" = "xyes")
AC_MSG_CHECKING([whether to build NSIS installer])
AC_MSG_RESULT([${enable_installer}])

AC_ARG_ENABLE([static],
	AS_HELP_STRING([--enable-static], [Link statically [no]]),
	[ enable_static=${enableval} ], [ enable_static=no ])
if test "x$enable_static" = "xyes" ; then
	# Add libs where even pkg-config cannot help statically linking them,
	# where one would otherwise have to provide them via commandline
	WIN32_LIBS="${WIN32_LIBS} -lz -lintl -liconv"
	# Tell pkg-config to link against static libraries, incl. all dependencies
	PKG_CONFIG="${PKG_CONFIG} --static"
fi

# -O0 turns off all optimizations; this is necessary for accurate debugging
# -Wno-unused-label is necessary because flex produces unused labels that we cannot fix
# Add later for stricter checking: -Wextra -Wmissing-declarations -Wstrict-prototypes
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug[=yes/relaxed/profile/debugprofile/no]],[Compile debug version [yes]]),
	[ enable_debug=${enableval} ], [ enable_debug=yes ])
AC_MSG_CHECKING([whether to compile in debug mode])
AC_MSG_RESULT([${enable_debug}])

if test "x$enable_debug" = "xyes" ; then
	if test "x$cc_icc" = "xyes" ; then
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -Wcheck -Werror"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	else
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -Wall -Werror -Wno-unused-label -Wno-pointer-to-int-cast -Wmissing-field-initializers -Wcast-align -Wwrite-strings"
		WZ_CFLAGS="${WZ_CFLAGS} -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	fi
elif test "x$enable_debug" = "xrelaxed" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -O0 -Wall -Wextra -Wwrite-strings"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
elif test "x$enable_debug" = "xdebugprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -O0 -pg -Wall -Wextra -Wwrite-strings"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
elif test "x$enable_debug" = "xprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -Os -pg -Wall -Wextra -Wwrite-strings"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DNDEBUG"
else
	WZ_CFLAGS="${WZ_CFLAGS} -g -Wall -Wwrite-strings"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DNDEBUG"
fi


# Commandline options - with:

AC_ARG_WITH([distributor],
	AS_HELP_STRING([--with-distributor], [Name of distributor compiling this package]),
	[ with_distributor=${withval} ], [ with_applicationdir="UNKNOWN" ])
AC_DEFINE_UNQUOTED([PACKAGE_DISTRIBUTOR], ["${with_distributor}"], [Name of distributor compiling this package])

AC_ARG_WITH([makensis],
	AS_HELP_STRING([--with-makensis], [Path to makensis]))

AC_ARG_WITH([installer_compression],
	AS_HELP_STRING([--with-installer-compression], [Compression method to use when building NSIS [lzma]]),
	[ with_installer_compression=${withval} ], [ with_installer_compression="lzma" ])
AC_SUBST([INSTALLER_COMPRESSION], [${with_installer_compression}])

AC_ARG_WITH([installer_extdir],
	AS_HELP_STRING([--with-installer-extdir], [Path to external data for NSIS script]))

AC_ARG_WITH([installer_version],
	AS_HELP_STRING([--with-installer-version], [4-tuble version of the installer. i.e. 1.2.3.4]))

if test "x$enable_installer" = "xyes" ; then
	if test "x$with_makensis" = "x" ; then
		AC_CHECK_TOOL([MAKENSIS], [makensis], [no])
		if test "x$MAKENSIS" = "xno" ; then
			AC_MSG_ERROR([makensis not found])
		fi
	else
		AC_MSG_CHECKING([for makensis])
		AC_MSG_RESULT([${with_makensis}])
		AC_SUBST([MAKENSIS], [${with_makensis}])
	fi
	# Doing that check here, because without enable-installer it is not an error
	if test "x$with_installer_version" = "x" ; then
		AC_MSG_ERROR([installer version invalid])
	else
		AC_MSG_CHECKING([for installer version])
		AC_MSG_RESULT([${with_installer_version}])
		AC_SUBST([INSTALLER_VERSION], [${with_installer_version}])
	fi
	# Doing that check here, because without enable-installer it is not an error
	if test "x$with_installer_extdir" = "x" ; then
		AC_MSG_ERROR([installer external dir not found])
	else
		AC_MSG_CHECKING([for installer extdir])
		AC_MSG_RESULT([${with_installer_extdir}])
		AC_SUBST([INSTALLER_EXTDIR], [${with_installer_extdir}])
	fi
fi


# Checks for modules:

PKG_CHECK_MODULES([SDL], [sdl >= 1.2])
PKG_CHECK_MODULES([OPENAL], [openal >= 0.0.8])
PKG_CHECK_MODULES([PNG], [libpng >= 1.2])
PKG_CHECK_MODULES([OGGVORBIS], [vorbisfile >= 1.1])

if test "x$enable_static" = "xyes" ; then
	# If linking statically, make sure to include the direct dependencies of
	# QuesoGLC as well, because it does not have a pkg-config file.
	# FIXME Remove this whenever QuesoGLC has a pkg-config file
	PKG_CHECK_MODULES([FONTCONFIG], [fontconfig])
	PKG_CHECK_MODULES([FREETYPE], [freetype2])
fi


# Checks for libraries:

# Look for Popt
AC_CHECK_HEADER(popt.h, , AC_MSG_ERROR([Popt header not found.]))
AC_CHECK_LIB(popt, poptGetContext, AC_SUBST([POPT_LIBS], [-lpopt]), AC_MSG_ERROR([Popt not found.]), [${WIN32_LIBS}])

# Look for PhysicsFS
AC_CHECK_HEADER(physfs.h, , AC_MSG_ERROR([PhysicsFS header not found.]))
AC_CHECK_LIB(physfs, PHYSFS_init, AC_SUBST([PHYSFS_LIBS], [-lphysfs]), AC_MSG_ERROR([PhysicsFS not found.]), [${WIN32_LIBS}])

# Look for SDL_net
AC_CHECK_HEADER(SDL/SDL_net.h, , AC_MSG_ERROR([SDL_net header not found.]))
AC_CHECK_LIB(SDL_net, SDLNet_Init, AC_SUBST([SDL_NET_LIBS], [-lSDL_net]), AC_MSG_ERROR([SDL_net not found.]), [${SDL_LIBS} ${WIN32_LIBS}])

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

# Look for OpenGLC
AC_CHECK_HEADER(GL/glc.h, ,
	AC_MSG_ERROR([OpenGLC header not found. Please install QuesoGLC: http://quesoglc.sourceforge.net/]))
AC_CHECK_LIB(GLC, main,
	OPENGLC_LIBS="-lGLC",
	AC_CHECK_LIB(glc32, main,
		OPENGLC_LIBS="-lglc32",
		AC_MSG_ERROR([OpenGLC library not found. Please install QuesoGLC: http://quesoglc.sourceforge.net/])))
if test "x$enable_static" = "xyes" ; then
	# If linking statically, make sure to link against the direct dependencies, too.
	# FIXME Remove this whenever QuesoGLC has a pkg-config file
	OPENGLC_LIBS="${OPENGLC_LIBS} ${FONTCONFIG_LIBS} ${FREETYPE_LIBS}"
fi
AC_SUBST([OPENGLC_LIBS], [${OPENGLC_LIBS}])

# When (cross-)compiling for Windows (MinGW) we need to link in BFD and iberty
# for the Dr. MinGW derived exception handler.
if test "x$host_os_mingw32" = "xyes" ; then
    AC_CHECK_LIB(iberty, main, AC_SUBST([IBERTY_LIBS], [-liberty]), AC_MSG_ERROR([libiberty not found.]))

    AC_CHECK_HEADER(bfd.h, , AC_MSG_ERROR([BFD header not found.]))
    AC_CHECK_LIB(bfd, bfd_openr, AC_SUBST([BFD_LIBS], [-lbfd]), AC_MSG_ERROR([BFD not found.]), [${IBERTY_LIBS}])

    WIN32_LIBS="${WIN32_LIBS} ${BFD_LIBS} ${IBERTY_LIBS} -lstdc++"
fi

WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDATADIR=\"\\\"\${datadir}/\${PACKAGE}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -DLOCALEDIR=\"\\\"\${localedir}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -I\$(top_srcdir)"


AC_SUBST([WZ_CPPFLAGS], [${WZ_CPPFLAGS}])
AC_SUBST([WZ_CFLAGS], [${WZ_CFLAGS}])


AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile
		po/Makefile.in
		doc/Makefile
		build_tools/Makefile
		build_tools/autorevision/Makefile
		icons/Makefile
		data/Makefile
		data/mods/Makefile
		data/mods/global/Makefile
		data/mods/global/aivolution/multiplay/skirmish/Makefile
		pkg/Makefile
		pkg/nsis/Makefile
		win32/Makefile
		lib/Makefile
		lib/framework/Makefile
		lib/exceptionhandler/Makefile
		lib/gamelib/Makefile
		lib/ivis_opengl/Makefile
		lib/ivis_common/Makefile
		lib/netplay/Makefile
		lib/script/Makefile
		lib/sequence/Makefile
		lib/sound/Makefile
		lib/sqlite3/Makefile
		lib/widget/Makefile
		src/Makefile])
AC_OUTPUT

if test "x$enable_debug" = "xyes" ; then
	AC_MSG_NOTICE()
	AC_MSG_NOTICE([*** Running in debug mode! ***])
fi

if test "x$with_distributor" = "x" ; then
	AC_MSG_NOTICE()
	AC_MSG_WARN([Distributor unknown!])
	AC_MSG_NOTICE([Use --with-distributor when compiling package for distribution])
fi
