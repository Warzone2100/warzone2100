AM_CPPFLAGS = -DSTATICLIB $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = -DSTATICLIB $(WZ_CFLAGS) -UDEBUG

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
	miniwget.h \
	minixml.h \
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
	upnpcommands.c \
	upnperrors.c \
	upnpreplyparse.c

EXTRA_DIST = \
	Changelog.txt \
	LICENCE \
	README
