SUBDIRS = miniupnpc

AM_CPPFLAGS = -DSTATICLIB $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = -DSTATICLIB $(WZ_CFLAGS)
AM_CXXFLAGS = -DSTATICLIB $(WZ_CXXFLAGS) $(QT4_CFLAGS)

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
