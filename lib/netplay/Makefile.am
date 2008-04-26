AM_CPPFLAGS = $(SDL_CFLAGS) $(SDL_NET_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)

noinst_LIBRARIES = libnetplay.a
noinst_HEADERS = netlog.h netplay.h nettypes.h

libnetplay_a_SOURCES = netjoin_stub.c netplay.c nettypes.c netlog.c

libnetplay_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
