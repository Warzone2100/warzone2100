include $(top_builddir)/config.mk


# Check for unset config

ifeq ($(MAKELEVEL),0)

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

ifeq ($(strip $(PLATFORM)),)
$(error You must set PLATFORM in $(top_srcdir)/makerules/config.mk)
else
$(info PLATFORM := $(PLATFORM))
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

ifeq ($(strip $(BISON)),)
$(error You must set BISON in $(top_srcdir)/makerules/config.mk)
else
$(info BISON := $(BISON))
endif

ifeq ($(strip $(FLEX)),)
$(error You must set FLEX in $(top_srcdir)/makerules/config.mk)
else
$(info FLEX := $(FLEX))
endif

ifneq ($(strip $(INSTALLER)),)
ifeq ($(strip $(MAKENSIS)),)
$(error You must set MAKENSIS in $(top_srcdir)/makerules/config.mk)
else
$(info MAKENSIS is := $(MAKENSIS))
endif
endif

$(info Config seems valid.)

endif


# Find ourselves

sub_path:=$(patsubst $(top_builddir)/%,%,$(CURDIR))
ifneq ($(strip $(sub_path)),$(top_builddir))
srcdir:=$(top_srcdir)/$(sub_path)
else
srcdir:=$(top_srcdir)
endif

builddir:=$(CURDIR)


# Setup paths and static values

CPPFLAGS+=-DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\" -DYY_STATIC -I$(builddir) -I$(srcdir) -I$(top_srcdir) -I$(DEVDIR)/include/SDL -I$(DEVDIR)/include/libpng12 -I$(DEVDIR)/include/bfd -I$(DEVDIR)/include
CFLAGS+=-std=gnu99
CXXFLAGS+=
LDFLAGS+=-L$(DEVDIR)/lib

# Setup build environment with config values

ifeq ($(strip $(MODE)),debug)
CPPFLAGS+=-DDEBUG -Wall -Werror-implicit-function-declaration
CFLAGS+=-g -O0
CXXFLAGS+=-g -O0
else
CPPFLAGS+=-DNDEBUG
endif

ifneq ($(strip $(USE_GETTEXT)),)
CPPFLAGS+=-DENABLE_NLS=1
ifneq ($(LOCALEDIR),)
CPPFLAGS+=-DLOCALEDIR=$(LOCALEDIR)
endif
endif

ifeq ($(strip $(PLATFORM)),windows)
DIRSEP:=\\
MV:=???
RM_F:=del /F
RMDIR:=???
MKDIR_P:=???
TEST_D:=???
EXEEXT:=.exe
AR:=ar
CC:=gcc
CXX:=g++
WINDRES:=windres
CPPFLAGS+=-DWIN32
CFLAGS+=-mwindows
CXXFLAGS+=-mwindows
LDFLAGS+=-lmingw32 -lSDLmain
else
ifeq ($(strip $(PLATFORM)),mingw32)
DIRSEP:=/
MV:=mv
RM_F:=rm -f
RMDIR:=rmdir
MKDIR_P:=mkdir -p
TEST_D:=test -d
EXEEXT:=.exe
AR:=mingw32-ar
CC:=mingw32-gcc
CXX:=mingw32-g++
WINDRES:=mingw32-windres
CPPFLAGS+=-DWIN32
CFLAGS+=-mwindows
CXXFLAGS+=-mwindows
LDFLAGS+=-lmingw32 -lSDLmain
else
DIRSEP:=/
MV:=mv
RM_F:=rm -f
RMDIR:=rmdir
MKDIR_P:=mkdir -p
TEST_D:=test -d
EXEEXT:=
AR:=ar
CC:=gcc
CXX:=g++
WINDRES:=
endif
endif

# Generic libs

LDFLAGS+=-lSDL -lSDL_net -lpng12 -lphysfs -lz -lvorbisfile -lvorbis -logg -lpopt -lintl

# Additional platform-dependend libs

ifeq ($(strip $(PLATFORM)),windows)
LDFLAGS+=-lGLC -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lwsock32 -lbfd -liberty
else
ifeq ($(strip $(PLATFORM)),mingw32)
LDFLAGS+=-lGLC -lglu32 -lopengl32 -lopenal32 -ldbghelp -lshfolder -lwinmm -lwsock32 -lbfd -liberty
else
LDFLAGS+=-lGLC -lGLU -lGL -lopenal
endif
endif

# Additionaly link against the deps of our deps

LDFLAGS+=-liconv -lz -lfreetype -lfontconfig -lexpat

include $(top_srcdir)/makerules/common.mk
