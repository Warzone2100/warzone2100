noinst_LIBRARIES = libnetplay.a
AM_CPPFLAGS = -I$(top_srcdir)
libnetplay_a_SOURCES = \
	netaudio_stub.c \
	netcrypt.c \
	netjoin_stub.c \
	netplay.c \
	netlog.c \
	netusers_stub.c \
	netplay.h \
	netlog.h

libnetplay_a_libs = ../framework/libframework.a
libnetplay_a_DEPENDENCIES = $(libnetplay_a_libs)
libnetplay_a_LIBADD = $(libnetplay_a_libs)
