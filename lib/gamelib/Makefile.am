AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = \
	audp_parser.h

noinst_LIBRARIES = libgamelib.a
noinst_HEADERS = \
	anim.h \
	animobj.h \
	gtime.h \
	hashtable.h \
	parser.h

libgamelib_a_SOURCES = \
	anim.cpp \
	animobj.cpp \
	audp_lexer.lpp \
	audp_parser.ypp \
	gtime.cpp \
	hashtable.cpp

