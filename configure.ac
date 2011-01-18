AC_PREREQ([2.56])
AC_INIT([Warzone 2100],[master],[http://wz2100.net/],[warzone2100])

AM_INIT_AUTOMAKE([1.10 tar-ustar])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/framework/frame.cpp])


# HACK
# Workaround stupid autoconf CFLAGS default
# (Bash considers set but empty variables different from null for the ${VAR:+alt} operator)
CFLAGS="${CFLAGS}"


# Make cross compilation options available in --help
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC_STDC
AC_PROG_RANLIB

# We need a C++ compiler for build_tools/autorevision
AC_PROG_CXX

AC_PROG_YACC
if test "x$YACC" != "xbison -y" ; then
	AC_MSG_ERROR([Bison is not installed])
fi
AC_PROG_VERSION_CHECK([bison], [1.31])

AC_PROG_LEX
AC_PROG_VERSION_CHECK([${LEX}], [2.5.33], [2.5.34])

AX_WITH_PROG([PERL], [perl])
AX_PROG_PERL_VERSION([5.10], [], [ PERL="" ])
AC_SUBST([PERL])
AM_CONDITIONAL([PERL_AVAILABLE], test -n "$PERL")

AC_CHECK_PROG(ZIP, zip, zip)
test -z "${ZIP}" && AC_MSG_ERROR(Zip not found!)
AC_CHECK_PROG(UNZIP, unzip, unzip)
test -z "${UNZIP}" && AC_MSG_ERROR(Unzip not found!)
# --filesync was added in zip 3.0, older versions need -u
ZIP_UPDATE="-u"
zip -q --filesync filesynctest.zip $srcdir/ChangeLog 2> /dev/null && export ZIP_UPDATE="--filesync"
echo $srcdir
rm -f filesynctest.zip
AC_SUBST([ZIP_UPDATE], [${ZIP_UPDATE}])

# Enable Compiler/LibC GNU extensions
AC_GNU_SOURCE

# Check for the strlcpy and strlcat functions
AX_CHECK_STRLCPY
AX_CHECK_STRLCAT

# Use -fstack-protector if possible
AX_STACK_PROTECT_CC
AX_STACK_PROTECT_CXX

AC_CHECK_HEADERS(alloca.h)

# Check for gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.15])

# Check for pkg-config
PKG_PROG_PKG_CONFIG
AC_PROG_VERSION_CHECK([pkg-config], [0.9])


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
		WIN32_LIBS='-ldbghelp -lshfolder -lshlwapi -lpsapi -lshell32 -lwinmm -lws2_32'
		AC_SUBST([WIN32_LIBS], [${WIN32_LIBS}])
		;;
	*openbsd*)
		AC_DEFINE([_XOPEN_SOURCE], 600, [Enable POSIX extensions if present])
		# Make sure that backtrace_symbols_fd (GNU raw back trace extension) has some symbols to work with
		LDFLAGS="-Wl,-export-dynamic ${LDFLAGS}"
		;;
	*)
		# Some platforms don't need _XOPEN_SOURCE
		case ${host_os} in
			*freebsd*) ;;
			*)
				AC_DEFINE([_XOPEN_SOURCE], 600, [Enable POSIX extensions if present])
				;;
		esac

		# Make sure that backtrace_symbols_fd (GNU raw back trace extension) has some symbols to work  with
		LDFLAGS="-rdynamic ${LDFLAGS}"
		;;
esac
AM_CONDITIONAL([MINGW32], test "x$host_os_mingw32" = "xyes")


# Set compilers to use when building for the build system in a cross compile build
# Fallback to the system compilers
AC_ARG_VAR([CXX_FOR_BUILD], [C++ compiler that produces binaries that will run
	    on the build system])
AS_IF([test -z "$CXX_FOR_BUILD"],[
    AC_SUBST([CXX_FOR_BUILD], [${CXX}])
])

AC_ARG_VAR([CC_FOR_BUILD], [C compiler that produces binaries that will run on
	    the build system])
AS_IF([test -z "$CC_FOR_BUILD"],[
    AC_SUBST([CC_FOR_BUILD], [${CC}])
])

AC_ARG_VAR([CFLAGS_FOR_BUILD], [Flags to use for the C
	    compiler that produces binaries for the build system])
AS_IF([test -z "$CFLAGS_FOR_BUILD"],[
    AC_SUBST([CFLAGS_FOR_BUILD], [${CFLAGS}])
])

