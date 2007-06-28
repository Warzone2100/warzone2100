noinst_LIBRARIES = libivis_common.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)/lib/gamelib -I$(top_srcdir)
libivis_common_a_SOURCES = \
	bitimage.c \
	bug.c \
	fbf.c \
	imd.c \
	imdload.c \
	pcx.c \
	bitimage.h \
	bspfunc.h \
	bspimd.h \
	bug.h \
	fbf.h \
	geo.h \
	imd.h \
	ivi.h \
	ivisdef.h \
	ivispatch.h \
	pieblitfunc.h \
	pieclip.h \
	piedef.h \
	piefunc.h \
	piemode.h \
	piepalette.h \
	piestate.h \
	pietypes.h \
	rendfunc.h \
	rendmode.h \
	tex.h \
	textdraw.h

libivis_common_a_libs = ../framework/libframework.a
libivis_common_a_DEPENDENCIES = $(libivis_common_a_libs)
libivis_common_a_LIBADD = $(libivis_common_a_libs)
