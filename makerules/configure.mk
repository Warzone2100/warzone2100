include $(MAKERULES)/config.mk


# Check for unset config

ifeq ($(strip $(VERSION)),)
$(error You must set VERSION in $(MAKERULES)/config.mk)
else
$(info VERSION set to $(VERSION))
endif

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

CFLAGS+=-m32 -DVERSION=$(VERSION) -DYY_STATIC -I.. -I../.. -I$(DEVDIR)/include
LDFLAGS+=-L$(DEVDIR)/lib


# Setup build environment with config values

ifeq ($(strip $(COMPILER)),g++)
CC=g++
CFLAGS+=-fpermissive
else
CC=gcc
endif

ifeq ($(strip $(MODE)),debug)
CFLAGS+=-Wall -O0 -g3 -DDEBUG
else
CFLAGS+=-march=i686 -Os -DNDEBUG
LDFLAGS+=-Wl,-S
endif

ifeq ($(strip $(PLATFORM)),windows)
DIRSEP=\\
RMF=del /F
EXEEXT=.exe
WINDRES=windres
CFLAGS+=-mwindows -DWIN32
LDFLAGS+=-lmingw32 -lglu32 -lopengl32 -lopenal32 -ljpeg6b -lpng13
else
DIRSEP=/
RMF=rm -f
EXEEXT=
WINDRES=
LDFLAGS+=-lGLU -lGL -lopenal -ljpeg -lpng
endif

LDFLAGS+=-lmad -lvorbisfile -lvorbis -logg -lphysfs -lSDLmain -lSDL -lSDL_net

include $(MAKERULES)/common.mk
