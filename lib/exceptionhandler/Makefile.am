AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) $(WZ_C99FLAGS)
noinst_LIBRARIES = libexceptionhandler.a
noinst_HEADERS = \
	dumpinfo.h \
	exceptionhandler.h \
	exchndl.h \
	include/coff/internal.h \
	include/demangle.h \
	include/libcoff.h

libexceptionhandler_a_SOURCES = \
	dumpinfo.cpp \
	exceptionhandler.c

if MINGW32
libexceptionhandler_a_SOURCES += exchndl.c
endif

