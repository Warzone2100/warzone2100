# Copyright (c) 1995, 3Dfx Interactive, Inc.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
# the contents of this file may not be disclosed to third parties, copied or
# duplicated in any form, in whole or in part, without the prior written
# permission of 3Dfx Interactive, Inc.
#
# RESTRICTED RIGHTS LEGEND:
# Use, duplication or disclosure by the Government is subject to restrictions
# as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
# and Computer Software clause at DFARS 252.227-7013, and/or in similar or
# successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
# rights reserved under the Copyright Laws of the United States.
#
# $Revision: 34 $
# $Date: 5/18/98 3:23p $

#
# Make include file, kept as simple as possible, defines/setups the following
#       1) global CC flags
#       2) global LD flags
#       3) default rule (all)
#       4) make depend rules
#       5) file removal rules
#       6) recursive rules

#--------------------------------------------------------------------------
# Configuration variables

# BUILD_ROOT_SWLIBS   = top of built swlibs components
# BUILD_ROOT_SST1     = top of built sst1 components

# INSTALL_DESTINATION = where components get installed

# KIT_INSTALL         = directory where kit source gets installed
# KIT_PROJECT         = current kit project

!ifdef FX_INSTALL_KIT

VAR=%%%%

!if "$(FX_COMPILER)" == "WATCOM"
FX_COMPLIB_DIR=\watcom
!else if "$(FX_COMPILER)" == "MICROSOFT"
FX_COMPLIB_DIR=\msvc
!endif

!else

FX_INSTALL_SOURCE = $(KIT_INSTALL)\src\$(KIT_PROJECT)
!ifdef INCLUDE

!endif

VAR=%%
FX_COMPLIB_DIR=

!endif

#--------------------------------------------------------------------------
# configuration variables for DLL builds
# FX_DLL_BUILD   user (or makefile) sets to indicate DLLs are desired
# FX_DLL_COMPILE makefile sets to indicate files compiled for linking
#                into a DLL
# FX_DLL_LIBRARY makefile sets to indicate linking for a DLL
#                implies FX_DLL_COMPILE
#
# COMPILE_DLL sets FX_DLL_ENABLE define
# library source sets FX_DLL_DEFINITION before including fxdll.h
# to indicate next header is interface file
# LINK_DLL    changes library construction

!if "$(FX_TARGET)" == "WIN32" && defined(FX_DLL_BUILD) && defined(FX_DLL_LIBRARY)
FX_DLL_COMPILE  = 1
LINK_DLL        = 1
!endif

!if "$(FX_TARGET)" == "DOS" && defined(FX_DLL_BUILD) && defined(FX_DLL_LIBRARY)
FX_DLL_COMPILE  = 1
LINK_DLL        = 1
!endif

!if "$(FX_TARGET)" == "WIN32" && defined(FX_DLL_BUILD) && defined(FX_DLL_COMPILE)
COMPILE_DLL     = 1
!endif

!if "$(FX_TARGET)" == "DOS" && defined(FX_DLL_BUILD) && defined(FX_DLL_COMPILE)
COMPILE_DLL     = 1
!endif


#--------------------------------------------------------------------------
# configuration variables for the compiler environment
#       FX_COMPILER must be set to either MICROSOFT, WATCOM, DJGPP, BORLAND, etc
#       FX_HOST must be to WIN32 or DOS, etc
#       FX_TARGET must be set to WIN32 or DOS, etc
#       DEBUG must be set to enable debugging flags for cc and link
#
!if "$(FX_COMPILER)" == "MICROSOFT"
!ifdef FX_BOUNDSCHECK
CC             = bcompile
CPLUSPLUS      = bcompile
!else
CC             = cl
CPLUSPLUS      = cl
!endif
CPP            = $(CC) -E /c
AOPTS          = /coff
ASM_LIST_FLAGS = /FAsc
CDEBUG         = -Z7
CNODEBUG       = -Ox
LDEBUG         = -Z7
LNODEBUG       = $(CDEBUG)
GCINCS         = -I$(BUILD_ROOT_SWLIBS)\include 
GCOPTS         = -nologo -W3 -G5 /c
GCDEFS         = -D__MSC__
!if "$(FX_TARGET_MINOR)" == "WIN95"
GLDOPTS        = -nologo /link /map -subsystem:windows,4.0 -debugtype:both -pdb:none
!else
GLDOPTS        = -nologo /link -subsystem:console -debugtype:both -pdb:none
!endif
LINK           = $(CC)
AR             = lib -nologo

