if MINGW32
WZ_CPPFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CXXFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
endif

AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) $(WZ_C99FLAGS) -UDEBUG

noinst_LIBRARIES = libmicroecc.a
noinst_HEADERS = \
	curve-specific.inc \
	platform-specific.inc \
	types.h \
	uECC_vli.h \
	uECC.h

libmicroecc_a_SOURCES = \
	uECC.c

EXTRA_DIST = \
	LICENSE.txt \
	README.md
