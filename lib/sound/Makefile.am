AM_CPPFLAGS = $(SDL_CFLAGS) $(OGGVORBIS_CFLAGS) $(OPENAL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libsound.a
noinst_HEADERS = \
	aud.h \
	audio.h \
	audio_id.h \
	cdaudio.h \
	mixer.h \
	playlist.h \
	oggvorbis.h \
	openal_error.h \
	track.h \
	tracklib.h

libsound_a_SOURCES = \
	audio.cpp \
	audio_id.cpp \
	cdaudio.cpp \
	oggvorbis.cpp \
	openal_error.cpp \
	openal_track.cpp \
	playlist.cpp \
	track.cpp

libsound_a_LIBADD = $(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a