!else if "$(FX_COMPILER)" == "WATCOM"
WC2DBG            = wc2dbg      
CC                = wcc386
CPLUSPLUS         = wpp386
CPP               =
!if "$(FX_PCI_TARGET)" == "PHARLAP"
CDEBUG            = -d2 -hc
!else
CDEBUG            = -d2
!endif
CNODEBUG          = -s -otexan
LDEBUG            = d all
LNODEBUG          =
GCINCS            = -I$(BUILD_ROOT_SWLIBS)\include;
!ifdef FX_WATCOM_REG_CALL
GCOPTS            = -w4 -e25 -7 -fp5 -zq -5r -ei
AOPTS             = /D__SW_3R
!else
GCOPTS            = -w4 -e25 -7 -fp5 -zq -5s -ei
AOPTS             = /D__SW_3S
!endif
GLDOPTS           = OP m OP ST=500k OP MAXE=25 OP Q 
LINK              = wlink
AR                = wlib -b -c -n -q -p=512
CC_ENV_VAR        = WCC386
CPLUSPLUS_ENV_VAR = WPP386
LD_ENV_VAR        = WLINK
!else if "$(FX_COMPILER)" == "DJGPP"

# xxx - gcc doesn't take semicolon terminated lists
# e.g. -I$(INCLUDE) doesn't work with typical DOS environment

CC                = gcc
CPLUSPLUS         = gcc
CPP               = 
CDEBUG            = -c -g -D_cdecl= -D__cdecl= 
CNODEBUG          = -c -O2 -ffast-math -m486 -mno-ieee-fp -D_cdecl= -D__cdecl=
LDEBUG            =
LNODEBUG          =
GCINCS            = -I$(BUILD_ROOT_SWLIBS)\include -I. -I$(INCLUDE)
GCOPTS            = 
GLDOPTS           = -lc
LINK              = $(CC)
AR                = ar
ARS               = ars.bat
RANLIB            = ranlib
AOPTS             = /coff

!else if "$(FX_COMPILER)" == "BORLAND"

!endif

#--------------------------------------------------------------------------
# configuration variables for the assembler
#       DEBUG must be set to enable debugging flags for masm
#   NOTE:  Only masm is supported, as wasm isn't intended by watcom to be 
#   used for development, as far as I can tell (chd)
#
AS                = ml.exe
ADEBUG            = /Zi
AOPTS             = $(AOPTS) /I. /c /Cp 
ANODEBUG          = 

# if we are not debugging then replace debug flags with nodebug flags
!ifndef DEBUG
CDEBUG   = $(CNODEBUG)
LDEBUG   = $(LNODEBUG)
ADEBUG   = $(ANODEBUG)
!endif

#--------------------------------------------------------------------------
# configuration variables for the target environment
#       FX_TARGET must be set to either WIN32 or DOS
!if "$(FX_TARGET)" == "WIN32"
GCDEFS     = $(GCDEFS) -DWIN32 -D__WIN32__
# stuff from NTWIN32.MAK, all the system libraries we need

!ifdef COMPILE_DLL
GCDEFS     = $(GCDEFS) -DFX_DLL_ENABLE
!endif

!if "$(FX_COMPILER)" == "MICROSOFT"

baselibs   = kernel32.lib $(optlibs) advapi32.lib
uilibs     = comdlg32.lib comctl32.lib ctl3d32.lib shell32.lib
winlibs    = $(baselibs) $(uilibs) user32.lib gdi32.lib winspool.lib winmm.lib
libcdll    = /MD
guilibsdll = $(libcdll) $(winlibs)

!else if "$(FX_COMPILER)" == "WATCOM"

