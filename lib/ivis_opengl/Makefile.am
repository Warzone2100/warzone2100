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

libivis_opengl_a_LIBADD = $(top_builddir)/lib/ivis_common/libivis_common.a \
	$(top_builddir)/lib/framework/libframework.a
