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
	chat_lexer.lex.c \
	chat_parser.tab.c \
	chat_parser.tab.h \
	script_lexer.lex.c \
	script_parser.tab.c \
	script_parser.tab.h

CLEANFILES = \
	chat_lexer.lex.c \
	chat_parser.tab.c \
	chat_parser.tab.h \
	script_lexer.lex.c \
	script_parser.tab.c \
	script_parser.tab.h

EXTRA_DIST = \
	chat_lexer.l chat_parser.y \
	script_lexer.l script_parser.y

noinst_LIBRARIES = libscript.a
noinst_HEADERS = \
	chat_parser.tab.h \
	script_parser.tab.h \
	chat_processing.h \
	codeprint.h \
	event.h \
	eventsave.h \
	interpreter.h \
	parse.h \
	script.h \
	stack.h

libscript_a_SOURCES = \
	chat_lexer.lex.c \
	chat_parser.tab.c \
	codeprint.c \
	event.c \
	eventsave.c \
	interpreter.c \
	script.c \
	script_lexer.lex.c \
	script_parser.tab.c \
	stack.c

libscript_a_LIBADD = $(top_builddir)/lib/framework/libframework.a

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
