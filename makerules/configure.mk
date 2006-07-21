include $(MAKERULES)/config.mk


# Check for unset config

ifeq ($(strip $(PLATFORM)),)
$(error You must set PLATFORM in $(MAKERULES)/config.mk)
else
$(info PLATFORM set to $(PLATFORM))
endif

ifeq ($(strip $(COMPILER)),)
$(error You must set COMPILER in $(MAKERULES)/config.mk)
else
$(info COMPILER set to $(COMPILER))
endif

ifeq ($(strip $(MODE)),)
$(error You must set MODE in $(MAKERULES)/config.mk)
else
$(info MODE set to $(MODE))
endif

ifeq ($(strip $(DEVDIR)),)
$(error You must set DEVDIR in $(MAKERULES)/config.mk)
else
$(info DEVDIR set to $(DEVDIR))
endif

ifeq ($(strip $(BISON)),)
$(error You must set BISON in $(MAKERULES)/config.mk)
else
$(info BISON is set to $(BISON))
endif

ifeq ($(strip $(FLEX)),)
$(error You must set FLEX in $(MAKERULES)/config.mk)
else
$(info FLEX is set to $(FLEX))
endif


# Setup paths and static values

CFLAGS+=-m32 -DYY_STATIC -I.. -I../.. -I$(DEVDIR)/include
LDFLAGS+=-L$(DEVDIR)/lib


# Setup build environment with config values

ifeq ($(strip $(PLATFORM)),windows)
DIRSEP=\\
RMF=del /F
CFLAGS+=-DWIN32
LDFLAGS+=-lmingw32 -lglu32 -lopengl32 -lopenal32
else
DIRSEP=/
RMF=rm -f
CFLAGS+=-lGLU -lGL -lopenal
endif

ifeq ($(strip $(COMPILER)),g++)
CC=g++
CFLAGS+=-fpermissive
else
CC=gcc
endif

ifeq ($(strip $(MODE)),debug)
CFLAGS+=-Wall -O0 -g3 -DDEBUG
else
CFLAGS+=-march=i686 -O2
endif

LDFLAGS+=-lmad -lvorbisfile -lvorbis -logg -ljpeg6b -lpng13 -lphysfs -lzlib1 -lSDLmain -lSDL -lSDL_net

include $(MAKERULES)/common.mk
