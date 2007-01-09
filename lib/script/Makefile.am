%.tab.h %.tab.c: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
noinst_LIBRARIES = libscript.a
AM_CPPFLAGS = -I$(top_srcdir)
BUILT_SOURCES = script_parser.tab.h chat_parser.tab.h
libscript_a_SOURCES = \
	script_lexer.lex.c \
	script_parser.tab.c \
	chat_lexer.lex.c \
	chat_parser.tab.c \
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
	chat_processing.h \
	stack.h

libscript_a_libs = ../framework/libframework.a
libscript_a_DEPENDENCIES = $(libscript_a_libs)
libscript_a_LIBADD = $(libscript_a_libs)
