noinst_LTLIBRARIES = libplatformsdl.la

libplatformsdl_la_SOURCES = \
	event.c \
	init.c \
	time.c

libplatformsdl_la_CPPFLAGS = $(SDL_CFLAGS)
libplatformsdl_la_LIBADD = $(SDL_LIBS)

if MINGW32
libplatformsdl_la_SOURCES += clipboardWin32.c
else
if MACOSX
libplatformsdl_la_SOURCES += clipboardOSX.m
else
# Assume X11 if none of the above match
libplatformsdl_la_SOURCES += clipboardX11.c
endif
endif

noinst_HEADERS = \
	event.h \
	init.h
