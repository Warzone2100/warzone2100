AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

MOCHEADER = \
	wzconfig.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC4) -o $@ $<

BUILT_SOURCES = \
	resource_parser.h \
	strres_parser.h

CLEANFILES = \
	$(MOCEDFILES)

EXTRA_DIST = \
	cocoa_wrapper.h cocoa_wrapper.mm

noinst_LIBRARIES = libframework.a
noinst_HEADERS = \
	$(MOCHEADER) \
	config-macosx.h \
	crc.h \
	cursors.h \
	debug.h \
	endian_hack.h \
	file.h \
	fixedpoint.h \
	frame.h \
	frameint.h \
	frameresource.h \
	geometry.h \
	gettext.h \
	i18n.h \
	input.h \
	lexer_input.h \
	listmacs.h \
	macros.h \
	math_ext.h \
	opengl.h \
	physfs_ext.h \
	rational.h \
	resly.h \
	stdio_ext.h \
	string_ext.h \
	strres.h \
	strresly.h \
	treap.h \
	trig.h \
	types.h \
	utf.h \
	vector.h \
	wzapp.h \
	wzfs.h \
	wzglobal.h

nodist_libframework_a_SOURCES = \
	wzconfig_moc.cpp

libframework_a_SOURCES = \
	crc.cpp \
	debug.cpp \
	frame.cpp \
	frameresource.cpp \
	geometry.cpp \
	i18n.cpp \
	lexer_input.cpp \
	resource_lexer.lpp \
	resource_parser.ypp \
	stdio_ext.cpp \
	strres.cpp \
	strres_lexer.lpp \
	strres_parser.ypp \
	treap.cpp \
	trig.cpp \
	utf.cpp \
	wzconfig.cpp
