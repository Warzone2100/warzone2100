# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT([Warzone 2100],[TRUNK],[http://wz2100.net/],[warzone2100])
AM_INIT_AUTOMAKE([1.8])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/framework/frame.c])


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


# Check for pkg-config
PKG_PROG_PKG_CONFIG


# Enable Compiler/LibC GNU extensions
AC_GNU_SOURCE

# Check for gettext
AM_GNU_GETTEXT([external])


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
		WIN32_PRE_LIBS='-lmingw32 -lSDLmain'
		AC_SUBST([WIN32_PRE_LIBS], [${WIN32_PRE_LIBS}])
		WIN32_LIBS='-lz -lintl -liconv -lfreetype -lfontconfig -lexpat -ldbghelp -lshfolder -lgdi32 -lwinmm -lwsock32'
		AC_SUBST([WIN32_LIBS], [${WIN32_LIBS}])
		;;
	*)
		AC_DEFINE([_XOPEN_SOURCE], 600, [Enable POSIX extensions if present])
		;;
esac
AM_CONDITIONAL([MINGW32], test "x$host_os_mingw32" = "xyes")

# Set compilers to use when building for the build system in a cross compile build
if test "x$CXX_FOR_BUILD" = "x" ; then
    CXX_FOR_BUILD=${CXX}
fi
AC_SUBST(CXX_FOR_BUILD)

if test "x$CC_FOR_BUILD" = "x" ; then
    CC_FOR_BUILD=${CC}
fi
AC_SUBST(CC_FOR_BUILD)

# Installation directories

AC_ARG_WITH(icondir,
	AS_HELP_STRING([--with-icondir=DIR],[icon files [DATADIR/icons]]),
	[ with_icondir=${withval} ], [ with_icondir=${datadir}/icons ])


AC_ARG_WITH(applicationdir,
	AS_HELP_STRING([--with-applicationdir=DIR],[application files [DATADIR/applications]]),
	[ with_applicationdir=${withval} ], [ with_applicationdir=${datadir}/applications ])


AC_SUBST([icondir], [${with_icondir}])
AC_SUBST([applicationdir], [${with_applicationdir}])
if test "x$docdir" = "x" ; then
	AC_SUBST([docdir], [${datarootdir}/doc/${AC_PACKAGE_TARNAME}])
fi
if test "x$localedir" = "x" ; then
	AC_SUBST([localedir], [${datadir}/locale])
fi


# Commandline options:

AC_ARG_ENABLE([installer],
	AS_HELP_STRING([--enable-installer],[Build NSIS installer [no]]),
	[ enable_installer=${enableval} ], [ enable_installer=no ])
AM_CONDITIONAL(INSTALLER, test "x$enable_installer" = "xyes")
AC_MSG_CHECKING([whether to build NSIS installer])
AC_MSG_RESULT([${enable_installer}])


AC_ARG_WITH(distributor,
	AS_HELP_STRING([--with-distributor],[Name of distributor compiling this package]))
if test "x$with_distributor" = "x" ; then
	AC_DEFINE([PACKAGE_DISTRIBUTOR], "UNKNOWN", [Name of distributor compiling this package])
else
	AC_DEFINE_UNQUOTED([PACKAGE_DISTRIBUTOR], ["${with_distributor}"], [Name of distributor compiling this package])
fi


AC_ARG_WITH(makensis,
	AS_HELP_STRING([--with-makensis],[Path to makensis]))


AC_ARG_WITH(installer_libdir,
	AS_HELP_STRING([--with-installer-extdir],[Path to external data for NSIS script]))


