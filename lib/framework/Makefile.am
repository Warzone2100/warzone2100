%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(SQLITE_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	resource_parser.tab.h \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	strres_parser.tab.h

CLEANFILES = \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	resource_parser.tab.h \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	strres_parser.tab.h

noinst_LIBRARIES = libframework.a
noinst_HEADERS = \
	configfile.h \
	cursors.h \
	debug.h \
	file.h \
	frame.h \
	frameint.h \
	frameresource.h \
	i18n.h \
	input.h \
	lexer_input.h \
	listmacs.h \
	math_ext.h \
	physfs_ext.h \
	physfs_vfs.h \
	resly.h \
	stdio_ext.h \
	string_ext.h \
	strres.h \
	strresly.h \
	tagfile.h \
	treap.h \
	trig.h \
	types.h \
	utf.h

libframework_a_SOURCES = \
	configfile.c \
	cursors16.c \
	cursors32.c \
	cursors.c \
	debug.c \
	frame.c \
	frameresource.c \
	i18n.c \
	input.c \
	lexer_input.c \
	physfs_vfs.c \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	SDL_framerate.c \
	stdio_ext.c \
	strres.c \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	tagfile.c \
	treap.c \
	trig.c \
	utf.c
