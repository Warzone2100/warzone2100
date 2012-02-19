AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools -I. -I$(top_srcdir)/lib/framework $(QT4_CFLAGS)
AM_CFLAGS = -g

BUILT_SOURCES = maplist.txt modellist.txt jslist.txt

#bin_PROGRAMS = qslint
check_PROGRAMS = maptest modeltest qtscripttest

#qslint_SOURCES = qslint.cpp lint.cpp
#qslint_LDADD = $(PHYSFS_LIBS) $(QT4_LIBS)

qtscripttest_SOURCES = qtscripttest.cpp lint.cpp
qtscripttest_LDADD = $(PHYSFS_LIBS) $(QT4_LIBS)

modeltest_SOURCES = modeltest.c

maptest_SOURCES = ../tools/map/mapload.cpp maptest.cpp
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

noinst_HEADERS = ../tools/map/mapload.h lint.h

CLEANFILES = \
	$(BUILT_SOURCES)

TESTS = maptest modeltest qtscripttest

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@

modellist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -iname \*.pie > $(abs_top_builddir)/tests/modellist.txt )
	touch $@

jslist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -name \*.js > $(abs_top_builddir)/tests/jslist.txt )
	touch $@