#GCINCS  = $(GCINCS) -I$(WATCOM)\h;$(WATCOM)\h\nt;
!ifdef COMPILE_DLL
GCOPTS  = $(GCOPTS) -mf -bt^#NT -bd
!else
GCOPTS  = $(GCOPTS) -mf -bt^#NT -bm
!endif
GLDOPTS = $(GLDOPTS) SYS NT

!else if "$(FX_COMPILER)" == "DJGPP"

!else if "$(FX_COMPILER)" == "BORLAND"

!endif

!else

GCDEFS  = $(GCDEFS) -D__DOS32__

!if "$(FX_COMPILER)" == "WATCOM"

#GCINCS  = $(GCINCS) -I$(WATCOM)\h
!ifdef COMPILE_DLL
GCDEFS     = $(GCDEFS) -DFX_DLL_ENABLE
!endif
GCOPTS  = $(GCOPTS) -mf -bt^#dos
!ifdef LINK_DLL
STUB = OP stub=$(BUILD_ROOT_SWLIBS)\lib\dllstub 
LIBPATHOPT = LIBP $(BUILD_ROOT_SWLIBS)\lib
DDLLSTKSZOPT=   OP stack=1024
GLDOPTS = $(GLDOPTS) SYS dos4g dll $(DDLLSTKSZOPT) $(STUB) $(LIBPATHOPT)
!else
GLDOPTS = $(GLDOPTS) SYS dos4g
!endif
#Below is a prototype line for linking Phar Lap with Watcom C
#GLDOPTS = $(GLDOPTS) SYS TNT

!endif

!endif

#--------------------------------------------------------------------------
# build up CFLAGS from global, local, and variable flags
#       each of which has includes, defines, and options
#
GCFLAGS = $(GCINCS) $(GCDEFS) $(GCOPTS)
LCFLAGS = $(LCINCS) $(LCDEFS) $(LCOPTS)
VCFLAGS = $(VCINCS) $(VCDEFS) $(VCOPTS)

CFLAGS  = $(CDEBUG) $(GCFLAGS) $(LCFLAGS) $(VCFLAGS)

#--------------------------------------------------------------------------
# build up global linker flags (LDFLAGS) and libraries (LDLIBS)
#       note that local libs are before global libs
#
GLDOPTS = $(LDEBUG) $(GLDOPTS)
GLDLIBS = $(guilibsdll)
LDFLAGS = $(GLDOPTS) $(LLDOPTS)
LDLIBS  = $(LLDLIBS) $(GLDLIBS)

#--------------------------------------------------------------------------
# build up global assembler flags
#
AFLAGS = $(LADEFS) $(ADEBUG) $(LAINCS) $(AOPTS) $(LAOPTS) $(AINCLUDES)

#--------------------------------------------------------------------------
# configure OS commands
#
!if "$(FX_HOST)" == "DOS"
XCOPY=xcopy
CALL=call
!else
XCOPY=@xcopy
CALL=@call
!endif

CHMODWR = attrib -r
!ifndef MKSUSER
MKSUSER = 0
!endif
!if "$(USERNAME)" == "sellers" || "$(USERNAME)" == "tarolli" || "$(USERNAME)" == "psmith"
MKSUSER = 1
!endif

!if $(MKSUSER)
RM      = rm
!else
RM      = del
!endif
INSTALL = $(XCOPY) /d /k /r /i /s /q

#--------------------------------------------------------------------------
#
# a default rule, serves 2 purposes
#       1) keeps us from typing "make install" all the time
#       2) allows this file to be included first, without clobber becoming
#               the default rule
default:: all

all: incs libs bins

!if "$(FX_COMPILER)" == "DJGPP"
OBJECTS = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(AFILES:.asm=.o)
!else
RESFILE = $(RCFILE:.rc=.res)
!if "$(FX_COMPILER)" == "MICROSOFT"
OBJECTS = $(CFILES:.c=.obj) $(CPPFILES:.cpp=.obj) $(AFILES:.asm=.obj) $(RESFILE) $(LOBJECTS)

!else if "$(FX_COMPILER)" == "WATCOM"
OBJECTS = $(CFILES:.c=.obj) $(CPPFILES:.cpp=.obj) $(AFILES:.asm=.obj)
!endif

