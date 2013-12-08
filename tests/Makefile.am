AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools \
	-I. -I$(top_srcdir)/lib/framework $(QT4_CFLAGS) $(LIBCRYPTO_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

BUILT_SOURCES = maplist.txt modellist.txt jslist.txt

if !MINGW32
bin_PROGRAMS = qslint
qslint_SOURCES = qslint.cpp lint.cpp
qslint_LDADD = $(PHYSFS_LIBS) $(QT4_LIBS)
endif

check_PROGRAMS = maptest modeltest qtscripttest framework_linktest
qtscripttest_SOURCES = qtscripttest.cpp lint.cpp
qtscripttest_LDADD = $(PHYSFS_LIBS) $(QT4_LIBS)

framework_linktest_SOURCES = framework_linktest.cpp
framework_linktest_LDADD = $(top_builddir)/lib/framework/libframework.a $(PHYSFS_LIBS) $(LIBCRYPTO_LIBS) $(LDFLAGS)

modeltest_SOURCES = modeltest.c

maptest_SOURCES = ../tools/map/mapload.cpp maptest.cpp
maptest_LDADD = $(PHYSFS_LIBS) $(PNG_LIBS)

noinst_HEADERS = ../tools/map/mapload.h lint.h

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	configs \
	Tests.xcodeproj

# qtscripttest commented out for 3.1
TESTS = maptest modeltest 

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@

modellist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -iname \*.pie > $(abs_top_builddir)/tests/modellist.txt )
	touch $@

jslist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -name \*.js > $(abs_top_builddir)/tests/jslist.txt )
	touch $@
