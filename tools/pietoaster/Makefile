CXX=g++
CXXFLAGS=-g
CPPFLAGS=-I/usr/include/SDL
LDFLAGS=
LIBS=-lSDL_image -lSDL -lGLU -lGL -lAntTweakBar

SOURCES= \
	game_io.cpp \
	gui.cpp \
	imd.cpp \
	imdloader.cpp \
	pie_internal.cpp \
	pietoaster.cpp \
	resmaster.cpp \
	screen.cpp \
	texture.cpp \
	texture_mapper.cpp

OBJECTFILES=$(SOURCES:%.cpp=%.o)

all: pietoaster

clean:
	-rm -f $(OBJECTFILES) pietoaster

pietoaster: $(OBJECTFILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: all clean
