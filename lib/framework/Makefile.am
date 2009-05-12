%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(SQLITE_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = \
	json_lexer.lex.c \
	json_parser.tab.c \
	json_parser.tab.h \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	resource_parser.tab.h \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	strres_parser.tab.h

CLEANFILES = \
	$(BUILT_SOURCES)

noinst_LIBRARIES = libframework.a
noinst_HEADERS = \
	json_parser.tab.h \
	strres_parser.tab.h \
	resource_parser.tab.h \
	configfile.h \
	cursors.h \
	debug.h \
	file.h \
	fixedpoint.h \
	frame.h \
	frameint.h \
	frameresource.h \
	i18n.h \
	input.h \
	json_value.h \
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
	utf.h \
	vector.h \
	config-macosx.h \
	endian_hack.h \
	gettext.h \
	macros.h \
	SDL_framerate.h \
	wzglobal.h

libframework_a_SOURCES = \
	json_lexer.lex.c \
	json_parser.tab.c \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	configfile.c \
	cursors16.c \
	cursors32.c \
	cursors.c \
	debug.c \
	frame.c \
	frameresource.c \
	i18n.c \
	input.c \
	json_value.c \
	lexer_input.c \
	physfs_vfs.c \
	SDL_framerate.c \
	stdio_ext.c \
	strres.c \
	tagfile.c \
	treap.c \
	trig.c \
	utf.c
