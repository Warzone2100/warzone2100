SUBDIRS = miniupnpc

AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libnetplay.a
noinst_HEADERS = \
	netlog.h \
	netplay.h \
	netqueue.h \
	netsocket.h \
	nettypes.h

libnetplay_a_SOURCES = \
	netjoin_stub.cpp \
	netlog.cpp \
	netplay.cpp \
	netqueue.cpp \
	netsocket.cpp \
	nettypes.cpp

libnetplay_a_LIBADD = $(top_builddir)/lib/framework/libframework.a $(top_builddir)/lib/netplay/miniupnpc/libminiupnpc.a
