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
CFLAGS+=-Wall -Wno-pointer-sign -fno-strict-aliasing -O2
else
CFLAGS+=-Wall -Wno-pointer-sign -g
endif

OBJ1=$(SRC_FILES:%.l=%.o)
OBJ2=$(OBJ1:%.y=%.o)
OBJ_FILES=$(OBJ2:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $(<)

%.c: %.l
	lex -o $@ $(<)
#	cp $@ $@.bak
# for debugging, since make deletes the generated files for some reason...

%.c: %.y
	bison -d -o $@ $(<)
#	cp $@ $@.bak
# for debugging, since make deletes the generated files for some reason...
