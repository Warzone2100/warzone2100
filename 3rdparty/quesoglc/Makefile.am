AM_CPPFLAGS = $(FREETYPE_CFLAGS) $(FRIBIDI_CFLAGS) $(FONTCONFIG_CFLAGS) $(WZ_CPPFLAGS) $(OPENGL_CFLAGS) $(GLEW_CFLAGS)
AM_CFLAGS =
AM_CXXFLAGS =

AUTOMAKE_OPTIONS = subdir-objects

WZ_CPPFLAGS += -I`pwd`
WZ_CFLAGS += -I`pwd`
WZ_CXXFLAGS += -I`pwd`

noinst_LIBRARIES = libquesoglc.a
noinst_HEADERS = \
	GL/glc.h \
	except.h \
	internal.h \
	oarray.h \
	ocharmap.h \
	ocontext.h \
	ofacedesc.h \
	ofont.h \
	oglyph.h \
	omaster.h \
	qglc_config.h \
	texture.h

libquesoglc_a_SOURCES = \
	database.c \
	context.c \
	except.c \
	font.c \
	global.c \
	master.c \
	measure.c \
	misc.c \
	oarray.c \
	ofacedesc.c \
	ofont.c \
	oglyph.c \
	render.c \
	scalable.c \
	texture.c \
	transform.c \
	unicode.c


if MINGW32
libquesoglc_a_SOURCES += win32/ocharmap.c win32/ocontext.c win32/omaster.c
else
libquesoglc_a_SOURCES += ocharmap.c ocontext.c omaster.c
endif
