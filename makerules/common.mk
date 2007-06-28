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
OPENGL_LIB=GLU GL
OPENAL_LIB=openal
endif

CC=gcc
#CC=g++ -fpermissive
CFLAGS+=-gstabs -DYY_STATIC

ifeq ($(MODE), prod)
CFLAGS+=-Wall -fno-strict-aliasing -O2
else
CFLAGS+=-Wall -g
endif

OBJ_FILES=$(SRC_FILES:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $(<)
