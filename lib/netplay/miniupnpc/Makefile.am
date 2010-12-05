AM_CPPFLAGS = -DSTATICLIB $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = -DSTATICLIB $(WZ_CFLAGS)

noinst_LIBRARIES = libminiupnpc.a
noinst_HEADERS = \
	miniupnpc.h \
	miniupnpcstrings.h \
	miniwget.h \
	minixml.h \
	minisoap.h \
	minissdpc.h \
	codelength.h \
	upnpcommands.h \
	igd_desc_parse.h \
	upnpreplyparse.h \
	upnperrors.h \
	declspec.h \
	bsdqueue.h

libminiupnpc_a_SOURCES = \
	miniwget.c \
	minixml.c \
	igd_desc_parse.c \
	minisoap.c \
	miniupnpc.c \
	upnpreplyparse.c \
	upnpcommands.c \
	minissdpc.c \
	upnperrors.c

EXTRA_DIST = LICENCE README

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