AC_ARG_WITH(installer_version,
	AS_HELP_STRING([--with-installer-version],[Version of the installer. i.e. 1.2.3.4]))


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
	if test "x$with_installer_version" = "x" ; then
		AC_MSG_ERROR([installer version invalid])
	else
		AC_MSG_CHECKING([for installer version])
		AC_MSG_RESULT([${with_installer_version}])
		AC_SUBST([INSTALLER_VERSION], [${with_installer_version}])
	fi
	if test "x$with_installer_extdir" = "x" ; then
		AC_MSG_ERROR([installer external dir not found])
	else
		AC_MSG_CHECKING([for installer extdir])
		AC_MSG_RESULT([${with_installer_extdir}])
		AC_SUBST([INSTALLER_EXTDIR], [${with_installer_extdir}])
	fi
fi


# -O0 turns off all optimizations; this is necessary for accurate debugging
# -Wno-unused-label is necessary because flex produces unused labels that we cannot fix
# Add later for stricter checking: -Wextra -Wmissing-declarations -Wstrict-prototypes
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--disable-debug[=yes/relaxed/no]],[Compile debug version [yes]]),
	[ enable_debug=${enableval} ], [ enable_debug=yes ])
AC_MSG_CHECKING([whether to compile in debug mode])
AC_MSG_RESULT([${enable_debug}])
if test "x$enable_debug" = "xyes" ; then
	if test "x$cc_icc" = "xyes" ; then
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -DDEBUG -Wcheck -Werror"
	else
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -DDEBUG -Wall -Werror -Wno-unused-label -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast"
	fi
elif test "x$enable_debug" = "xrelaxed" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -DDEBUG -Wall -Wextra"
else
	WZ_CFLAGS="${WZ_CFLAGS} -g -DNDEBUG -Wall"
fi


# Checks for modules:

PKG_CHECK_MODULES([SDL], [sdl >= 1.2])
PKG_CHECK_MODULES([OPENGL], [gl >= 6.0]) # This is the Mesa version, not GL version
PKG_CHECK_MODULES([OPENAL], [openal >= 0.0.8])
PKG_CHECK_MODULES([PNG], [libpng >= 1.2])
PKG_CHECK_MODULES([OGGVORBIS], [ogg >= 1.1 vorbis >= 1.1 vorbisfile >= 1.1])


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

# Look for OpenGLC
AC_CHECK_HEADER(GL/glc.h, ,
	AC_MSG_ERROR([OpenGLC header not found. Please install QuesoGLC: http://quesoglc.sourceforge.net/]))
AC_CHECK_LIB(GLC, main,
	OPENGLC_LIBS="-lGLC",
	AC_CHECK_LIB(glc32, main,
		OPENGLC_LIBS="-lglc32",
		AC_MSG_ERROR([OpenGLC library not found. Please install QuesoGLC: http://quesoglc.sourceforge.net/])))
AC_SUBST([OPENGLC_LIBS], [${OPENGLC_LIBS}])

# Look for OpenAL
AC_CHECK_HEADER(AL/al.h, ,
	AC_MSG_ERROR([OpenAL header not found.]))


WZ_CFLAGS="${WZ_CFLAGS} -DDATADIR=\"\\\"\${datadir}/\${PACKAGE}\\\"\""
WZ_CFLAGS="${WZ_CFLAGS} -DLOCALEDIR=\"\\\"\${localedir}\\\"\""


# add some required C flags here
# -DYY_STATIC is required by flex
WZ_CFLAGS="${WZ_CFLAGS} -DYY_STATIC"


CPPFLAGS="${CPPFLAGS} -I\$(top_srcdir)"
CFLAGS="${SDL_CFLAGS} ${PNG_CFLAGS} ${OGGVORBIS_CFLAGS} $} ${OPENAL_CFLAGS} ${OPENGL_CFLAGS} ${CFLAGS} ${WZ_CFLAGS}"


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

if test "x$enable_debug" = "xyes" ; then
	AC_MSG_NOTICE()
	AC_MSG_NOTICE([*** Running in debug mode! ***])
fi

if test "x$with_distributor" = "x" ; then
	AC_MSG_NOTICE()
	AC_MSG_WARN([Distributor unknown!])
	AC_MSG_NOTICE([Use --with-distributor when compiling package for distribution])
fi
