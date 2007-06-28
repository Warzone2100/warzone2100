noinst_LIBRARIES = libsound.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)/lib/gamelib -I$(top_srcdir)
libsound_a_SOURCES = \
	audio.c \
	cdaudio.c \
	mixer_stub.c \
	openal_track.c \
	playlist.c \
	track.c \
	aud.h \
	audio.h \
	cdaudio.h \
	mixer.h \
	track.h \
	tracklib.h

libsound_a_libs = ../gamelib/libgamelib.a \
                  ../framework/libframework.a
libsound_a_DEPENDENCIES = $(libsound_a_libs)
libsound_a_LIBADD = $(libsound_a_libs)
