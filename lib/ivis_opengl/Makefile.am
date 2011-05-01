AM_CPPFLAGS = $(SDL_CFLAGS) $(PNG_CFLAGS) $(OPENGLC_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) $(GLee_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) -Wno-missing-declarations
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libivis_opengl.a
noinst_HEADERS = \
	piematrix.h \
	screen.h

libivis_opengl_a_SOURCES = \
	ivi.c \
	pieblitfunc.c \
	piedraw.c \
	piefunc.c \
	piematrix.c \
	piemode.c \
	piepalette.c \
	piestate.c \
	rendmode.c \
	screen.c \
	tex.c \
	textdraw.c

if !SYSTEM_GLEE
libivis_opengl_a_SOURCES += GLee.c
noinst_HEADERS += GLee.h
endif

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
