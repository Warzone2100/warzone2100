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
	bitimage.cpp \
	imd.cpp \
	imdload.cpp \
	jpeg_encoder.cpp \
	pieclip.cpp \
	piestate.cpp \
	png_util.cpp

libivis_common_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
