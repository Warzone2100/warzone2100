WINEBUILD = winebuild
CC = winegcc
CFLAGS = -g -Wall
DLLS = winstr
rpl2avi_OBJECTS = rpl2avi.o

rpl2avi: LIBS = -lavifil32 -L. $(DLLS:%=-l%)

all: rpl2avi

lib%.def: %.spec
	$(WINEBUILD) --def -E $< -o $@

rpl2avi: $(rpl2avi_OBJECTS) $(DLLS:%=lib%.def)
	$(CC) -o $@ $(rpl2avi_OBJECTS) $(LIBS)
