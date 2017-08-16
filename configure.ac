AC_PREREQ([2.56])
AC_INIT([Warzone 2100],[master],[http://wz2100.net/],[warzone2100])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-xz])

# Set to the default or stupid automake will use the much slower but useless -e(xtreme!!!) option.
AC_SUBST([XZ_OPT], ["-6"])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/framework/frame.cpp])

# HACK
# Workaround stupid autoconf CFLAGS default
# (Bash considers set but empty variables different from null for the ${VAR:+alt} operator)
CFLAGS="${CFLAGS} -fPIC"
CXXFLAGS="${CXXFLAGS} -fPIC -std=c++11"

# Make cross compilation options available in --help
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC_STDC
AC_CHECK_PROG(CCOMPILER, ${CC}, ccompiler)
test -z "${CCOMPILER}" && AC_MSG_ERROR(No C compiler found!)
AC_PROG_RANLIB

AC_PROG_CXX
AC_CHECK_PROG(CXXCOMPILER, ${CXX}, cxxcompiler)
test -z "${CXXCOMPILER}" && AC_MSG_ERROR(No C++ compiler found!)

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
rm -f filesynctest.zip
AC_SUBST([ZIP_UPDATE], [${ZIP_UPDATE}])

AC_CHECK_PROG(A2X, a2x, a2x)
AC_CHECK_PROG(WKHTMLTOPDF, wkhtmltopdf, wkhtmltopdf)
AM_CONDITIONAL([A2X_AVAILABLE], test -n "$A2X")
AM_CONDITIONAL([WKHTMLTOPDF_AVAILABLE], test -n "$WKHTMLTOPDF")

AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
AC_CHECK_PROG(HEVEA, hevea, hevea)
AC_CHECK_PROG(HACHA, hacha, hacha)
AM_CONDITIONAL(PDFLATEX, test -n "$PDFLATEX")
AM_CONDITIONAL(HEVEA, test -n "$HEVEA")
AM_CONDITIONAL(HACHA, test -n "$HACHA")

# Enable Compiler/LibC GNU extensions
AC_GNU_SOURCE

# __builtin_expect
AX_C___BUILTIN_EXPECT

# Check for the strlcpy and strlcat functions
AX_CHECK_STRLCPY
AX_CHECK_STRLCAT

# Use -fstack-protector if possible
AX_STACK_PROTECT_CC
AX_STACK_PROTECT_CXX

AC_CHECK_HEADERS(alloca.h sys/ucontext.h)

# Check for gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.15])

# Check for pkg-config
PKG_PROG_PKG_CONFIG
AC_PROG_VERSION_CHECK([pkg-config], [0.9])

AC_MSG_CHECKING([for backend])
AC_ARG_WITH([backend],
	AS_HELP_STRING([--with-backend=sdl/qt], [Backend rendering code to use [sdl]]),
	[ backend=${withval} ], [ backend="sdl" ])
AM_CONDITIONAL([BACKEND_QT], test "$backend" = "qt")
AM_CONDITIONAL([BACKEND_SDL], test "$backend" = "sdl")

# Check for Qt
if test "$backend" = "qt"; then
	AC_MSG_RESULT([Qt])
	PKG_CHECK_MODULES(QT5, Qt5Core Qt5Gui Qt5Widgets Qt5Script Qt5OpenGL Qt5X11Extras,,[:])
else
	AC_MSG_RESULT([SDL])
	PKG_CHECK_MODULES(QT5, Qt5Core Qt5Gui Qt5Widgets Qt5Script,,[:])
fi
if test "$pkg_failed" = "yes" ; then
	AC_MSG_ERROR([Qt 5 not found - required!])
