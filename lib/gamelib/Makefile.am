AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS)


noinst_LIBRARIES = libgamelib.a
noinst_HEADERS = \
	audp_parser.h \
	gtime.h \
	parser.h

libgamelib_a_SOURCES = \
	audp_lexer.cpp \
	audp_parser.cpp \
	gtime.cpp