AC_ARG_VAR([CXXFLAGS_FOR_BUILD], [Flags to use for the C++
	    compiler that produces binaries for the build system])
AS_IF([test -z "$CXXFLAGS_FOR_BUILD"],[
    AC_SUBST([CXXFLAGS_FOR_BUILD], [${CXXFLAGS}])
])

AC_ARG_VAR([CPPFLAGS_FOR_BUILD], [Preprocessor flags to use for the C and C++
	    compilers that produce binaries for the build system])
AS_IF([test -z "$CPPFLAGS_FOR_BUILD"],[
    AC_SUBST([CPPFLAGS_FOR_BUILD], [${CPPFLAGS}])
])


# pkg-config hooks:

AC_ARG_VAR([PKGCONFIG_PREFIX], [Prefix path to look in for pkg-config templates])
AS_IF([test -n "$PKGCONFIG_PREFIX"],[
	PKG_CONFIG="${PKG_CONFIG} --define-variable=prefix=${PKGCONFIG_PREFIX}"
])

AC_ARG_VAR([PKGCONFIG_DEFAULT_PREFIX], [Prefix path to look in for pkg-config templates])
AS_IF([test -n "$PKGCONFIG_DEFAULT_PREFIX"],[
	PKG_CONFIG="${PKG_CONFIG} --define-variable=__PREFIX__=${PKGCONFIG_DEFAULT_PREFIX}"
])

# Variable fixes for older autoconf versions:

AS_IF([test -z "$docdir"],[
	AC_SUBST([docdir], [${datarootdir}/doc/${AC_PACKAGE_TARNAME}])
])
AS_IF([test -z "$localedir"],[
	AC_SUBST([localedir], [${datadir}/locale])
])


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

AC_ARG_ENABLE([sound],
	AS_HELP_STRING([--disable-sound], [Disable sound [no]]),
	[ enable_sound="${enableval}" ], [ enable_sound="yes" ])
AS_IF([test "x$enable_sound" != "xyes"],[
	AC_DEFINE([WZ_NOSOUND], [], [Causes sound to be disabled])
])

# -O0 turns off all optimizations; this is necessary for accurate debugging
# -Wno-unused-label is necessary because flex produces unused labels that we cannot fix
# Add later for stricter checking: -Wextra -Wmissing-declarations -Wstrict-prototypes
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug[=yes/relaxed/profile/debugprofile/optimised/no]],[Compile debug version [[yes]]]),
	[ enable_debug=${enableval} ], [ enable_debug=yes ])
AC_MSG_CHECKING([whether to compile in debug mode])
AC_MSG_RESULT([${enable_debug}])

# gcc 4.5 added some warnings to -Wall that weren't even valid in previous
# versions. The -Werror is necessary since otherwise gcc prints a warning that
# the option is only valid for c++/objc++, but the test succeeds. The order is
# also important, it doesn't work if -Werror comes last.
AX_C_CHECK_FLAG([-Werror -Wno-switch], , , CFLAGS_IGNORE_WARNINGS="${CFLAGS_IGNORE_WARNINGS} -Wno-switch")
AX_C_CHECK_FLAG([-Werror -Wno-enum-compare], , , CFLAGS_IGNORE_WARNINGS="${CFLAGS_IGNORE_WARNINGS} -Wno-enum-compare")

# zlib
LDFLAGS="-lz ${LDFLAGS}"

if test "x$enable_warnings" = "xyes" ; then
	# Only disable -Werror for the warning.
	WZ_Wno_="-Wno-error="
else
	# Disable warning completely.
	WZ_Wno_="-Wno-"
fi

WZ_WARNINGS_GCC="-Wall -Wextra ${WZ_Wno_}unused-parameter ${WZ_Wno_}sign-compare -Wcast-align -Wwrite-strings -Wpointer-arith ${WZ_Wno_}format-security"
WZ_WARNINGS_GCC_C="${WZ_WARNINGS_GCC} -Wstrict-prototypes -Wdeclaration-after-statement -Wc++-compat ${WZ_Wno_}c++-compat ${CFLAGS_IGNORE_WARNINGS}"
WZ_WARNINGS_GCC_CXX="${WZ_Wno_}enum-compare ${WZ_WARNINGS_GCC}"
if test "x$enable_debug" = "xyes" ; then
	if test "x$cc_icc" = "xyes" ; then
		# "-fp-model precise -fp-model source -ftz -no-fma" was here
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -Wcheck -Werror"
		WZ_CXXFLAGS="${WZ_CXXFLAGS} -O0 -g -Wcheck -Werror"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	else
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -Werror ${WZ_WARNINGS_GCC_C}"
		WZ_CXXFLAGS="${WZ_CXXFLAGS} -O0 -g -Werror ${WZ_WARNINGS_GCC_CXX}"
		WZ_C99FLAGS="${WZ_C99FLAGS} -Wno-declaration-after-statement"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	fi
