if MINGW32
WZ_CPPFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB -DMINIUPNP_STATICLIB
WZ_CFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB -DMINIUPNP_STATICLIB
WZ_CXXFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB -DMINIUPNP_STATICLIB
endif

AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) -fno-strict-aliasing -UDEBUG

noinst_LIBRARIES = libminiupnpc.a
noinst_HEADERS = \
	codelength.h \
	connecthostport.h \
	igd_desc_parse.h \
	minisoap.h \
	minissdpc.h \
	miniupnpc.h \
	miniupnpc_declspec.h \
	miniupnpcstrings.h \
	miniupnpctypes.h \
	miniwget.h \
	minixml.h \
	portlistingparse.h \
	receivedata.h \
	upnpcommands.h \
	upnpdev.h \
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
	upnpdev.c \
	upnperrors.c \
	upnpreplyparse.c

EXTRA_DIST = \
	Changelog.txt \
	LICENSE \
	README
