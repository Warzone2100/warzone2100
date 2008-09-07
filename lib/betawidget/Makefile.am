lib_LTLIBRARIES = libbetawidget.la

libbetawidget_la_SOURCES = font.c geom.c hBox.c spacer.c textEntry.c vector.c \
	widget.c window.c

libbetawidget_la_CPPFLAGS = $(CAIRO_SVG_CFLAGS) -DLIB_COMPILE=1
libbetawidget_la_LDFLAGS = $(CAIRO_SVG_LIBS) $(OPENGL_LIBS)

# FIXME: Headers required for the public API should be placed in the
#        inlude_HEADERS directory.
noinst_HEADERS = clipboard.h font.h geom.h hBox.h internal-cairo.h keycode.h \
	spacer.h textEntry.h vector.h widget.h window.h

# utf.c, GLee
libbetawidget_la_SOURCES += ../framework/utf.c ../ivis_opengl/GLee.c
noinst_HEADERS += ../framework/types.h ../framework/utf.h ../ivis_opengl/GLee.h

# Platform selection
PLATFORM=platform/sdl

CPPFLAGS += $(SDL_CFLAGS)
LIBS += $(SDL_LIBS)

libbetawidget_la_SOURCES += $(PLATFORM)/event.c $(PLATFORM)/init.c \
	$(PLATFORM)/time.c

if MINGW32
libbetawidget_la_SOURCES += $(PLATFORM)/clipboardWin32.c
else
if MACOSX
libbetawidget_la_SOURCES += $(PLATFORM)/clipboardOSX.m
else
# Assume X11 if none of the above match
libbetawidget_la_SOURCES += $(PLATFORM)/clipboardX11.c
endif
endif

noinst_HEADERS += $(PLATFORM)/event.h $(PLATFORM)/init.h

noinst_PROGRAMS = tools/sdl-testapp

tools_sdl_testapp_SOURCES = tools/sdl-testapp.c
tools_sdl_testapp_CPPFLAGS = -I..
tools_sdl_testapp_LDADD = $(OPENGL_LIBS) libbetawidget.la
