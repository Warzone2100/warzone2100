%.tab.hpp %.tab.cpp:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.cpp:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

MOCHEADER = wzapp.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC4) -o $@ $<

BUILT_SOURCES = \
	$(MOCEDFILES) \
	resource_lexer.lex.cpp \
	resource_parser.tab.cpp \
	resource_parser.tab.hpp \
	strres_lexer.lex.cpp \
	strres_parser.tab.cpp \
	strres_parser.tab.hpp

CLEANFILES = \
	$(MOCEDFILES) \
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
	$(MOCHEADER) \
	wzapp_c.h \
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
	geometry.h \
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
	treap.h \
	trig.h \
	types.h \
	utf.h \
	vector.h \
	config-macosx.h \
	endian_hack.h \
	gettext.h \
	macros.h \
	wzapp_c.h \
	wzfs.h \
	wzglobal.h

libframework_a_SOURCES = \
	wzapp_moc.cpp \
	wzapp.cpp \
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
	geometry.cpp \
	i18n.cpp \
	lexer_input.cpp \
	stdio_ext.cpp \
	strres.cpp \
	treap.cpp \
	trig.cpp \
	utf.cpp
