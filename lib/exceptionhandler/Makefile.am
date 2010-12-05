AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) $(WZ_C99FLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)
noinst_LIBRARIES = libexceptionhandler.a
noinst_HEADERS = \
	dumpinfo.h \
	exceptionhandler.h \
	exchndl.h \
	include/coff/internal.h \
	include/demangle.h \
	include/libcoff.h

libexceptionhandler_a_SOURCES = \
	dumpinfo.cpp \
	exceptionhandler.c

if MINGW32
libexceptionhandler_a_SOURCES += exchndl.c
endif

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
