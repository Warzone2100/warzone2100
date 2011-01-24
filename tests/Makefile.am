AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools -I. -I$(top_srcdir)/lib/framework
AM_CFLAGS = -g

BUILT_SOURCES = \
	maplist.txt

check_PROGRAMS = \
	maptest

maptest_SOURCES = ../tools/map/mapload.c maptest.c
noinst_HEADERS = ../tools/map/mapload.h
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = \
	maptest

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base mods -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@