!endif

#--------------------------------------------------------------------------
# rules for INCS, LIBS, and BINS , the three major targets
#

$(THISDIR)incs: $(BUILD_TOOLS) $(HEADERS)
!       ifdef HEADERS
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\include
    !attrib +r $(INSTALL_DESTINATION)\include\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\include
    !attrib +r $(FX_INSTALL_DESTINATION)\include\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\include
    !attrib +r $(FX_INSTALL_DESTINATION)\include\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\include
    !attrib +r $(INSTALL_DESTINATION)\include\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing HEADERS
!endif
!endif

#--------------------------------------------------------------------------
# rules for LIBRARIES
#       NOTE: we supply a default rule for making a library
!ifdef LIBRARIES
LIBPARTS = $(OBJECTS) $(SUBLIBRARIES)

!ifdef LINK_DLL

# this needs to be cleaned up a lot
!if "$(FX_TARGET)" != "DOS"
DLLS = $(LIBRARIES:.lib=.dll)
EXPS = $(LIBRARIES:.lib=.exp)
!else
DLLS = $(LIBRARIES:.lib=.ovl)
EXPS = $(LIBRARIES:.lib=.exp)
!endif

!if "$(FX_COMPILER)" == "MICROSOFT"
$(LIBRARIES) : $(LIBPARTS)
    $(LINK) -o $* -LD $(LIBPARTS) $(LDLIBS) $(LDFLAGS)

$(DLLS) : $(LIBRARIES)

!else if "$(FX_COMPILER)" == "WATCOM"
$(DLLS) : $(LIBPARTS)
    echo name $@ > tmp.lnk
!if "$(FX_TARGET)" == "WIN32" 
    echo system nt_dll initinstance terminstance >> tmp.lnk
!else
    echo SYS dos4g dll >> tmp.lnk
    echo $(DDLLSTKSZOPT) $(STUB) $(LIBPATHOPT)  >> tmp.lnk
    echo LIBR clib3s >> tmp.lnk
!endif
    for $(VAR)i IN ($(OBJECTS)) DO echo file $(VAR)i >> tmp.lnk
!if "$(SUBLIBRARIES)" != ""
    for $(VAR)i IN ($(SUBLIBRARIES)) DO echo libr $(VAR)i >> tmp.lnk
!endif
    $(LINK) $(LDEBUG) OP m OP MAXE=25 OP Q @tmp.lnk
!if "$(FX_TARGET)" == "DOS" && defined(DEBUG)
    $(WC2DBG) $@
!endif
    $(RM) tmp.lnk

!if "$(FX_TARGET)" != "DOS"
$(LIBRARIES) : $(DLLS)
    $(AR) $* +$*.dll
!endif

!endif

!if defined(LIBRARIES) || defined(DLLS)
$(THISDIR)libs: $(LIBRARIES) $(DLLS)
    echo FX_INSTALL_DESTINATION=$(FX_INSTALL_DESTINATION)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $(LIBRARIES) $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(INSTALL_DESTINATION)\bin\$(DLLS)
    !$(INSTALL) $(LIBRARIES) $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$(DLLS)
!else
    !$(INSTALL) $(LIBRARIES) $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$(DLLS)
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $(LIBRARIES) $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(INSTALL_DESTINATION)\bin\$(DLLS)
!else
   @echo INSTALL_DESTINATION not defined, not installing LIBRARIES
!endif
!else
$(THISDIR)libs:
!endif

!else

!if "$(FX_COMPILER)" == "MICROSOFT"
$(LIBRARIES) : $(LIBPARTS)
   $(AR) /out:$*.lib $(LIBPARTS)

!else if "$(FX_COMPILER)" == "WATCOM"
$(LIBRARIES) : $(LIBPARTS)
    type NUL > tmp.lnk
    !echo +'$**' >> tmp.lnk
    $(AR) $* @tmp.lnk
    $(RM) tmp.lnk

