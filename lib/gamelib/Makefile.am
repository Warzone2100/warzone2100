%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<
#	sed -i 's/\(fwrite(.*)\)/abs(\1)/' $@

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = \
	audp_lexer.lex.c \
	audp_parser.tab.c \
	audp_parser.tab.h

CLEANFILES = \
	audp_lexer.lex.c \
	audp_parser.tab.c \
	audp_parser.tab.h

EXTRA_DIST = \
	audp_lexer.l audp_parser.y

noinst_LIBRARIES = libgamelib.a
noinst_HEADERS = \
	audp_parser.tab.h \
	anim.h \
	animobj.h \
	gtime.h \
	hashtable.h \
	parser.h

libgamelib_a_SOURCES = \
	anim.c \
	animobj.c \
	audp_lexer.lex.c \
	audp_parser.tab.c \
	gtime.c \
	hashtable.c

libgamelib_a_LIBADD = $(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a $(top_builddir)/lib/framework/libframework.a
