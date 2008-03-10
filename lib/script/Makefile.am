%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(WZ_CPPFLAGS)
AM_CFLAGS = $(SDL_CFLAGS) $(WZ_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = script_lexer.lex.c chat_lexer.lex.c script_parser.tab.c chat_parser.tab.c script_parser.tab.h chat_parser.tab.h
CLEANFILES = script_lexer.lex.c chat_lexer.lex.c script_parser.tab.c chat_parser.tab.c script_parser.tab.h chat_parser.tab.h

noinst_LIBRARIES = libscript.a
noinst_HEADERS = chat_processing.h codeprint.h event.h evntsave.h interp.h \
	parse.h script.h stack.h

libscript_a_SOURCES = script_lexer.lex.c script_parser.tab.c chat_lexer.lex.c \
	chat_parser.tab.c codeprint.c event.c evntsave.c interp.c script.c stack.c

libscript_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
