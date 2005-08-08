noinst_LIBRARIES = libivis_common.a
AM_CPPFLAGS = -I../framework -I../gamelib -I../..
libivis_common_a_SOURCES = \
	bug.c \
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
	pcx.h \
	pieblitfunc.h \
	pieclip.h \
	piedef.h \
	piefunc.h \
	piemode.h \
	piepalette.h \
	piestate.h \
	pietypes.h \
	rendfunc.h \
	textdraw.h

libivis_common_a_libs = ../framework/libframework.a
libivis_common_a_DEPENDENCIES = $(libivis_common_a_libs)
libivis_common_a_LIBADD = $(libivis_common_a_libs)
