noinst_LIBRARIES = libscript.a
AM_CPPFLAGS = -I../framework -I../..
libscript_a_SOURCES = \
	codeprint.c \
	event.c \
	evntsave.c \
	interp.c \
	script.c \
	script_l.c \
	script_y.c \
	stack.c \
	codeprint.h \
	event.h \
	evntsave.h \
	interp.h \
	parse.h \
	script.h \
	script_y.h \
	stack.h

libscript_a_libs = ../framework/libframework.a
libscript_a_DEPENDENCIES = $(libscript_a_libs)
libscript_a_LIBADD = $(libscript_a_libs)
