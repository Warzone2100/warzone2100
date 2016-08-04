AM_CPPFLAGS = $(VORBISFILE_CFLAGS) $(VORBIS_CFLAGS) $(OPENAL_CFLAGS) $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS) $(QT5_CFLAGS)

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
