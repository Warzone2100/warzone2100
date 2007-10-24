CXX=g++
CFLAGS=-g
CXXFLAGS=
CPPFLAGS=

SOURCES=game_io.cpp \
	imd.cpp \
	imdloader.cpp \
	pie_internal.cpp \
	pietoaster.cpp \
	png_util.cpp \
	resmaster.cpp \
	screen.cpp \
	texture.cpp \
	texture_mapper.cpp

OBJECTFILES=$(SOURCES:%.cpp=%.o)

all: pietoaster

clean:
	-rm -f $(OBJECTFILES) pietoaster

pietoaster: $(OBJECTFILES)
	$(CXX) -o $@ $^ -lSDL_image -lSDL -lpng -lphysfs -lGLU -lGL -lAntTweakBar

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: all clean