elif test "x$enable_debug" = "xoptimised" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -O2 -g -Werror ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -O2 -g -Werror ${WZ_WARNINGS_GCC_CXX}"
	WZ_C99FLAGS="${WZ_C99FLAGS} -Wno-declaration-after-statement"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
elif test "x$enable_debug" = "xrelaxed" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -O0 ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -g -O0 ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
elif test "x$enable_debug" = "xdebugprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -O0 -pg ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -g -O0 -pg ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	LDFLAGS="-pg ${LDFLAGS}"
elif test "x$enable_debug" = "xprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -g -Os -pg ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -g -Os -pg ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DNDEBUG"
	LDFLAGS="-pg ${LDFLAGS}"
elif test "x$enable_debug" = "xg++" ; then
	WZ_CFLAGS="${WZ_CXXFLAGS} -g -O2 ${WZ_WARNINGS_GCC_CXX}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -g -O2 ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	CC="g++"
else
	WZ_CFLAGS="${WZ_CFLAGS} -g ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -g ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DNDEBUG"
fi


# Commandline options - with:

AC_ARG_WITH([distributor],
	AS_HELP_STRING([--with-distributor], [Name of distributor compiling this package]),
	[ with_distributor=${withval} ], [ with_distributor="UNKNOWN" ])
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
PKG_CHECK_MODULES([PNG], [libpng >= 1.2])
PKG_CHECK_MODULES([THEORA], [theora >= 1.0])

AS_IF([test "x$enable_sound" = "xyes"],[
	PKG_CHECK_MODULES([OPENAL], [openal >= 0.0.8])
	PKG_CHECK_MODULES([OGGVORBIS], [vorbisfile >= 1.1])
])

if test "x$enable_static" = "xyes" ; then
	# If linking statically, make sure to include the direct dependencies of
	# QuesoGLC as well, because it does not have a pkg-config file.
	# FIXME Remove this whenever QuesoGLC has a pkg-config file
	PKG_CHECK_MODULES([FONTCONFIG], [fontconfig])
	PKG_CHECK_MODULES([FREETYPE], [freetype2])
fi

system_glee=false
if test "x$host_os_mingw32" != "xyes" ; then
	PKG_CHECK_MODULES([GLee], [glee >= 5.4.0], [system_glee=true], [system_glee=false])
	if test "x$system_glee" = "xfalse" ; then
		AX_EXT_HAVE_HEADER([GLee.h], [/usr/local/include/GL /usr/include/GL])
		save_cppflags="${CPPFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${GLEE_H_CPPFLAGS}"
		AC_CHECK_HEADER([GLee.h], [system_glee=true], [system_glee=false])
		CPPFLAGS=${save_cppflags}
		if test "x$system_glee" = "xtrue" ; then
			AC_CHECK_LIB([GLee], [GLeeInit], [system_glee=true], [system_glee=false])
			if test "x$system_glee" = "xtrue" ; then
				AC_SUBST([GLee_CFLAGS], [${GLEE_H_CPPFLAGS}])
				AC_SUBST([GLee_LIBS], [-lGLee])
			fi
		fi
	fi
fi
AM_CONDITIONAL([SYSTEM_GLEE], [test "x$system_glee" = "xtrue"])
if test "x$system_glee" = "xfalse" ; then
	AC_MSG_NOTICE([Using integrated GLee])
	AC_SUBST([GLee_CFLAGS], ['-I$(top_srcdir)/lib/ivis_opengl'])
fi

# Checks for libraries:

# check for nearbyint()
AC_CHECK_LIB(m, nearbyint, [MATH_LIB=""], AC_MSG_ERROR([nearbyint not found.]))

# When (cross-)compiling for Windows (MinGW) we need to link in iberty for the Dr. MinGW derived exception handler.
if test "x$host_os_mingw32" = "xyes" ; then
    AC_CHECK_LIB(iberty, main, AC_SUBST([IBERTY_LIBS], [-liberty]), AC_MSG_ERROR([libiberty not found.]))

    WIN32_LIBS="${WIN32_LIBS} ${IBERTY_LIBS}"
fi

