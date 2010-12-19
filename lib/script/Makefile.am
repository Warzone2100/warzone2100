%.tab.hpp %.tab.cpp:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.cpp:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = \
	chat_lexer.lex.cpp \
	chat_parser.tab.cpp \
	chat_parser.tab.hpp \
	script_lexer.lex.cpp \
	script_parser.tab.cpp \
	script_parser.tab.hpp

CLEANFILES = \
	chat_lexer.lex.cpp \
	chat_parser.tab.cpp \
	chat_parser.tab.hpp \
	script_lexer.lex.cpp \
	script_parser.tab.cpp \
	script_parser.tab.hpp

EXTRA_DIST = \
	chat_lexer.l chat_parser.y \
	script_lexer.l script_parser.y

noinst_LIBRARIES = libscript.a
noinst_HEADERS = \
	chat_parser.tab.hpp \
	script_parser.tab.hpp \
	chat_processing.h \
	codeprint.h \
	event.h \
	eventsave.h \
	interpreter.h \
	parse.h \
	script.h \
	stack.h

libscript_a_SOURCES = \
	chat_lexer.lex.cpp \
	chat_parser.tab.cpp \
	codeprint.cpp \
	event.cpp \
	eventsave.cpp \
	interpreter.cpp \
	script.cpp \
	script_lexer.lex.cpp \
	script_parser.tab.cpp \
	stack.cpp

libscript_a_LIBADD = $(top_builddir)/lib/framework/libframework.a
