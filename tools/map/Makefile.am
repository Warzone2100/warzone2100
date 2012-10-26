AM_CPPFLAGS = -DYY_NO_INPUT $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS) -I../..

BUILT_SOURCES = 
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = \
	configs \
	MapUtilities.xcodeproj \
noinst_PROGRAMS = mapconv mapinfo map2lnd map2preview map2png
noinst_HEADERS = maplib.h mapload.h pngsave.h vector.h

LIBLIST = $(LTLIBINTL) $(PHYSFS_LIBS) $(PNG_LIBS) $(QT4_LIBS)

mapconv_SOURCES = mapconv.cpp mapload.cpp maplib.cpp pngsave.cpp
mapconv_LDADD = $(LIBLIST)

mapinfo_SOURCES = mapinfo.cpp mapload.cpp maplib.cpp
mapinfo_LDADD = $(LIBLIST)

map2lnd_SOURCES = map2lnd.cpp mapload.cpp maplib.cpp
map2lnd_LDADD = $(LIBLIST)

map2png_SOURCES = map2png.cpp mapload.cpp maplib.cpp pngsave.cpp
map2png_LDADD = $(LIBLIST)

map2preview_SOURCES = map2preview.cpp mapload.cpp maplib.cpp pngsave.cpp
map2preview_LDADD = $(LIBLIST)
