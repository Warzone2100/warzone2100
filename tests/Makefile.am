AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools -I. -I$(top_srcdir)/lib/framework
AM_CFLAGS = -g

BUILT_SOURCES = \
	maplist.txt

check_PROGRAMS = \
	astartest \
	maptest \
	pointtreetest

astartest_SOURCES = astartest.c ../lib/framework/debug.c
astartest_LDADD = $(SDL_LIBS)

maptest_SOURCES = ../tools/map/mapload.c maptest.c
noinst_HEADERS = ../tools/map/mapload.h
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

pointtreetest_SOURCES = pointtreetest.c ../src/pointtree.cpp
pointtreetest_CPPFLAGS = $(AM_CPPFLAGS) -DDUMP_IMAGE

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = \
	astartest \
	maptest \
	pointtreetest

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base mods -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@
