AM_CPPFLAGS = $(SDL_CFLAGS) $(THEORA_CFLAGS) $(WZ_CPPFLAGS) $(GLee_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libsequence.a
noinst_HEADERS = \
	sequence.h \
	timer.h

libsequence_a_SOURCES = \
	sequence.c \
	timer.c

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