fi
QT5_INCLUDES=$QT5_FRONTEND_CFLAGS
QT5_LDFLAGS=$QT5_FRONTEND_LIBS
QT5_LIB=$QT5_FRONTEND_LIBS
QT5_PLUGIN=`$PKG_CONFIG --variable=exec_prefix Qt5Core`
AC_SUBST([QT5_PLUGIN])
QT5_VERSION=`$PKG_CONFIG --modversion Qt5Core`
AC_SUBST(QT5_VERSION)
QT_PATH=`$PKG_CONFIG --variable=exec_prefix Qt5Core`
QT_HOST_PATH=`$PKG_CONFIG --variable=host_bins Qt5Core`
AC_PATH_PROGS(MOC5, [moc-qt5 moc], moc, ["${QT_HOST_PATH}" "${QT_PATH}/bin"])
AC_PATH_PROGS(UIC5, [uic-qt5 uic], uic, ["${QT_HOST_PATH}" "${QT_PATH}/bin"])
AC_PATH_PROGS(RCC5, [rcc-qt5 rcc], rcc, ["${QT_HOST_PATH}" "${QT_PATH}/bin"])

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
		WIN32_LIBS='-ldbghelp -lshfolder -lshlwapi -lpsapi -lshell32 -lwinmm -lws2_32 -liphlpapi -ladvapi32'
		AC_SUBST([WIN32_LIBS], [${WIN32_LIBS}])
		LDFLAGS="-Wl,-subsystem,windows ${LDFLAGS}"
		;;
	*)
		# Some platforms don't need _XOPEN_SOURCE
		case ${host_os} in
			*bsd* | dragonfly* | bitrig*) ;;
			*)
				AC_DEFINE([_XOPEN_SOURCE], 600, [Enable POSIX extensions if present])
				;;
		esac

		# Make sure that backtrace_symbols_fd (GNU raw back trace extension) has some symbols to work  with
		LDFLAGS="-rdynamic ${LDFLAGS}"
		;;
esac
AM_CONDITIONAL([MINGW32], test "x$host_os_mingw32" = "xyes")


# Check for X11 (for qtgame and scrap)
if test "x$host_os_mingw32" != "xyes" ; then
	X_LIBS=""
	AC_PATH_X
	if test x$no_x != "xyes"; then
		X_LIBS="-lX11 -lXrandr ${X_LIBS}"
	fi
	AC_SUBST(X_LIBS)
fi

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
	AC_SUBST([localedir], [${datarootdir}/locale])
])


# Installation directories

AC_ARG_WITH(icondir,
	AS_HELP_STRING([--with-icondir=DIR],[icon files [DATADIR/icons]]),
	[ with_icondir=${withval} ], [ with_icondir=${datarootdir}/icons ])
AC_SUBST([icondir], [${with_icondir}])

AC_ARG_WITH(applicationdir,
	AS_HELP_STRING([--with-applicationdir=DIR],[application files [DATADIR/applications]]),
	[ with_applicationdir=${withval} ], [ with_applicationdir=${datarootdir}/applications ])
AC_SUBST([applicationdir], [${with_applicationdir}])

AC_ARG_WITH(appdatadir,
	AS_HELP_STRING([--with-appdatadir=DIR],[application files [DATADIR/metainfo]]),
	[ with_appdatadir=${withval} ], [ with_appdatadir=${datarootdir}/metainfo ])
AC_SUBST([appdatadir], [${with_appdatadir}])

# Commandline options - enable:

AC_ARG_ENABLE([installer],
	AS_HELP_STRING([--enable-installer], [Build NSIS installer [no]]),
	[ enable_installer=${enableval} ], [ enable_installer=no ])
AM_CONDITIONAL(INSTALLER, test "x$enable_installer" = "xyes")
AC_MSG_CHECKING([whether to build NSIS installer])
AC_MSG_RESULT([${enable_installer}])

# windows only option at this time
AC_ARG_ENABLE([portable],
	AS_HELP_STRING([--enable-portable], [Build portable version of Warzone (windows ONLY!) [no]]),
	[ enable_portable="${enableval}" ], [ enable_portable=no ])
AS_IF([test "x$enable_portable" = "xyes"],[
	AC_DEFINE([WZ_PORTABLE], 1, [Makes builds become portable (windows ONLY)])
])
AM_CONDITIONAL(PORTABLE, test $enable_portable = yes)

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
	AS_HELP_STRING([--enable-debug[=yes/relaxed/profile/debugprofile/optimised/no]],[Compile debug version [[yes]]]),
	[ enable_debug=${enableval} ], [ enable_debug=yes ])
