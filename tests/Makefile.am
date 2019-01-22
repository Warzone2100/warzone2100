AM_CPPFLAGS = $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) -I$(top_srcdir)/tools \
	-I. -I$(top_srcdir)/lib/framework $(QT5_CFLAGS) $(OPENGLC_CFLAGS) $(GLEW_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

BUILT_SOURCES = maplist.txt modellist.txt jslist.txt

#if !MINGW32
#bin_PROGRAMS = qslint
#qslint_SOURCES = qslint.cpp lint.cpp
#qslint_LDADD = $(PHYSFS_LIBS) $(QT5_LIBS)
#endif

check_PROGRAMS = maptest modeltest framework_linktest ivis_linktest
#qtscripttest

#qtscripttest_SOURCES = qtscripttest.cpp lint.cpp
#qtscripttest_LDADD = $(PHYSFS_LIBS) $(QT5_LIBS)

framework_linktest_SOURCES = framework_linktest.cpp
framework_linktest_LDADD = $(top_builddir)/lib/framework/libframework.a $(PHYSFS_LIBS) $(LDFLAGS)

ivis_linktest_SOURCES = ivis_linktest.cpp
ivis_linktest_LDADD =
ivis_linktest_LDADD += $(top_builddir)/lib/sdl/libsdl.a
ivis_linktest_LDADD += $(top_builddir)/lib/framework/libframework.a \
	$(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(PHYSFS_LIBS) $(LIBCRYPTO_LIBS) $(QT5_LIBS) $(SDL_LIBS) $(OPENGL_LIBS) $(OPENGLC_LIBS) $(GLEW_LIBS) \
	$(X_LIBS) $(X_EXTRA_LIBS) $(LDFLAGS) $(PNG_LIBS) $(FONT_LIBS)

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
TESTS = maptest modeltest framework_linktest

maplist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -name game.map > $(abs_top_builddir)/tests/maplist.txt )
	touch $@

modellist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -iname \*.pie > $(abs_top_builddir)/tests/modellist.txt )
	touch $@

jslist.txt:
	(cd $(abs_top_srcdir)/data ; find base mp -name \*.js > $(abs_top_builddir)/tests/jslist.txt )
	touch $@
