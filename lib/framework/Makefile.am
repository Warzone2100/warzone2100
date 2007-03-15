%.tab.h %.tab.c: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
AM_CPPFLAGS = -I$(top_srcdir)
BUILT_SOURCES = resource_parser.tab.h strres_parser.tab.h

noinst_LIBRARIES = libframework.a
noinst_HEADERS = block.h configfile.h cursors.h cursors16.h debug.h font.h fractions.h frame.h \
	frameint.h frameresource.h heap.h input.h listmacs.h mem.h memint.h resly.h \
	strres.h strresly.h treap.h treapint.h trig.h types.h

libframework_a_SOURCES = SDL_framerate.c block.c configfile.c debug.c exceptionhandler.c frame.c \
	frameresource.c heap.c input.c mem.c resource_lexer.lex.c resource_parser.tab.c \
	strres.c strres_lexer.lex.c strres_parser.tab.c treap.c trig.c