!else if "$(FX_COMPILER)" == "DJGPP"
# xxx This is a truly gross gross hack.
LIBRARIES = $(LIBRARIES:.lib=.a)
!if "$(SUBLIBRARIES)" != ""
SUBLIBRARIES = $(SUBLIBRARIES:.lib=.a)
!endif
$(LIBRARIES) : $(LIBPARTS)
    -$(RM) $(LIBRARIES)
    ECHO $(OBJECTS) > TMP.LNK
    $(AR) qc $(LIBRARIES) @TMP.LNK
    $(RM) TMP.LNK
!if "$(SUBLIBRARIES)" != ""
    for $(VAR)i in ($(SUBLIBRARIES)) do call $(ARS) $(LIBRARIES) $(VAR)i
!endif
    $(RANLIB) $(LIBRARIES)

!else if "$(FX_COMPILER)" == "BORLAND"
!endif

$(THISDIR)libs: $(LIBRARIES)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing LIBRARIES
!endif
!endif
!else
$(THISDIR)libs:
!endif

#--------------------------------------------------------------------------
# rules for BINS
#       NOTE: calling makefile must define rules for making programs
#       or define SIMPLE_EXE or MULTI_EXE
!ifdef PROGRAM

!if "$(FX_COMPILER)" == "MICROSOFT"
$(PROGRAM) : $(LLDLIBS) $(OBJECTS) $(DEPDATA)
    $(LINK) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)

!else if "$(FX_COMPILER)" == "WATCOM"
!if "$(FX_PCI_TARGET)" == "PHARLAP"
$(PROGRAM) : $(LLDLIBS) $(OBJECTS) $(DEPDATA)
    echo NUL > tmp.lnk
    for $(VAR)i IN ($(OBJECTS)) DO echo $(VAR)i >> tmp.lnk
    for $(VAR)i IN ($(LDLIBS)) DO echo $(VAR)i >> tmp.lnk
    386link @wc386.lnk @tmp.lnk -exe $@ -fullsym
    $(RM) tmp.lnk
!else
$(PROGRAM) : $(LLDLIBS) $(OBJECTS) $(RCFILE) $(DEPDATA)
!ifdef DEBUG
    echo name $@ > tmp.lnk
    for $(VAR)i IN ($(OBJECTS)) DO echo file $(VAR)i >> tmp.lnk
    for $(VAR)i IN ($(LDLIBS)) DO echo libr $(VAR)i >> tmp.lnk
    $(LINK) $(LDFLAGS) @tmp.lnk
    $(WC2DBG) $@
    $(RM) tmp.lnk
!else
    echo name $@ > tmp.lnk
    for $(VAR)i IN ($(OBJECTS)) DO echo file $(VAR)i >> tmp.lnk
    for $(VAR)i IN ($(LDLIBS)) DO echo libr $(VAR)i >> tmp.lnk
    $(LINK) $(LDFLAGS) @tmp.lnk
    $(RM) tmp.lnk
!endif
!ifdef RCFILE
    wrc -bt=nt $(RCFILE) $@
!endif
!endif

!else if "$(FX_COMPILER)" == "DJGPP"
# xxx This is a truly gross gross hack.
LLDLIBS = $(LLDLIBS:.lib=.a)
$(PROGRAM) : $(LLDLIBS) $(OBJECTS)
!if "$(DEBUG)" == ""
        ECHO $(OBJECTS) $(LDLIBS) $(LDFLAGS) > TMP.LNK
        $(LINK) -o $@ @TMP.LNK
        $(RM) TMP.LNK
!else
        ECHO $(OBJECTS) $(LDLIBS) $(LDFLAGS) > TMP.LNK
        $(LINK) -o $* @TMP.LNK
        $(RM) TMP.LNK
!endif

!else if "$(FX_COMPILER)" == "BORLAND"
!endif
!endif

!ifdef PROGRAMS

!if "$(FX_COMPILER)" == "MICROSOFT"
$(PROGRAMS) : $(LLDLIBS) $*.obj $(RESFILE)
        $(LINK) -o $@ $*.obj $(RESFILE) $(LDLIBS) $(LDFLAGS)

