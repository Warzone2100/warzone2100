AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
noinst_LIBRARIES = libscript.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)
BUILT_SOURCES = script_parser.h
libscript_a_SOURCES = \
	script_lexer.l \
	script_parser.y \
	codeprint.c \
	event.c \
	evntsave.c \
	interp.c \
	script.c \
	stack.c \
	codeprint.h \
	event.h \
	evntsave.h \
	interp.h \
	parse.h \
	script.h \
	stack.h

libscript_a_libs = ../framework/libframework.a
libscript_a_DEPENDENCIES = $(libscript_a_libs)
libscript_a_LIBADD = $(libscript_a_libs)
