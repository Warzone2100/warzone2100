AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libsdl.a
noinst_HEADERS = \
	cursors_sdl.h \
	scrap.h \
	wz2100icon.h

libsdl_a_SOURCES = \
	cursors_sdl.cpp \
	main_sdl.cpp \
	scrap.cpp

EXTRA_DIST= \
	swapinterval.mm
