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
	audio.c \
	audio_id.c \
	cdaudio.c \
	oggvorbis.c \
	openal_error.c \
	openal_track.c \
	playlist.c \
	track.c

libsound_a_LIBADD = $(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
