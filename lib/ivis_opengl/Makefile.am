AM_CPPFLAGS = $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(FONT_CFLAGS) $(WZ_CPPFLAGS) $(GLEW_CFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) -Wno-missing-declarations
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS)

noinst_LIBRARIES = libivis_opengl.a
noinst_HEADERS = \
	piematrix.h \
	screen.h \
	bitimage.h \
	imd.h \
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
	tex.h \
	textdraw.h

libivis_opengl_a_SOURCES = \
	pieblitfunc.cpp \
	piedraw.cpp \
	piefunc.cpp \
	piematrix.cpp \
	piemode.cpp \
	piepalette.cpp \
	piestate.cpp \
	screen.cpp \
	tex.cpp \
	textdraw.cpp \
	bitimage.cpp \
	imdload.cpp \
	jpeg_encoder.cpp \
	pieclip.cpp \
	png_util.cpp
