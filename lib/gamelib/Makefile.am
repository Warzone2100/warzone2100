%.tab.hpp %.tab.cpp:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.cpp:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = \
	audp_lexer.lex.cpp \
	audp_parser.tab.cpp \
	audp_parser.tab.hpp

CLEANFILES = \
	audp_lexer.lex.cpp \
	audp_parser.tab.cpp \
	audp_parser.tab.hpp

EXTRA_DIST = \
	audp_lexer.l audp_parser.y

noinst_LIBRARIES = libgamelib.a
noinst_HEADERS = \
	audp_parser.tab.hpp \
	anim.h \
	animobj.h \
	gtime.h \
	hashtable.h \
	parser.h

libgamelib_a_SOURCES = \
	anim.cpp \
	animobj.cpp \
	audp_lexer.lex.cpp \
	audp_parser.tab.cpp \
	gtime.cpp \
	hashtable.cpp
