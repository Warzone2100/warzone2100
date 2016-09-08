AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

# Here, we need to set "plugins" for Qt5 to be static
# FIXME: At a future date, we should check if they want static builds, but, since we only support those for now...
if MINGW32
AM_CFLAGS += -DQT_STATICPLUGIN
AM_CPPFLAGS += -DQT_STATICPLUGIN
AM_CXXFLAGS += -DQT_STATICPLUGIN
endif

noinst_LIBRARIES = libsdl.a
noinst_HEADERS = \
	cursors_sdl.h \
	wz2100icon.h

libsdl_a_SOURCES = \
	cursors_sdl.cpp \
	main_sdl.cpp

EXTRA_DIST= \
	swapinterval.mm
