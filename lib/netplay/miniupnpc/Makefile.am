AM_CPPFLAGS = -DSTATICLIB $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = -DSTATICLIB $(WZ_CFLAGS)

noinst_LIBRARIES = libminiupnpc.a
noinst_HEADERS = \
	miniupnpc.h \
	miniupnpcstrings.h \
	miniwget.h \
	minixml.h \
	minisoap.h \
	minissdpc.h \
	codelength.h \
	upnpcommands.h \
	igd_desc_parse.h \
	upnpreplyparse.h \
	upnperrors.h \
	declspec.h \
	bsdqueue.h

libminiupnpc_a_SOURCES = \
	miniwget.c \
	minixml.c \
	igd_desc_parse.c \
	minisoap.c \
	miniupnpc.c \
	upnpreplyparse.c \
	upnpcommands.c \
	minissdpc.c \
	upnperrors.c

EXTRA_DIST = LICENCE README