!else if "$(FX_COMPILER)" == "WATCOM"
!if "$(FX_PCI_TARGET)" == "PHARLAP"
$(PROGRAMS) : $(LLDLIBS) $*.obj
    echo $*.obj > tmp.lnk
    for $(VAR)i IN ($(LDLIBS)) DO echo $(VAR)i >> tmp.lnk
    386link @wc386.lnk @tmp.lnk -exe $@ -fullsym
    $(RM) tmp.lnk
!else
$(PROGRAMS) : $(LLDLIBS) $*.obj $(RCFILE)
    echo name $@ > tmp.lnk
    echo file $*.obj >> tmp.lnk
    for $(VAR)i IN ($(LDLIBS)) DO echo libr $(VAR)i >> tmp.lnk
    $(LINK) $(LDFLAGS) @tmp.lnk
!ifdef DEBUG
    $(WC2DBG) $@
!endif
    $(RM) tmp.lnk
!ifdef RCFILE
    wrc -bt=nt $(RCFILE) $@
!endif
!endif

!else if "$(FX_COMPILER)" == "BORLAND"

!else if "$(FX_COMPILER)" == "DJGPP"
# xxx This is a truly gross gross hack.
LLDLIBS = $(LLDLIBS:.lib=.a)
$(PROGRAMS) : $(LLDLIBS) $*.o
!if "$(DEBUG)" == ""
        ECHO $(LDLIBS) $(LDFLAGS) > TMP.LNK
        $(LINK) -o $@ $*.o @TMP.LNK
        $(RM) TMP.LNK
!else
        ECHO $(LDLIBS) $(LDFLAGS) > TMP.LNK
        $(LINK) -o $* $*.o @TMP.LNK
        $(RM) TMP.LNK
!endif

!else
!endif
!endif


!if defined(PROGRAM) || defined(PROGRAMS) || defined(BATS)  || defined(DIAGS)
$(THISDIR)bins: $(PROGRAM) $(PROGRAMS) $(BATS) $(DATAFILES) $(DIAGS)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\bin\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\bin\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing PROGRAMS
!endif
!else
$(THISDIR)bins:
!endif

TARGETS = $(TARGETS) $(LIBRARIES) $(DLLS) $(EXPS) $(PROGRAM) $(PROGRAMS) $(DEPDATA) $(DIAGS)

#--------------------------------------------------------------------------
# Source kit targets

!ifdef FX_INSTALL_KIT

# keep quiet about sublibraries missing
!if "$(SUBLIBRARIES)" != ""
$(SUBLIBRARIES):
!endif
!ifdef LLDLIBS
$(LLDLIBS):
!endif

!endif

!if "$(FX_TARGET)" == "DOS"
FX_STEM=dos
!else if "$(FX_TARGET)" == "WIN32"
FX_STEM=win
!endif

!if "$(FX_COMPILER)" == "WATCOM"
FX_STEM2=wat
!else if "$(FX_COMPILER)" == "MICROSOFT"
FX_STEM2=msc
!endif

mk$(FX_STEM)$(FX_STEM2).bat:
    type NUL > $@
!if 0
    echo @echo off                          >> $@
!endif
    echo if "%1" == "" goto usage           >> $@
    echo if "%1" == "all" %0 incs libs bins >> $@
    echo :loop                              >> $@
    echo if "%1" == "" goto done            >> $@
    echo if "%1" == "incs" goto doincs      >> $@
    echo if "%1" == "libs" goto dolibs      >> $@
    echo if "%1" == "bins" goto dobins      >> $@
    echo echo Unknown: %1                   >> $@
    echo goto usage                         >> $@
    echo :doincs                            >> $@
    $(MAKE) /$(MAKEFLAGS) /nologo /n FX_INSTALL_KIT=1 incs >> $@
    echo if ERRORLEVEL 1 goto done          >> $@
    echo goto next                          >> $@
    echo :dolibs                            >> $@
    $(MAKE) /$(MAKEFLAGS) /nologo /n FX_INSTALL_KIT=1 libs >> $@
    echo if ERRORLEVEL 1 goto done          >> $@
    echo goto next                          >> $@
    echo :dobins                            >> $@
    $(MAKE) /$(MAKEFLAGS) /nologo /n FX_INSTALL_KIT=1 bins >> $@
    echo if ERRORLEVEL 1 goto done          >> $@
    echo goto next                          >> $@
    echo :next                              >> $@
    echo shift                              >> $@
    echo goto loop                          >> $@
    echo :usage                             >> $@
    echo echo Usage: mk$(FX_STEM)$(FX_STEM2) [ all . incs . libs . bins ] >> $@
    echo :done                              >> $@

