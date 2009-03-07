AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I../tools -I. -I../lib/framework

BUILT_SOURCES = maplist.txt
check_PROGRAMS = maptest
maptest_SOURCES = ../tools/map/mapload.c maptest.c
maptest_LDADD = $(SDL_CFLAGS) $(PHYSFS_LIBS) $(PNG_LIBS) $(OPENGL_LIBS)
CLEANFILES = maplist.txt

TESTS = maptest

maplist.txt:
	(cd ../data ; find base -name game.map > ../tests/maplist.txt )
	touch $@
