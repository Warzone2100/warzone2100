AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)

noinst_LIBRARIES = libwidget.a
noinst_HEADERS = \
	bar.h \
	button.h \
	editbox.h \
	form.h \
	label.h \
	scrap.h \
	slider.h \
	tip.h \
	widgbase.h \
	widget.h \
	widgint.h

libwidget_a_SOURCES = \
	bar.c \
	button.c \
	editbox.c \
	form.c \
	label.c \
	scrap.c \
	slider.c \
	tip.c \
	widget.c

libwidget_a_LIBADD = $(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a $(top_builddir)/lib/framework/libframework.a
