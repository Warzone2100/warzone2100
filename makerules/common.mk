.SUFFIXES: .o

ifeq ($(EMUL), yes)
CFLAGS=-m32
LDFLAGS=-m32
SDLCONFIG=/emul/linux/x86/usr/bin/sdl-config
else
SDLCONFIG=sdl-config
endif

ifeq ($(OSTYPE), msys)
OPENGL_LIB=opengl32
OPENAL_LIB=openal32
else
OPENGL_LIB=GL
OPENAL_LIB=openal
endif

CPP=gcc
CFLAGS+=-Wall -g
CFLAGS+=-gstabs -DYY_STATIC

