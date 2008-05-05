AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
LDADD = $(top_builddir)/lib/framework/libframework.a $(SDL_LIBS) $(PHYSFS_LIBS)

noinst_PROGRAMS = t1000

t1000_SOURCES = t1000.c
t1000_LDADD = $(top_builddir)/src/level_parser.tab.o \
	$(top_builddir)/src/level_lexer.lex.o \
	$(LDADD)

# Execute all unit tests
test: $(noinst_PROGRAMS)
	for test in $^; do "$(builddir)/$${test}" || exit 1; done

.PHONY: test
