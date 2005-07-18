noinst_LIBRARIES = libnetplay.a
AM_CPPFLAGS = -I../framework -I../..
libnetplay_a_SOURCES = \
	netaudio_stub.c \
	netcrypt.c \
	netjoin_stub.c \
	netlobby_stub.c \
	netplay_stub.c \
	netprov_stub.c \
	netsupp_stub.c \
	netusers_stub.c \
	netlobby.h \
	netplay.h \
	netprov.h \
	netsupp.h

libnetplay_a_libs = ../framework/libframework.a
libnetplay_a_DEPENDENCIES = $(libnetplay_a_libs)
libnetplay_a_LIBADD = $(libnetplay_a_libs)