# Look for X11 if required
AC_TRY_COMPILE(, [
#if   (defined(__OS2__) && !defined(__EMX__))
# error Running OS2, not a Unix system.
#elif (!defined(SAG_COM) && (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WIN64__) || defined(__NT__))) || (defined(__MWERKS__) && defined(__INTEL__))
# error Running on Windows, not a Unix system.
#endif
],[
	AC_CHECK_LIB(X11, XSetSelectionOwner, AC_SUBST([X11_LIBS], [-lX11]),
		AC_MSG_ERROR([X11 library not found]))
])

# Look for PhysicsFS
AC_CHECK_HEADER(physfs.h, , AC_MSG_ERROR([PhysicsFS header not found.]))
AC_CHECK_LIB(physfs, PHYSFS_init, AC_SUBST([PHYSFS_LIBS], [-lphysfs]), AC_MSG_ERROR([PhysicsFS not found.]), [${WIN32_LIBS}])

# Look for OpenGL
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
AC_CHECK_HEADER([SDL_opengl.h], [],
	AC_MSG_ERROR([SDL OpenGL header not found. Please install SDL with OpenGL support.]))
CPPFLAGS="$CPPFLAGS_SAVE"

AC_CHECK_LIB(GL, main,
	OPENGL_LIBS="-lGL",
	AC_CHECK_LIB(opengl32, main,
		OPENGL_LIBS="-lopengl32",
		AC_MSG_ERROR([OpenGL library not found.])))
AC_CHECK_LIB(GLU, main,
	OPENGL_LIBS="${OPENGL_LIBS} -lGLU",
	AC_CHECK_LIB(glu32, main,
		OPENGL_LIBS="${OPENGL_LIBS} -lglu32",
		AC_MSG_ERROR([GLU library not found.]),
		[${OPENGL_LIBS} -lm]), [${OPENGL_LIBS} -lm])
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

# When (cross-)compiling for Windows (MinGW) we need to link in BFD for the Dr.
# MinGW derived exception handler.
if test "x$host_os_mingw32" = "xyes" ; then
    AC_CHECK_HEADER(bfd.h, , AC_MSG_ERROR([BFD header not found.]))
    AC_CHECK_LIB(bfd, bfd_openr, AC_SUBST([BFD_LIBS], [-lbfd]), AC_MSG_ERROR([BFD not found.]), [${IBERTY_LIBS}])

    WIN32_LIBS="${WIN32_LIBS} ${BFD_LIBS} ${IBERTY_LIBS} -lstdc++"
fi

WZ_CPPFLAGS="${WZ_CPPFLAGS} -DWZ_DATADIR=\"\\\"\${datadir}/\${PACKAGE}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -DLOCALEDIR=\"\\\"\${localedir}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -I\$(top_srcdir)"


AC_SUBST([WZ_CPPFLAGS], [${WZ_CPPFLAGS}])
AC_SUBST([WZ_CFLAGS], [${WZ_CFLAGS}])
AC_SUBST([WZ_CXXFLAGS], [${WZ_CXXFLAGS}])
AC_SUBST([WZ_C99FLAGS], [${WZ_C99FLAGS}])


AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile
		po/Makefile.in
		doc/Makefile
		build_tools/autorevision/Makefile
		icons/Makefile
		data/Makefile
		data/mods/multiplay/Makefile
		data/music/Makefile
		pkg/nsis/Makefile
		win32/Makefile
		tests/Makefile
		lib/framework/Makefile
		lib/exceptionhandler/Makefile
		lib/gamelib/Makefile
		lib/iniparser/Makefile
		lib/ivis_opengl/Makefile
		lib/netplay/Makefile
		lib/netplay/miniupnpc/Makefile
		lib/script/Makefile
		lib/sequence/Makefile
		lib/sound/Makefile
		lib/widget/Makefile
		src/Makefile])
AC_OUTPUT

AS_IF([test "x$enable_debug" = "xyes"],[
	AC_MSG_NOTICE()
	AC_MSG_NOTICE([*** Running in debug mode! ***])
])

AS_IF([test "x$with_distributor" = "xUNKNOWN"],[
	AC_MSG_NOTICE()
	AC_MSG_WARN([Distributor unknown!])
	AC_MSG_NOTICE([Use --with-distributor when compiling package for distribution])
])

AS_IF([test -z "$PERL"],[
	AC_MSG_NOTICE()
	AC_MSG_WARN([Perl >= 5.10 isn't installed, autogeneration of some sources will not work.])
])
