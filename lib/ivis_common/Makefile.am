AM_CPPFLAGS = -I$(top_srcdir)

noinst_LIBRARIES = libivis_common.a
noinst_HEADERS = bitimage.h bspfunc.h bspimd.h bug.h imd.h ivi.h ivisdef.h \
	ivispatch.h pieblitfunc.h pieclip.h piedef.h piefunc.h piemode.h piepalette.h \
	piestate.h pietypes.h rendfunc.h rendmode.h tex.h textdraw.h

libivis_common_a_SOURCES = bitimage.c bug.c imd.c imdload.c pcx.c pieclip.c \
	piestate.c

libivis_common_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
