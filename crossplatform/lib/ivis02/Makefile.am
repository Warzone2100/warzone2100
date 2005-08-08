noinst_LIBRARIES = libivis02.a
AM_CPPFLAGS = -I../framework -I../gamelib -I../..
libivis02_a_SOURCES = \
	bitimage.c \
	bspimd.c \
	bug.c \
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
	v4101_stub.c \
	vsr.c \
	bspimd.h    fbf.h        pcx.h          piemode.h     rendmode.h  vid.h \
	bug.h       geo.h        pieblitfunc.h  piepalette.h  screen.h    vsr.h \
	imd.h       pieclip.h    piestate.h    \
	ivi.h       piedef.h     pietexture.h tex.h \
	bitimage.h  dglide.h     ivisdef.h    piefunc.h      pietypes.h    textdraw.h \
	bspfunc.h   ivispatch.h  piematrix.h  rendfunc.h     v4101.h

libivis02_a_libs = ../framework/libframework.a
libivis02_a_DEPENDENCIES = $(libivis02_a_libs)
libivis02_a_LIBADD = $(libivis02_a_libs)
