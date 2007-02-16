AM_CPPFLAGS = -I$(top_srcdir)

noinst_LIBRARIES = libnetplay.a
noinst_HEADERS = netlog.h netplay.h

libnetplay_a_SOURCES = netaudio_stub.c netcrypt.c netjoin_stub.c netplay.c \
	netlog.c netusers_stub.c

libnetplay_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
