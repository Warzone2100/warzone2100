AM_CPPFLAGS = $(SDL_CFLAGS) $(PNG_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libivis_common.a
noinst_HEADERS = \
	bitimage.h \
	imd.h \
	ivi.h \
	ivisdef.h \
	jpeg_encoder.h \
	pieblitfunc.h \
	pieclip.h \
	piedef.h \
	piefunc.h \
	piemode.h \
	piepalette.h \
	piestate.h \
	pietypes.h \
	png_util.h \
	rendmode.h \
	tex.h \
	textdraw.h

libivis_common_a_SOURCES = \
	bitimage.c \
	imd.c \
	imdload.c \
	jpeg_encoder.c \
	pieclip.c \
	piestate.c \
	png_util.c

libivis_common_a_LIBADD = $(top_builddir)/lib/framework/libframework.a

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
