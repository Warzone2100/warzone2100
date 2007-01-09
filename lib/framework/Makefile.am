%.tab.h %.tab.c: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = resource_parser.tab.h strres_parser.tab.h
noinst_LIBRARIES = libframework.a
AM_CPPFLAGS = -I$(top_srcdir)
libframework_a_SOURCES = \
    SDL_framerate.c \
	resource_lexer.lex.c \
	resource_parser.tab.c \
	strres_lexer.lex.c \
	strres_parser.tab.c \
	block.c \
	configfile.c \
	debug.c \
	frame.c \
	frameresource.c \
	heap.c \
	input.c \
	mem.c \
	strres.c \
	treap.c \
	trig.c \
	block.h \
	font.h \
	heap.h \
	memint.h \
	treap.h \
	fractions.h \
	mono.h \
	strres.h \
	treapint.h \
	frame.h \
	input.h \
	strresly.h \
	trig.h \
	debug.h \
	frameint.h \
	listmacs.h \
	resly.h \
	types.h \
	frameresource.h \
	configfile.h \
	mem.h

# ugly hack to circumvent ugly hack
EXTRA_DIST= cursors.h cursors16.h
