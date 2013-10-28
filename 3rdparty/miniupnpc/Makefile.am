if MINGW32
WZ_CPPFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CXXFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
endif

AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) -fno-strict-aliasing -UDEBUG

noinst_LIBRARIES = libminiupnpc.a
noinst_HEADERS = \
	bsdqueue.h \
	codelength.h \
	connecthostport.h \
	declspec.h \
	igd_desc_parse.h \
	minisoap.h \
	minissdpc.h \
	miniupnpc.h \
	miniupnpcstrings.h \
	miniupnpctypes.h \
	miniwget.h \
	minixml.h \
	portlistingparse.h \
	receivedata.h \
	upnpcommands.h \
	upnperrors.h \
	upnpreplyparse.h

libminiupnpc_a_SOURCES = \
	connecthostport.c \
	igd_desc_parse.c \
	minisoap.c \
	minissdpc.c \
	miniupnpc.c \
	miniwget.c \
	minixml.c \
	portlistingparse.c \
	receivedata.c \
	upnpcommands.c \
	upnperrors.c \
	upnpreplyparse.c

EXTRA_DIST = \
	Changelog.txt \
	LICENSE \
	README
