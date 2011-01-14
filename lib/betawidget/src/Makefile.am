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
	button.cpp \
	imageButton.cpp \
	font.cpp \
	geom.cpp \
	hBox.cpp \
	spacer.cpp \
	svgManager.cpp \
	table.cpp \
	textEntry.cpp \
	patternManager.cpp \
	vector.cpp \
	widget.cpp \
	window.cpp

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

# utf.cpp, GLee
noinst_LTLIBRARIES = libutil.la
libutil_la_SOURCES = \
	../../framework/utf.cpp \
	../../ivis_opengl/GLee.c

libutil_la_CPPFLAGS = $(SDL_CFLAGS) -DLIB_COMPILE=1
noinst_HEADERS += \
	../../framework/types.h \
	../../framework/utf.h \
	../../ivis_opengl/GLee.h
