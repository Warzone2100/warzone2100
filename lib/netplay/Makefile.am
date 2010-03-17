SUBDIRS = miniupnpc

AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)

noinst_LIBRARIES = libnetplay.a
noinst_HEADERS = \
	netlog.h \
	netplay.h \
	netsocket.h \
	nettypes.h

libnetplay_a_SOURCES = \
	netjoin_stub.c \
	netlog.c \
	netplay.c \
	netsocket.cpp \
	nettypes.c

libnetplay_a_LIBADD = $(top_builddir)/lib/framework/libframework.a $(top_builddir)/lib/netplay/miniupnpc/libminiupnpc.a

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