AC_MSG_CHECKING([whether to compile in debug mode])
AC_MSG_RESULT([${enable_debug}])

# gcc 4.5 added some warnings to -Wall that weren't even valid in previous
# versions. The -Werror is necessary since otherwise gcc prints a warning that
# the option is only valid for c++/objc++, but the test succeeds. The order is
# also important, it doesn't work if -Werror comes last.
AX_C_CHECK_FLAG([-Werror -Wno-switch -Wno-error=cpp], [#warning x], , CFLAGS_IGNORE_WARNINGS="${CFLAGS_IGNORE_WARNINGS} -Wno-switch")
AX_C_CHECK_FLAG([-Werror -Wno-enum-compare -Wno-error=cpp], [#warning x], , CFLAGS_IGNORE_WARNINGS="${CFLAGS_IGNORE_WARNINGS} -Wno-enum-compare")
# Only needed for CXX, actually.
WZ_CPPFLAGS="${WZ_CPPFLAGS} -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS"

# zlib
LDFLAGS="-lz ${LDFLAGS} -fPIC"

# thread support
LDFLAGS="${LDFLAGS} -lpthread"

if test "x$enable_warnings" = "xyes" ; then
	# Only disable -Werror for the warning.
	WZ_Wno_="-Wno-error="
else
	# Disable warning completely.
	WZ_Wno_="-Wno-"
fi

WZ_WARNINGS_GCC="-Wall -Wextra ${WZ_Wno_}unused-parameter ${WZ_Wno_}sign-compare -Wcast-align -Wwrite-strings -Wpointer-arith ${WZ_Wno_}format-security"
AX_C_CHECK_FLAG([-Werror -Wno-expansion-to-defined -Wno-error=cpp], [#warning x], , WZ_WARNINGS_GCC="${WZ_WARNINGS_GCC} -Wno-expansion-to-defined")
WZ_WARNINGS_GCC_C="${WZ_WARNINGS_GCC} -Wstrict-prototypes -Wdeclaration-after-statement ${CFLAGS_IGNORE_WARNINGS}"
WZ_WARNINGS_GCC_CXX="${WZ_Wno_}enum-compare ${WZ_WARNINGS_GCC}"
if test "x$enable_debug" = "xyes" ; then
	if test "x$cc_icc" = "xyes" ; then
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -g -Wcheck -Werror"
		WZ_CXXFLAGS="${WZ_CXXFLAGS} -O0 -g -Wcheck -Werror"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	else
		# -ggdb is very useful when running the linux perf tool
		WZ_CFLAGS="${WZ_CFLAGS} -O0 -ggdb -Werror ${WZ_WARNINGS_GCC_C}"
		WZ_CXXFLAGS="${WZ_CXXFLAGS} -O0 -ggdb -Werror ${WZ_WARNINGS_GCC_CXX}"
		WZ_C99FLAGS="${WZ_C99FLAGS} -Wno-declaration-after-statement"
		WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	fi
# debugging with optimizations for gcc or clang
elif test "x$enable_debug" = "xoptimised" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -O2 -ggdb -fno-omit-frame-pointer -Werror ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -O2 -ggdb -fno-omit-frame-pointer -Werror ${WZ_WARNINGS_GCC_CXX}"
	WZ_C99FLAGS="${WZ_C99FLAGS} -Wno-declaration-after-statement"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
# debug build while ignoring warnings for gcc or clang
elif test "x$enable_debug" = "xrelaxed" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -O0 ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -O0 ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
# profiling builds using gprof (probably a better idea to use perf, if you can)
elif test "x$enable_debug" = "xdebugprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -O0 -pg ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -O0 -pg ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	LDFLAGS="-pg ${LDFLAGS}"
elif test "x$enable_debug" = "xprofile" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -Os -fno-omit-frame-pointer -pg ${WZ_WARNINGS_GCC_C}"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -Os -fno-omit-frame-pointer -pg ${WZ_WARNINGS_GCC_CXX}"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DNDEBUG"
	LDFLAGS="-pg ${LDFLAGS}"
elif test "x$enable_debug" = "xsanitize-undefined" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_C} -Werror -fsanitize=undefined"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_CXX} -Werror -fsanitize=undefined"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	LDFLAGS="${LDFLAGS} -fsanitize=undefined"
elif test "x$enable_debug" = "xsanitize-address" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_C} -Werror -fsanitize=address"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_CXX} -Werror -fsanitize=address"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	LDFLAGS="${LDFLAGS} -fsanitize=address -lpthread"
elif test "x$enable_debug" = "xsanitize-thread" ; then
	WZ_CFLAGS="${WZ_CFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_C} -Werror -fsanitize=thread"
	WZ_CXXFLAGS="${WZ_CXXFLAGS} -ggdb -Os -fno-omit-frame-pointer ${WZ_WARNINGS_GCC_CXX} -Werror -fsanitize=thread"
	WZ_CPPFLAGS="${WZ_CPPFLAGS} -DDEBUG"
	LDFLAGS="${LDFLAGS} -fsanitize=thread -lpthread"
# normal, non-debug build
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

AC_ARG_WITH([videos],
	AS_HELP_STRING([--with-videos], [Install videos with the portable version of Warzone (windows ONLY!) [no]]),
	[ with_videos="${withval}" ], [ with_videos=no ])
#AS_IF([test "x$with_videos" = "xyes"],[
#	AC_DEFINE([WZ_PORTABLE], 1, [Makes builds become portable (windows ONLY)])
#])
AM_CONDITIONAL(VIDEOS, test $with_videos = yes)

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

if test "$backend" = "sdl"; then
	PKG_CHECK_MODULES([SDL], [sdl2 >= 2.0.0])
fi
PKG_CHECK_MODULES([PNG], [libpng >= 1.2])
PKG_CHECK_MODULES([THEORA], [theora >= 1.0])
PKG_CHECK_MODULES([OPENAL], [openal >= 0.0.8])
PKG_CHECK_MODULES([VORBIS], [vorbis >= 1.1])
PKG_CHECK_MODULES([VORBISFILE], [vorbisfile >= 1.1])

PKG_CHECK_MODULES([GLEW], [glew >= 1.5.2])

# Checks for libraries:

# check for nearbyint()
AC_CHECK_LIB(m, nearbyint, [MATH_LIB=""], AC_MSG_ERROR([nearbyint not found.]))

# When (cross-)compiling for Windows (MinGW) we need to link in iberty for the Dr. MinGW derived exception handler.
if test "x$host_os_mingw32" = "xyes" ; then
    AC_CHECK_LIB(iberty, main, AC_SUBST([IBERTY_LIBS], [-liberty]), AC_MSG_ERROR([libiberty not found.]))

    WIN32_LIBS="${WIN32_LIBS} ${IBERTY_LIBS}"
fi

# Look for PhysicsFS
AC_CHECK_HEADER(physfs.h, , AC_MSG_ERROR([PhysicsFS header not found.]))
AC_CHECK_LIB(physfs, PHYSFS_mount, AC_SUBST([PHYSFS_LIBS], [-lphysfs]), AC_MSG_ERROR([PhysicsFS 2 not found.]), [${WIN32_LIBS}])

if test "x$host_os_mingw32" != "xyes" ; then
	AC_CHECK_HEADER(X11/extensions/Xrandr.h, , AC_MSG_ERROR([Xrandr header not found.]))
fi


# Look for OpenGL
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

PKG_CHECK_MODULES([FRIBIDI], [fribidi])
PKG_CHECK_MODULES([FREETYPE], [freetype2])
PKG_CHECK_MODULES([HARFBUZZ], [harfbuzz])
WZ_CXXFLAGS="${WZ_CXXFLAGS} ${HARFBUZZ_CFLAGS} ${FREETYPE_CFLAGS}"
AC_CHECK_LIB(freetype, FTC_Manager_New,
             AC_DEFINE([HAVE_FT_CACHE], [1],
                       [Define if FreeType supports the caching routines]))
PKG_CHECK_MODULES([FONTCONFIG], [fontconfig])

FONT_CFLAGS="${FRIBIDI_CFLAGS} ${FREETYPE_CFLAGS} ${FRIBIDI_CFLAGS} ${FONTCONFIG_CFLAGS} ${HARFBUZZ_CFLAGS}"
FONT_LIBS="${FRIBIDI_LIBS} ${FREETYPE_LIBS} ${FRIBIDI_LIBS} ${FONTCONFIG_LIBS} ${OPENGL_LIBS} ${GLEW_LIBS} ${HARFBUZZ_LIBS}"
AC_SUBST([FONT_CFLAGS], [${FONT_CFLAGS}])
AC_SUBST([FONT_LIBS], [${FONT_LIBS}])

# When (cross-)compiling for Windows (MinGW) we need to link in BFD for the Dr.
# MinGW derived exception handler.
if test "x$host_os_mingw32" = "xyes" ; then
    AC_CHECK_HEADER(bfd.h, , AC_MSG_ERROR([BFD header not found.]))
    AC_CHECK_LIB(bfd, bfd_openr, AC_SUBST([BFD_LIBS], [-lbfd]), AC_MSG_ERROR([BFD not found.]), [${WIN32_LIBS}])

    WIN32_LIBS="${BFD_LIBS} ${WIN32_LIBS} -lstdc++"
fi

WZ_CPPFLAGS="${WZ_CPPFLAGS} -DWZ_DATADIR=\"\\\"\${datadir}/\${PACKAGE}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -DLOCALEDIR=\"\\\"\${localedir}\\\"\""
WZ_CPPFLAGS="${WZ_CPPFLAGS} -I\$(top_srcdir)"

# 3rdparty libs
WZ_CPPFLAGS="${WZ_CPPFLAGS} -I\$(top_srcdir)/3rdparty"


AC_SUBST([WZ_CPPFLAGS], [${WZ_CPPFLAGS}])
AC_SUBST([WZ_CFLAGS], [${WZ_CFLAGS}])
AC_SUBST([WZ_CXXFLAGS], [${WZ_CXXFLAGS}])
AC_SUBST([WZ_C99FLAGS], [${WZ_C99FLAGS}])


AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile
		po/Makefile.in
		doc/Makefile
		icons/Makefile
		data/Makefile
		data/music/Makefile
		pkg/nsis/Makefile
		win32/Makefile
		tests/Makefile
		3rdparty/miniupnpc/Makefile
		3rdparty/micro-ecc/Makefile
		3rdparty/sha2/Makefile
		lib/framework/Makefile
		lib/exceptionhandler/Makefile
		lib/gamelib/Makefile
		lib/ivis_opengl/Makefile
		lib/netplay/Makefile
		lib/qtgame/Makefile
		lib/sdl/Makefile
		lib/script/Makefile
		lib/sequence/Makefile
		lib/sound/Makefile
		lib/widget/Makefile
		tools/map/Makefile
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

AS_IF([test -z "$A2X" -o -z "$WKHTMLTOPDF"],[
	AC_MSG_NOTICE()
	AC_MSG_WARN([a2x (asciidoc) or wkhtmltopdf not found, can't convert documentation. This doesn't affect building the game,
		but means that things like "make dist" and building a Windows installer will fail.])
])

AS_IF([test "x$enable_portable" = "xyes"],[
	AC_MSG_NOTICE()
	AC_MSG_NOTICE([*** Creating Portable version of Warzone (this options only affects Windows builds!) ***])
])
AS_IF([test "x$with_videos" = "xyes"],[
	AC_MSG_NOTICE()
	AC_MSG_NOTICE([*** Creating Portable version of Warzone WITH VIDEOS (this options only affects Windows builds!) ***])
])



AC_MSG_NOTICE()
AC_MSG_NOTICE(Backend used: ${backend})
