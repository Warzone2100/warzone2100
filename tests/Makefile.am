AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools -I. -I$(top_srcdir)/lib/framework
AM_CFLAGS = -g

BUILT_SOURCES = maplist.txt modellist.txt

check_PROGRAMS = maptest modeltest

modeltest_SOURCES = modeltest.c

maptest_SOURCES = ../tools/map/mapload.c maptest.c
noinst_HEADERS = ../tools/map/mapload.h
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = maptest modeltest

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base mods -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@

modellist.txt:
	(cd $(abs_top_srcdir)/data ; find base mods mp -iname \*.pie > $(abs_top_builddir)/tests/modellist.txt )
	touch $@
