%.tab.h %.tab.c: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
noinst_LIBRARIES = libgamelib.a
AM_CPPFLAGS = -I$(top_srcdir)
BUILT_SOURCES = audp_parser.tab.h
libgamelib_a_SOURCES = \
	anim.c \
	animobj.c \
	audp_lexer.lex.c \
	audp_parser.tab.c \
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