build-source-tools: mk$(FX_STEM)$(FX_STEM2).bat

copy-sources: $(HEADERS) $(PRIVATE_HEADERS) $(CFILES) $(CPPFILES) $(AFILES) $(DATAFILES) $(OTHER_CFILES) makefile
    !$(INSTALL) $** $(FX_INSTALL_SOURCE)
    !attrib +r $(FX_INSTALL_SOURCE)\$**

copy-source-tools: mk$(FX_STEM)$(FX_STEM2).bat
    !$(INSTALL) $** $(FX_INSTALL_SOURCE)
    !attrib +r $(FX_INSTALL_SOURCE)\$**

install-sources: copy-sources copy-source-tools 

install-source-headers: $(HEADERS)
    !$(INSTALL) $** $(KIT_INSTALL)\src\include
    !attrib +r $(KIT_INSTALL)\src\include\$**

#--------------------------------------------------------------------------
# MKDEPFILE is the name of the dependency database
#
MKDEPFILE = makedep

#--------------------------------------------------------------------------
# File removal rules: there are four
#       - neat removes dirt
#       - clean removes intermediates and neat (dirt)
#       - clobber removes targets and clean (intermediates and dirt)
#       - rmtargets removes targets only
#

#--------------------------------------------------------------------------
# DIRT definitions
#
GDIRT   = *.cod *.bak *.pdb *.ilk *.map *.sym *.err *.i stderr.out tmp.opt *.evt
DIRT    = $(GDIRT) $(LDIRT)
JUNK    = __junk__

OBJECTS = $(OBJECTS) builddos.bat buildwin.bat 

$(THISDIR)clobber: $(THISDIR)clean $(THISDIR)rmtargets
!if $(MKSUSER)
        $(RM) -f $(MKDEPFILE) $(JUNK) > NUL
!else
        FOR %i IN ($(MKDEPFILE) $(JUNK)) DO $(CHMODWR) %i > NUL
        FOR %i IN ($(MKDEPFILE) $(JUNK)) DO $(RM) %i > NUL
!endif

$(THISDIR)clean: $(THISDIR)neat
!if $(MKSUSER)
        $(RM) -f $(OBJECTS) $(JUNK) > NUL
!else
        FOR %i IN ($(OBJECTS) $(JUNK)) DO $(CHMODWR) %i > NUL
        FOR %i IN ($(OBJECTS) $(JUNK)) DO $(RM) %i > NUL
!endif

$(THISDIR)neat:
!if $(MKSUSER)
        $(RM) -f $(DIRT) $(JUNK) > NUL
!else
        FOR %i IN ($(DIRT) $(JUNK)) DO $(CHMODWR) %i > NUL
        FOR %i IN ($(DIRT) $(JUNK)) DO $(RM) %i > NUL
!endif

$(THISDIR)rmtargets:
!if $(MKSUSER)
        $(RM) -f $(TARGETS) $(JUNK) > NUL
!else
        FOR %i IN ($(TARGETS) $(JUNK)) DO $(CHMODWR) %i > NUL
        FOR %i IN ($(TARGETS) $(JUNK)) DO $(RM) %i > NUL
!endif

.SUFFIXES: .cod .i

!if "$(FX_COMPILER)" == "DJGPP"
.c.o:
        ECHO $(CFLAGS) | perl -pe "s,\\,/,g" > TMP.OPT
        $(CC) @TMP.OPT $<
        $(RM) TMP.OPT
!else
.c.obj:
!if "$(FX_COMPILER)" == "WATCOM"
        SET $(CC_ENV_VAR)=$(CFLAGS)
        $(CC) $<
!ifdef FX_INSTALL_KIT
    if ERRORLEVEL 1 goto done
!endif
!else
        $(CC) $(CFLAGS) $<
