noinst_LIBRARIES = libivis_common.a
noinst_HEADERS = bitimage.h imd.h ivi.h ivisdef.h \
	ivispatch.h pieblitfunc.h pieclip.h piedef.h piefunc.h piemode.h piepalette.h \
	piestate.h pietypes.h png_util.h rendmode.h tex.h textdraw.h

libivis_common_a_SOURCES = bitimage.c imd.c imdload.c png_util.c pieclip.c \
	piestate.c

libivis_common_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
libivis_common_a_CFLAGS = $(SDL_CFLAGS)
