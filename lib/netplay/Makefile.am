SUBDIRS = \
	miniupnpc \
	bson

if MINGW32
WZ_CPPFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CXXFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
endif

AM_CPPFLAGS = $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

noinst_LIBRARIES = libnetplay.a
noinst_HEADERS = \
	netlobby.h \
	netlog.h \
	netplay.h \
	netqueue.h \
	netsocket.h \
	nettypes.h

libnetplay_a_SOURCES = \
	netsocket.cpp \
	netlobby.cpp \
	netlog.cpp \
	netplay.cpp \
	netqueue.cpp \
	nettypes.cpp
