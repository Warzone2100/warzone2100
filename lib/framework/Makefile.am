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
	resource_lexer.lex.cpp \
	resource_parser.tab.cpp \
	resource_parser.tab.hpp \
	strres_lexer.lex.cpp \
	strres_parser.tab.cpp \
	strres_parser.tab.hpp

CLEANFILES = \
	resource_lexer.lex.cpp \
	resource_parser.tab.cpp \
	resource_parser.tab.hpp \
	strres_lexer.lex.cpp \
	strres_parser.tab.cpp \
	strres_parser.tab.hpp

EXTRA_DIST = \
	resource_lexer.l resource_parser.y \
	strres_lexer.l strres_parser.y

noinst_LIBRARIES = libframework.a
noinst_HEADERS = \
	strres_parser.tab.hpp \
	resource_parser.tab.hpp \
	configfile.h \
	crc.h \
	cursors.h \
	debug.h \
	file.h \
	fixedpoint.h \
	frame.h \
	frameint.h \
	frameresource.h \
	i18n.h \
	input.h \
	lexer_input.h \
	listmacs.h \
	math_ext.h \
	opengl.h \
	physfs_ext.h \
	resly.h \
	stdio_ext.h \
	string_ext.h \
	strres.h \
	strresly.h \
	tagfile.h \
	treap.h \
	trig.h \
	types.h \
	utf.h \
	vector.h \
	config-macosx.h \
	endian_hack.h \
	gettext.h \
	macros.h \
	SDL_framerate.h \
	wzapp_c.h \
	wzglobal.h

libframework_a_SOURCES = \
	resource_lexer.lex.cpp \
	resource_parser.tab.cpp \
	strres_lexer.lex.cpp \
	strres_parser.tab.cpp \
	configfile.cpp \
	crc.cpp \
	cursors16.cpp \
	cursors32.cpp \
	cursors.cpp \
	debug.cpp \
	frame.cpp \
	frameresource.cpp \
	i18n.cpp \
	input.cpp \
	lexer_input.cpp \
	SDL_framerate.cpp \
	stdio_ext.cpp \
	strres.cpp \
	tagfile.cpp \
	treap.cpp \
	trig.cpp \
	utf.cpp
