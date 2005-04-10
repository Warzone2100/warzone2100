.SUFFIXES: .o

ifeq ($(EMUL), yes)
CFLAGS=-m32
LDFLAGS=-m32
SDLCONFIG=/emul/linux/x86/usr/bin/sdl-config
else
SDLCONFIG=sdl-config
endif

CPP=gcc
CFLAGS+=-Wall -g
CFLAGS+=-gstabs -DYY_STATIC

