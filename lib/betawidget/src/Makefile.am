SUBDIRS = platform

%_wrap_lua.cpp: %.i
	$(SWIG) $(SWIG_LUA_OPT) -o $@ $<

EXTRA_DIST = \
	betawidget.i \
	betawidget.pc.in \
	lua_memberfnptr.i \
	lua_table_as_array.i

BUILT_SOURCES = \
	betawidget_wrap_lua.cpp
CLEANFILES = \
	betawidget_wrap_lua.cpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = betawidget.pc

lib_LTLIBRARIES = libbetawidget.la

libbetawidget_la_SOURCES = \
	betawidget_wrap_lua.cpp \
	button.c \
	imageButton.c \
	font.c \
	geom.c \
	hBox.c \
	spacer.c \
	svgManager.c \
	table.c \
	textEntry.c \
	patternManager.c \
	vector.c \
	widget.c \
	window.c

libbetawidget_la_CPPFLAGS = $(PANGO_CAIRO_CFLAGS) $(SVG_CAIRO_CFLAGS) $(CAIRO_CFLAGS) $(LUA_CFLAGS) -DLIB_COMPILE=1
libbetawidget_la_LDFLAGS = $(PANGO_CAIRO_LIBS) $(SVG_CAIRO_LIBS) $(CAIRO_CFLAGS) $(LUA_LIBS) $(CAIRO_LIBS) $(OPENGL_LIBS) -version-info $(LIBTOOL_VERSION) -DLIB_COMPILE=1
libbetawidget_la_LIBADD = platform/libplatform.la libutil.la

# FIXME: As much non-API headers (i.e. headers that shouldn't be directly
#        included by client code of betawidget) as possible should be moved to
#        noinst_HEADERS.
pkginclude_HEADERS = \
	button.h \
	imageButton.h \
	clipboard.h \
	font.h \
	internal-cairo.h \
	keycode.h \
	geom.h \
	hBox.h \
	spacer.h \
	svgManager.h \
	table.h \
	textEntry.h \
	patternManager.h \
	vector.h \
	widget.h \
	window.h

noinst_HEADERS = \
	lua-wrap.h

# utf.c, GLee
noinst_LTLIBRARIES = libutil.la
libutil_la_SOURCES = \
	../../framework/utf.c \
	../../ivis_opengl/GLee.c

libutil_la_CPPFLAGS = $(SDL_CFLAGS) -DLIB_COMPILE=1
noinst_HEADERS += \
	../../framework/types.h \
	../../framework/utf.h \
	../../ivis_opengl/GLee.h

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
