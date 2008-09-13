SUBDIRS = platform

%_wrap_lua.cpp: %.i
	$(SWIG) $(SWIG_LUA_OPT) -o $@ $<

BUILT_SOURCES = betawidget_wrap_lua.cpp
CLEANFILES = betawidget_wrap_lua.cpp

lib_LTLIBRARIES = libbetawidget.la

libbetawidget_la_SOURCES = betawidget_wrap_lua.cpp font.c geom.c hBox.c \
	spacer.c textEntry.c vector.c widget.c window.c

libbetawidget_la_CPPFLAGS = $(CAIRO_SVG_CFLAGS) $(LUA_CFLAGS) -DLIB_COMPILE=1
libbetawidget_la_LDFLAGS = $(CAIRO_SVG_LIBS) $(OPENGL_LIBS)
libbetawidget_la_LIBADD = $(LUA_LIBTOOL) platform/libplatform.la libutil.la

# FIXME: As much non-API headers (i.e. headers that shouldn't be directly
#        included by client code of betawidget) as possible should be moved to
#        noinst_HEADERS.
pkginclude_HEADERS = clipboard.h font.h internal-cairo.h keycode.h geom.h \
	hBox.h spacer.h textEntry.h vector.h widget.h window.h
noinst_HEADERS = 

# utf.c, GLee
noinst_LTLIBRARIES = libutil.la
libutil_la_SOURCES = ../framework/utf.c ../ivis_opengl/GLee.c
libutil_la_CPPFLAGS = $(SDL_CFLAGS) -DLIB_COMPILE=1
noinst_HEADERS += ../framework/types.h ../framework/utf.h ../ivis_opengl/GLee.h

# Platform selection
PLATFORM=sdl

noinst_PROGRAMS = tools/sdl-testapp

tools_sdl_testapp_SOURCES = tools/sdl-testapp.c
tools_sdl_testapp_CPPFLAGS = -I.. $(LUA_CFLAGS) $(SDL_CFLAGS)
tools_sdl_testapp_LDADD = $(OPENGL_LIBS) $(SDL_LIBS) libbetawidget.la $(LUA_LIBTOOL)
