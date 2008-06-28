include $(top_builddir)/config.mk


# Check for unset config

$(info Checking config...)

ifeq ($(strip $(PACKAGE)),)
$(error You must set PACKAGE in $(top_srcdir)/makerules/config.mk)
else
$(info PACKAGE := $(PACKAGE))
endif

ifeq ($(strip $(PACKAGE_NAME)),)
$(error You must set PACKAGE_NAME in $(top_srcdir)/makerules/config.mk)
else
$(info PACKAGE_NAME := $(PACKAGE_NAME))
endif

ifeq ($(strip $(PACKAGE_VERSION)),)
$(error You must set PACKAGE_VERSION in $(top_srcdir)/makerules/config.mk)
else
$(info PACKAGE_VERSION := $(PACKAGE_VERSION))
endif

ifeq ($(strip $(PACKAGE_BUGREPORT)),)
$(error You must set PACKAGE_BUGREPORT in $(top_srcdir)/makerules/config.mk)
else
$(info PACKAGE_BUGREPORT := $(PACKAGE_BUGREPORT))
endif

ifeq ($(strip $(BUILD)),)
$(error You must set BUILD in $(top_srcdir)/makerules/config.mk)
else
$(info BUILD := $(BUILD))
endif

ifeq ($(strip $(TARGET)),)
$(error You must set TARGET in $(top_srcdir)/makerules/config.mk)
else
$(info TARGET := $(TARGET))
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

WZ_CPPFLAGS:=-DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\" -DYY_STATIC -I$(DEVDIR)/include/SDL -I$(DEVDIR)/include/libpng12 -I$(DEVDIR)/include/bfd -I$(DEVDIR)/include
WZ_CFLAGS:=-std=gnu99
WZ_CXXFLAGS:=
WZ_LDFLAGS:=-L$(DEVDIR)/lib


# Setup build environment with config values

ifeq ($(strip $(MODE)),debug)
WZ_CPPFLAGS+=-DDEBUG
WZ_CFLAGS+=-g -O0 -Wall -Werror-implicit-function-declaration
WZ_CXXFLAGS+=-g -O0 -Wall -Werror-implicit-function-declaration
else
WZ_CPPFLAGS+=-DNDEBUG
endif

ifneq ($(strip $(TRANSLATION)),)
WZ_CPPFLAGS+=-DENABLE_NLS=1
endif

ifeq ($(strip $(BUILD)),windows)
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
else
DIRSEP:=/
MV:=mv
RM_F:=rm -f
RMDIR:=rmdir
MKDIR_P:=mkdir -p
XGETTEXT:=xgettext
MSGMERGE:=msgmerge
MSGFMT:=msgfmt
FLEX:=flex
BISON:=bison
MAKENSIS:=makensis
endif

ifeq ($(strip $(TARGET)),windows)
EXEEXT:=.exe
AR:=mingw32-ar
CC:=mingw32-gcc
CXX:=mingw32-g++
WINDRES:=mingw32-windres
WZ_CPPFLAGS+=-DWIN32
WZ_CFLAGS+=-mwindows
WZ_CXXFLAGS+=-mwindows
WZ_LDFLAGS+=-lmingw32 -lSDLmain
else
EXEEXT:=
AR:=ar
CC:=gcc
CXX:=g++
WINDRES:=
endif


# Generic libs

WZ_LDFLAGS+=-lSDL -lSDL_net -lpng12 -lphysfs -lz -lvorbisfile -lvorbis -logg -lpopt -lintl


# Additional target-platform-dependend libs

ifeq ($(strip $(TARGET)),windows)
WZ_LDFLAGS+=-lGLC -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lwsock32 -lbfd -liberty
else
WZ_LDFLAGS+=-lGLC -lGLU -lGL -lopenal
endif


# Additionaly link against the deps of our deps

WZ_LDFLAGS+=-liconv -lz -lfreetype -lfontconfig -lexpat


# Import environment variables

CPPFLAGS:=$(WZ_CPPFLAGS) $(CPPFLAGS)
CFLAGS:=$(WZ_CFLAGS) $(CFLAGS)
CXXFLAGS:=$(WZ_CXXFLAGS) $(CXXFLAGS)
LDFLAGS:=$(WZ_LDFLAGS) $(LDFLAGS)


# Export to environment

export DIRSEP MV RM_F RMDIR MKDIR_P
export XGETTEXT MSGMERGE MSGFMT
export FLEX BISON MAKENSIS
export EXEEXT AR CC CXX WINDRES
export CPPFLAGS CFLAGS CXXFLAGS LDFLAGS
