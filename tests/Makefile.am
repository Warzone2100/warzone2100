AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I../tools -I. -I../lib/framework

BUILT_SOURCES = \
	maplist.txt

check_PROGRAMS = \
	json \
	maptest

json_SOURCES = json.c
json_LDADD = $(top_builddir)/lib/framework/libframework.a $(PHYSFS_LIBS)

maptest_SOURCES = ../tools/map/mapload.c maptest.c
maptest_LDADD = $(SDL_CFLAGS) $(PHYSFS_LIBS) $(PNG_LIBS) $(OPENGL_LIBS)

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = \
	jsontest \
	maptest

maplist.txt:
	(cd ../data ; find base mods -name game.map > ../tests/maplist.txt )
	touch $@
