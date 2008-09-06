CPPFLAGS=$(CAIRO_SVG_CFLAGS) $(SDL_CFLAGS)
LIBS=$(CAIRO_SVG_LIBS) $(SDL_LIBS)

lib_LTLIBRARIES = libbetawidget.la

libbetawidget_la_SOURCES = font.c geom.c hBox.c spacer.c textEntry.c vector.c \
	widget.c window.c

# FIXME: Headers required for the public API should be placed in the
#        inlude_HEADERS directory.
noinst_HEADERS = clipboard.h font.h geom.h hBox.h keycode.h spacer.h \
	textEntry.h vector.h widget.h window.h

PLATFORM=platform/sdl

libbetawidget_la_SOURCES += $(PLATFORM)/event.c $(PLATFORM)/init.c \
	$(PLATFORM)/time.c

noinst_HEADERS += $(PLATFORM)/event.h $(PLATFORM)/init.h
