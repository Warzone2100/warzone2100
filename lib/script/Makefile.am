AM_CPPFLAGS = -DYY_NO_INPUT $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

BUILT_SOURCES = \
	chat_parser.h \
	script_parser.h

noinst_LIBRARIES = libscript.a
noinst_HEADERS = \
	chat_processing.h \
	codeprint.h \
	event.h \
	eventsave.h \
	interpreter.h \
	parse.h \
	script.h \
	stack.h

libscript_a_SOURCES = \
	chat_lexer.cpp \
	chat_parser.cpp \
	codeprint.cpp \
	event.cpp \
	eventsave.cpp \
	interpreter.cpp \
	script.cpp \
	script_lexer.cpp \
	script_parser.cpp \
	stack.cpp

