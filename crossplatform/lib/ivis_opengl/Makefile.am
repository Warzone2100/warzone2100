noinst_LIBRARIES = libivis_opengl.a
AM_CPPFLAGS = -I../framework -I../gamelib -I../.. -I../ivis_common
libivis_opengl_a_SOURCES = \
	bitimage.c \
	bspimd.c \
	fbf.c \
	imd.c \
	imdload.c \
	ivi.c \
	pcx.c \
	pieblitfunc.c \
	pieclip.c \
	piedraw.c \
	piefunc.c \
	piematrix.c \
	piemode.c \
	piepalette.c \
	piestate.c \
	pietexture.c \
	rendfunc.c \
	rendmode.c \
	screen.c \
	tex.c \
	textdraw.c \
	ivis02.h \
	piematrix.h \
	pietexture.h \
	screen.h \
	tex.h \
	rendmode.h \
	vid.h

libivis_opengl_a_libs = ../framework/libframework.a ../ivis_common/libivis_common.a
libivis_opengl_a_DEPENDENCIES = $(libivis_opengl_a_libs)
libivis_opengl_a_LIBADD = $(libivis_opengl_a_libs)
