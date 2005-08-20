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
	3dfxfunc.h  bspimd.h     fbf.h        pcx.h          piemode.h     rendmode.h  vid.h \
	3dfxmode.h  bug.h        geo.h        pieblitfunc.h  piepalette.h  screen.h    vsr.h \
	3dfxtext.h  d3dmode.h    imd.h        pieclip.h      piestate.h    texd3d.h \
	amd3d.h     d3drender.h  ivi.h        piedef.h       pietexture.h  tex.h \
	bitimage.h  dglide.h     ivisdef.h    piefunc.h      pietypes.h    textdraw.h \
	bspfunc.h   dx6texman.h  ivispatch.h  piematrix.h    rendfunc.h    v4101.h

libivis02_a_libs = ../framework/libframework.a
libivis02_a_DEPENDENCIES = $(libivis02_a_libs)
libivis02_a_LIBADD = $(libivis02_a_libs)