!endif
!endif

!if "$(FX_COMPILER)" == "DJGPP"
.cpp.o:
        ECHO $(CFLAGS) > TMP.OPT
        $(CPLUSPLUS) @TMP.OPT $<
        $(RM) TMP.OPT
!else
.cpp.obj:
!if "$(FX_COMPILER)" == "WATCOM"
        SET $(CPLUSPLUS_ENV_VAR)=$(CFLAGS)
        $(CPLUSPLUS) $<
!ifdef FX_INSTALL_KIT
    if ERRORLEVEL 1 goto done
!endif
!else
        $(CPLUSPLUS) $(CFLAGS) $<
!endif
!endif

!if "$(FX_COMPILER)" == "MICROSOFT"
.rc.res:
    rc $(GCINCS) $(LCINCS) $(VCINCS) $(GCDEFS) $(LCDEFS) $(VCDEFS) $<
!ifdef FX_INSTALL_KIT
    if ERRORLEVEL 1 goto done
!endif
!endif

.c.cod:
        $(CC) $(CFLAGS) $(ASM_LIST_FLAGS) $<

.c.i:
        $(CPP) $(CFLAGS) $< > $@

.asm.obj:
        $(AS) $(AFLAGS) $<

.asm.o:
    $(AS) $(AFLAGS) $<
    rename $*.obj $@


#--------------------------------------------------------------------------
# Include a makedepend file if necessary.  Don't know if this
# works.
#
!if EXISTS ($(MKDEPFILE))
!include $(MKDEPFILE)
!endif

!ifndef MAKEFILE
MAKEFILE = makefile
!endif

!ifdef CFILES
$(OBJECTS): $(HEADERS) $(PRIVATE_HEADERS) $(MAKEFILE)
!endif

!ifdef CPPFILES
$(OBJECTS): $(HEADERS) $(PRIVATE_HEADERS) $(MAKEFILE)
!endif

#--------------------------------------------------------------------------
# RECURSIVE rules
#       A recursive makefile should set SUBDIRS and THISDIR
#       setting THISDIR prefixes all the common targets with $(THISDIR)
#       and enables the recursive definitions of them below
#       SUBDIRS1 is used when SUBDIRS exceeds 6 (DOS args only go up to %9)
!ifdef THISDIR

# Recursive targets and rules (decend into each subdirectory)
RETARGETS= clobber clean neat rmtargets depend incs libs bins
RETARGETS2= install uninstall install-sources install-source-headers
MAKE     = $(MAKE) 

$(RETARGETS): $(THISDIR)$$@
        @for %d IN ($(SUBDIRS)) DO $(CALL) %BUILD_ROOT_SWLIBS%\include\nmake\updown %d $(MAKE) $@ "debug=$(DEBUG)"
$(RETARGETS2): $(THISDIR)$$@
        @for %d IN ($(SUBDIRS)) DO $(CALL) %BUILD_ROOT_SWLIBS%\include\nmake\updown %d $(MAKE) $@ "debug=$(DEBUG)"

#$(RETARGETS): $(THISDIR)$$@
#       $(BUILD_ROOT_SWLIBS)\include\nmake\remake $(MAKE) $@ $(SUBDIRS)
#!ifdef SUBDIRS1
#       $(BUILD_ROOT_SWLIBS)\include\nmake\remake $(MAKE) $@ $(SUBDIRS1)
#!endif

!endif

.SUFFIXES: .bof .cmd

.cmd.bof:
        echo $(TOBOF) -f $@ -o $<
        $(TOBOF) -o $@ -f $<

TAGS: $(HEADERS) $(PRIVATE_HEADERS) $(CFILES) $(CPPFILES) $(OTHER_CFILES) $(TAG_REGEXP)
!if "$(TAG_REGEXP)" == ""
        etags $(HEADERS) $(PRIVATE_HEADERS) $(CFILES) $(CPPFILES) $(OTHER_CFILES)
!else
        etags -r @$(TAG_REGEXP) $(HEADERS) $(PRIVATE_HEADERS) $(CFILES) $(CPPFILES) $(OTHER_CFILES)
!endif