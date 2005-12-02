AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
noinst_LIBRARIES = libgamelib.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)/lib/sound \
	-I$(top_srcdir)/lib/$(RENDERER) -I$(top_srcdir)/lib/ivis_common -I$(top_srcdir)
libgamelib_a_SOURCES = \
	anim.c \
	animobj.c \
	audp_lexer.l \
	audp_parser.y \
	gtime.c \
	hashtabl.c \
	pqueue.c \
	ptrlist.c \
	ani.h \
	animobj.h \
	hashtabl.h \
	parser.h \
	pqueue.h \
	ptrlist.h \
	anim.h \
	gtime.h \
	maxpidef.h \
	parser_y.h \
	priority.h

libgamelib_a_libs = ../framework/libframework.a \
		../$(RENDERER)/lib$(RENDERER).a \
		../ivis_common/libivis_common.a
libgamelib_a_DEPENDENCIES = $(libgamelib_a_libs)
libgamelib_a_LIBADD = $(libgamelib_a_libs)
