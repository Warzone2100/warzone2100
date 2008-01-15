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

ifneq ($(strip $(INSTALLER)),)
ifeq ($(strip $(MAKENSIS)),)
$(error You must set MAKENSIS in $(MAKERULES)/config.mk)
else
$(info MAKENSIS is set to $(MAKENSIS))
endif
endif


# Setup paths and static values

CFLAGS+=-DPACKAGE_VERSION=\"$(VERSION)\" -DYY_STATIC -DLOCALEDIR=\"$(LOCALEDIR)\" -DPACKAGE=\"$(PACKAGE)\" -I.. -I../.. -I$(DEVDIR)/include/SDL -I$(DEVDIR)/include/libpng12 -I$(DEVDIR)/include
CXXFLAGS+=-DPACKAGE_VERSION=\"$(VERSION)\" -DYY_STATIC -DLOCALEDIR=\"$(LOCALEDIR)\" -DPACKAGE=\"$(PACKAGE)\" -I.. -I../.. -I$(DEVDIR)/include/SDL -I$(DEVDIR)/include/libpng12 -I$(DEVDIR)/include
LDFLAGS+=-L$(DEVDIR)/lib

# Use C99
CFLAGS+=-std=gnu99

# Setup build environment with config values

ifeq ($(strip $(MODE)),debug)
CFLAGS+=-g -O0 -DDEBUG -Wall -Werror-implicit-function-declaration
CXXFLAGS+=-g -O0 -DDEBUG -Wall
else
CFLAGS+=-DNDEBUG
CXXFLAGS+=-DNDEBUG
endif

ifeq ($(strip $(USE_GETTEXT)),yes)
CFLAGS+=-DENABLE_NLS=1
endif

ifeq ($(strip $(PLATFORM)),windows)
DIRSEP=\\
RMF=del /F
EXEEXT=.exe
AR=ar
CC=gcc
CXX=g++
WINDRES=windres
CFLAGS+=-mwindows -DWIN32
CXXFLAGS+=-mwindows -DWIN32
LDFLAGS+=-lmingw32 -lSDLmain
else
ifeq ($(strip $(PLATFORM)),mingw32)
DIRSEP=/
RMF=rm -f
EXEEXT=.exe
AR=mingw32-ar
CC=mingw32-gcc
CXX=mingw32-g++
WINDRES=mingw32-windres
CFLAGS+=-mwindows -DWIN32
CXXFLAGS+=-mwindows -DWIN32
LDFLAGS+=-lmingw32 -lSDLmain
else
DIRSEP=/
RMF=rm -f
EXEEXT=
AR=ar
CC=gcc
CXX=g++
WINDRES=
endif
endif

# Generic libs

LDFLAGS+=-lSDL -lSDL_net -lpng12 -lphysfs -lz -lvorbisfile -lvorbis -logg -lpopt -lintl

# Additional platform-dependend libs

ifeq ($(strip $(PLATFORM)),windows)
LDFLAGS+=-lGLC -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lwsock32
else
ifeq ($(strip $(PLATFORM)),mingw32)
LDFLAGS+=-lglc32 -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lwsock32
else
LDFLAGS+=-lGLC -lGLU -lGL -lopenal
endif
endif

# Additionaly link against the deps of our deps

LDFLAGS+=-liconv -lz -lfreetype -lfontconfig -lexpat

include $(MAKERULES)/common.mk
