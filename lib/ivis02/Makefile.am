noinst_LIBRARIES = libivis02.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)/lib/gamelib \
	-I$(top_srcdir) -I$(top_srcdir)/lib/ivis_common
libivis02_a_SOURCES = \
	bspimd.c \
	ivi.c \
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
	vid.h \
	screen.h \
	vsr.h \
	pietexture.h \
	piematrix.h \
	v4101.h

libivis02_a_libs = ../framework/libframework.a ../ivis_common/libivis_common.a
libivis02_a_DEPENDENCIES = $(libivis02_a_libs)
libivis02_a_LIBADD = $(libivis02_a_libs)
