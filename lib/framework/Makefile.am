AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = resource_parser.h strres_parser.h
noinst_LIBRARIES = libframework.a
AM_CPPFLAGS = -I$(top_srcdir)
libframework_a_SOURCES = \
	resource_lexer.l \
	resource_parser.y \
	strres_lexer.l \
	strres_parser.y \
	block.c \
	configfile.c \
	debug.c \
	frame_stub.c \
	frameresource.c \
	heap.c \
	ignorecase.c \
	input_stub.c \
	mem.c \
	strres.c \
	treap.c \
	trig.c \
	block.h \
	font.h \
	heap.h \
	memint.h \
	treap.h \
	fractions.h \
	ignorecase.h \
	mono.h \
	strres.h \
	treapint.h \
	frame.h \
	input.h \
	strresly.h \
	trig.h \
	debug.h \
	frameint.h \
	listmacs.h \
	resly.h \
	types.h \
	frameresource.h \
	configfile.h \
	mem.h

# ugly hack to circumvent ugly hack
EXTRA_DIST= cursors.c
