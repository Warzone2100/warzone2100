AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I../tools -I. -I../lib/framework

BUILT_SOURCES = \
	maplist.txt

check_PROGRAMS = \
	maptest \
	astartest

maptest_SOURCES = ../tools/map/mapload.c maptest.c
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

astartest_SOURCES = astartest.c
astartest_LDADD = $(SDL_LIBS)

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = \
	maptest \
	astartest

maplist.txt:
	(cd ../data ; find base mods -name game.map > ../tests/maplist.txt )
	touch $@
