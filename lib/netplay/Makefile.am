AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT4_CFLAGS) $(QJSON_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

if MINGW32
WZ_CPPFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
WZ_CXXFLAGS += -D_WIN32_WINNT=0x0501 -DSTATICLIB
endif

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
