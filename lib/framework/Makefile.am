AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS) -DUTF8PROC_STATIC
AM_CFLAGS = $(WZ_CFLAGS) -DUTF8PROC_STATIC
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS) -DUTF8PROC_STATIC

EXTRA_DIST = \
	cocoa_wrapper.h cocoa_wrapper.mm

noinst_LIBRARIES = libframework.a
noinst_HEADERS = \
	config-macosx.h \
	crc.h \
	cursors.h \
	debug.h \
	endian_hack.h \
	file.h \
	fixedpoint.h \
	frame.h \
	frameresource.h \
	geometry.h \
	gettext.h \
	i18n.h \
	input.h \
	lexer_input.h \
	macros.h \
	math_ext.h \
	opengl.h \
	physfs_ext.h \
	rational.h \
	resly.h \
	resource_parser.h \
	stdio_ext.h \
	string_ext.h \
	strres.h \
	strres_parser.h \
	strresly.h \
	treap.h \
	trig.h \
	types.h \
	utf.h \
	vector.h \
	wzapp.h \
	wzconfig.h \
	wzglobal.h \
	wzpaths.h \
	wzstring.h

libframework_a_SOURCES = \
	crc.cpp \
	debug.cpp \
	frame.cpp \
	frameresource.cpp \
	geometry.cpp \
	i18n.cpp \
	lexer_input.cpp \
	resource_lexer.cpp \
	resource_parser.cpp \
	stdio_ext.cpp \
	strres.cpp \
	strres_lexer.cpp \
	strres_parser.cpp \
	treap.cpp \
	trig.cpp \
	utf.cpp \
	wzconfig.cpp \
	wzpaths.cpp \
	wzstring.cpp
