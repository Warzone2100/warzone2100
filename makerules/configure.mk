include $(top_builddir)/makerules/config.mk


# Check for unset config

$(info Checking config...)

ifeq ($(strip $(PACKAGE_VERSION)),)
$(error You must set PACKAGE_VERSION in $(top_srcdir)/makerules/config.mk)
else
$(info PACKAGE_VERSION := $(PACKAGE_VERSION))
endif

ifeq ($(strip $(MODE)),)
$(error You must set MODE in $(top_srcdir)/makerules/config.mk)
else
$(info MODE := $(MODE))
endif

ifeq ($(strip $(DEVDIR)),)
$(error You must set DEVDIR in $(top_srcdir)/makerules/config.mk)
else
$(info DEVDIR := $(DEVDIR))
endif

$(info Config seems valid.)


# Setup paths and static values

PACKAGE:=warzone2100
PACKAGE_NAME:=Warzone 2100
PACKAGE_BUGREPORT:=http://wz2100.net/

WZ_CPPFLAGS:=-DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\" -DYY_STATIC -I$(DEVDIR)/include/SDL -I$(DEVDIR)/include/libpng12 -I$(DEVDIR)/include/bfd -I$(DEVDIR)/include
WZ_CFLAGS:=-std=gnu99
WZ_CXXFLAGS:=
WZ_LDFLAGS:=-L$(DEVDIR)/lib


# Setup build environment with config values

WZ_CFLAGS+=-g -Wall -Werror-implicit-function-declaration
WZ_CXXFLAGS+= -g -Wall

ifeq ($(strip $(MODE)),debug)
WZ_CPPFLAGS+=-DDEBUG
WZ_CFLAGS+=-O0
WZ_CXXFLAGS+=-O0
else
WZ_CPPFLAGS+=-DNDEBUG
endif

ifneq ($(strip $(TRANSLATION)),)
WZ_CPPFLAGS+=-DENABLE_NLS=1
endif

DIRSEP:=\\
MV:=move
RM_F:=del /F
RMDIR:=rmdir
MKDIR_P:=mkdir
XGETTEXT:=xgettext
MSGMERGE:=msgmerge
MSGFMT:=msgfmt
FLEX:=flex
BISON:=bison
MAKENSIS:=makensis

EXEEXT:=.exe
AR:=ar
CC:=gcc
CXX:=g++
WINDRES:=windres
WZ_CPPFLAGS+=-DWIN32
WZ_LDFLAGS+=-mwindows -lmingw32 -lSDLmain -lSDL -lpng12 -lphysfs -lz -lvorbisfile -lvorbis -logg -lpopt -lintl -lGLC -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lshlwapi -lpsapi -lshell32 -lws2_32 -lbfd -liberty -liconv -lz -lfreetype -lfontconfig -lexpat -ltheora


# Import environment variables

CPPFLAGS:=$(WZ_CPPFLAGS) $(CPPFLAGS)
CFLAGS:=$(WZ_CFLAGS) $(CFLAGS)
CXXFLAGS:=$(WZ_CXXFLAGS) $(CXXFLAGS)
LDFLAGS:=$(WZ_LDFLAGS) $(LDFLAGS)


# Export to environment

export PACKAGE PACKAGE_NAME PACKAGE_BUGREPORT
export MV RM_F MKDIR_P RMDIR
export XGETTEXT MSGMERGE MSGFMT
export FLEX BISON MAKENSIS
export EXEEXT AR CC CXX WINDRES
export CPPFLAGS CFLAGS CXXFLAGS LDFLAGS
