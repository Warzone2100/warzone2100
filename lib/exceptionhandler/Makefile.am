AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
noinst_LIBRARIES = libexceptionhandler.a
noinst_HEADERS = exceptionhandler.h exchndl.h

libexceptionhandler_a_SOURCES = exceptionhandler.c

if MINGW32
libexceptionhandler_a_SOURCES += exchndl.c
endif
