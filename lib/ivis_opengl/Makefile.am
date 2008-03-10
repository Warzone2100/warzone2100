AM_CPPFLAGS = $(SDL_CFLAGS) $(PNG_CFLAGS) $(OPENGLC_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)

noinst_LIBRARIES = libivis_opengl.a
noinst_HEADERS = piematrix.h screen.h

libivis_opengl_a_SOURCES = ivi.c pieblitfunc.c piedraw.c piefunc.c \
	piematrix.c piemode.c piepalette.c piestate.c rendmode.c \
	screen.c tex.c textdraw.c

libivis_opengl_a_LIBADD = $(top_builddir)/lib/ivis_common/libivis_common.a \
	$(top_builddir)/lib/framework/libframework.a
