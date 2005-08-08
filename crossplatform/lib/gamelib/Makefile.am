noinst_LIBRARIES = libgamelib.a
AM_CPPFLAGS = -I../framework -I../sound -I../$(RENDERER) -I../ivis_common -I../..
libgamelib_a_SOURCES = \
	anim.c \
	animobj.c \
	gtime.c \
	hashtabl.c \
	parser_l.c \
	parser_y.c \
	pqueue.c \
	ptrlist.c \
	ani.h   animobj.h  hashtabl.h  parser.h    pqueue.h    ptrlist.h \
	anim.h  gtime.h    maxpidef.h  parser_y.h  priority.h

libgamelib_a_libs = ../framework/libframework.a \
		../$(RENDERER)/lib$(RENDERER).a \
		../ivis_common/libivis_common.a
libgamelib_a_DEPENDENCIES = $(libgamelib_a_libs)
libgamelib_a_LIBADD = $(libgamelib_a_libs)
