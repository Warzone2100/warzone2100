%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(subst -Wmissing-declarations,,$(WZ_CFLAGS))
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = resource_lexer.lex.c strres_lexer.lex.c resource_parser.tab.c strres_parser.tab.c resource_parser.tab.h strres_parser.tab.h
CLEANFILES = resource_lexer.lex.c strres_lexer.lex.c resource_parser.tab.c strres_parser.tab.c resource_parser.tab.h strres_parser.tab.h

noinst_LIBRARIES = libframework.a
noinst_HEADERS = configfile.h cursors.h cursors16.h debug.h frame.h frameint.h \
	frameresource.h input.h listmacs.h math-help.h printf_ext.h resly.h \
	strlfuncs.h strnlen1.h strres.h strresly.h treap.h treapint.h trig.h \
	types.h tagfile.h i18n.h file.h physfs_ext.h

libframework_a_SOURCES = SDL_framerate.c configfile.c debug.c exceptionhandler.c \
	frame.c frameresource.c input.c printf_ext.c resource_lexer.lex.c \
	resource_parser.tab.c strnlen1.c strres.c strres_lexer.lex.c \
	strres_parser.tab.c treap.c trig.c tagfile.c i18n.c
