# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=MAXplugIn - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to MAXplugIn - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MAXplugIn - Win32 Release" && "$(CFG)" !=\
 "MAXplugIn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "PIEplugIn.mak" CFG="MAXplugIn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MAXplugIn - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MAXplugIn - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "MAXplugIn - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "d:\3dsmax2\plugins\pumpkin.dle"

CLEAN : 
	-@erase "$(INTDIR)\Bmtex.obj"
	-@erase "$(INTDIR)\max2pie.obj"
	-@erase "$(INTDIR)\max2pie.res"
	-@erase "$(OUTDIR)\pumpkin.exp"
	-@erase "$(OUTDIR)\pumpkin.lib"
	-@erase "d:\3dsmax2\plugins\pumpkin.dle"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "d:\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "d:\maxsdk\include" /D "WIN32" /D "NDEBUG"\
 /D "_WINDOWS" /Fp"$(INTDIR)/PIEplugIn.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/max2pie.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/PIEplugIn.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"d:\3dsmax2\plugins\pumpkin.dle"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/pumpkin.pdb"\
 /machine:I386 /def:".\max2pie.def" /out:"d:\3dsmax2\plugins\pumpkin.dle"\
 /implib:"$(OUTDIR)/pumpkin.lib" 
DEF_FILE= \
	".\max2pie.def"
LINK32_OBJS= \
	"$(INTDIR)\Bmtex.obj" \
	"$(INTDIR)\max2pie.obj" \
	"$(INTDIR)\max2pie.res" \
	"D:\Maxsdk\lib\Bmm.lib" \
	"D:\Maxsdk\lib\Core.lib" \
	"D:\Maxsdk\lib\Geom.lib" \
	"D:\Maxsdk\lib\Mesh.lib" \
	"D:\Maxsdk\lib\Util.lib"

"d:\3dsmax2\plugins\pumpkin.dle" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "d:\3dsmax2\plugins\pumpkin.dle" "$(OUTDIR)\PIEplugIn.bsc"

CLEAN : 
	-@erase "$(INTDIR)\Bmtex.obj"
	-@erase "$(INTDIR)\Bmtex.sbr"
	-@erase "$(INTDIR)\max2pie.obj"
	-@erase "$(INTDIR)\max2pie.res"
	-@erase "$(INTDIR)\max2pie.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\PIEplugIn.bsc"
	-@erase "$(OUTDIR)\pumpkin.exp"
	-@erase "$(OUTDIR)\pumpkin.lib"
	-@erase "$(OUTDIR)\pumpkin.pdb"
	-@erase "d:\3dsmax2\plugins\pumpkin.dle"
	-@erase "d:\3dsmax2\plugins\pumpkin.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "d:\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "d:\maxsdk\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/PIEplugIn.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/max2pie.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/PIEplugIn.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Bmtex.sbr" \
	"$(INTDIR)\max2pie.sbr"

"$(OUTDIR)\PIEplugIn.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"d:\3dsmax2\plugins\pumpkin.dle"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/pumpkin.pdb" /debug\
 /machine:I386 /def:".\max2pie.def" /out:"d:\3dsmax2\plugins\pumpkin.dle"\
 /implib:"$(OUTDIR)/pumpkin.lib" 
DEF_FILE= \
	".\max2pie.def"
LINK32_OBJS= \
	"$(INTDIR)\Bmtex.obj" \
	"$(INTDIR)\max2pie.obj" \
	"$(INTDIR)\max2pie.res" \
	"D:\Maxsdk\lib\Bmm.lib" \
	"D:\Maxsdk\lib\Core.lib" \
	"D:\Maxsdk\lib\Geom.lib" \
	"D:\Maxsdk\lib\Mesh.lib" \
	"D:\Maxsdk\lib\Util.lib"

"d:\3dsmax2\plugins\pumpkin.dle" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "MAXplugIn - Win32 Release"
# Name "MAXplugIn - Win32 Debug"

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=D:\Maxsdk\lib\Util.lib

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=D:\Maxsdk\lib\Core.lib

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=D:\Maxsdk\lib\Geom.lib

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=D:\Maxsdk\lib\Mesh.lib

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=D:\Maxsdk\lib\Bmm.lib

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\max2pie.rc

"$(INTDIR)\max2pie.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\max2pie.def

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\max2pie.h

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\max2pie.cpp

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

DEP_CPP_MAX2P=\
	".\Bmtex.h"\
	".\maxpidef.h"\
	".\Mtlhdr.h"\
	"D:\Maxsdk\Include\Bitmap.h"\
	"d:\maxsdk\include\bmmlib.h"\
	"D:\Maxsdk\Include\Buildver.h"\
	"D:\Maxsdk\Include\Captypes.h"\
	"D:\Maxsdk\Include\Custcont.h"\
	"d:\maxsdk\include\decomp.h"\
	"d:\maxsdk\include\imtl.h"\
	"D:\Maxsdk\Include\Linklist.h"\
	"d:\maxsdk\include\max.h"\
	"D:\Maxsdk\Include\Maxtypes.h"\
	"D:\Maxsdk\Include\Palutil.h"\
	"d:\maxsdk\include\plugapi.h"\
	"D:\Maxsdk\Include\Polyshp.h"\
	"D:\Maxsdk\Include\Shape.h"\
	"D:\Maxsdk\Include\Shphier.h"\
	"D:\Maxsdk\Include\Shpsels.h"\
	"D:\Maxsdk\Include\Spline3d.h"\
	"d:\maxsdk\include\splshape.h"\
	"d:\maxsdk\include\stdmat.h"\
	"d:\maxsdk\include\strbasic.h"\
	"d:\maxsdk\include\texutil.h"\
	"d:\maxsdk\include\trig.h"\
	"D:\Maxsdk\Include\Winutil.h"\
	

"$(INTDIR)\max2pie.obj" : $(SOURCE) $(DEP_CPP_MAX2P) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

DEP_CPP_MAX2P=\
	".\Bmtex.h"\
	".\maxpidef.h"\
	".\Mtlhdr.h"\
	"D:\Maxsdk\Include\Acolor.h"\
	"D:\Maxsdk\Include\Animtbl.h"\
	"D:\Maxsdk\Include\Appio.h"\
	"D:\Maxsdk\Include\Assert1.h"\
	"D:\Maxsdk\Include\Bitarray.h"\
	"D:\Maxsdk\Include\Bitmap.h"\
	"d:\maxsdk\include\bmmlib.h"\
	"D:\Maxsdk\Include\Box2.h"\
	"D:\Maxsdk\Include\Box3.h"\
	"D:\Maxsdk\Include\Buildver.h"\
	"D:\Maxsdk\Include\Captypes.h"\
	"D:\Maxsdk\Include\Channels.h"\
	"D:\Maxsdk\Include\Cmdmode.h"\
	"D:\Maxsdk\Include\Color.h"\
	"D:\Maxsdk\Include\Control.h"\
	"D:\Maxsdk\Include\Coreexp.h"\
	"D:\Maxsdk\Include\Custcont.h"\
	"D:\Maxsdk\Include\Dbgprint.h"\
	"d:\maxsdk\include\decomp.h"\
	"D:\Maxsdk\Include\Dpoint3.h"\
	"D:\Maxsdk\Include\Euler.h"\
	"D:\Maxsdk\Include\Evuser.h"\
	"d:\maxsdk\include\export.h"\
	"D:\Maxsdk\Include\Gencam.h"\
	"D:\Maxsdk\Include\Genhier.h"\
	"D:\Maxsdk\Include\Genlight.h"\
	"D:\Maxsdk\Include\Genshape.h"\
	"D:\Maxsdk\Include\Geom.h"\
	"D:\Maxsdk\Include\Geomlib.h"\
	"D:\Maxsdk\Include\Gfloat.h"\
	"D:\Maxsdk\Include\Gfx.h"\
	"D:\Maxsdk\Include\Gfxlib.h"\
	"D:\Maxsdk\Include\Gutil.h"\
	"d:\maxsdk\include\hitdata.h"\
	"D:\Maxsdk\Include\Hold.h"\
	"D:\Maxsdk\Include\Impapi.h"\
	"D:\Maxsdk\Include\Impexp.h"\
	"d:\maxsdk\include\imtl.h"\
	"D:\Maxsdk\Include\Inode.h"\
	"D:\Maxsdk\Include\Interval.h"\
	"d:\maxsdk\include\ioapi.h"\
	"D:\Maxsdk\Include\Iparamb.h"\
	"D:\Maxsdk\Include\Ipoint2.h"\
	"D:\Maxsdk\Include\Ipoint3.h"\
	"D:\Maxsdk\Include\Linklist.h"\
	"D:\Maxsdk\Include\Lockid.h"\
	"D:\Maxsdk\Include\Log.h"\
	"D:\Maxsdk\Include\Matrix2.h"\
	"D:\Maxsdk\Include\Matrix3.h"\
	"d:\maxsdk\include\max.h"\
	"D:\Maxsdk\Include\Maxapi.h"\
	"D:\Maxsdk\Include\Maxcom.h"\
	"D:\Maxsdk\Include\Maxtess.h"\
	"D:\Maxsdk\Include\Maxtypes.h"\
	"D:\Maxsdk\Include\Mesh.h"\
	"D:\Maxsdk\Include\Meshlib.h"\
	"D:\Maxsdk\Include\Mouseman.h"\
	"D:\Maxsdk\Include\Mtl.h"\
	"D:\Maxsdk\Include\Nametab.h"\
	"D:\Maxsdk\Include\Object.h"\
	"D:\Maxsdk\Include\Objmode.h"\
	"D:\Maxsdk\Include\Palutil.h"\
	"D:\Maxsdk\Include\Patch.h"\
	"D:\Maxsdk\Include\Patchlib.h"\
	"D:\Maxsdk\Include\Patchobj.h"\
	"d:\maxsdk\include\plugapi.h"\
	"D:\Maxsdk\Include\Plugin.h"\
	"D:\Maxsdk\Include\Point2.h"\
	"D:\Maxsdk\Include\Point3.h"\
	"D:\Maxsdk\Include\Point4.h"\
	"D:\Maxsdk\Include\Polyshp.h"\
	"D:\Maxsdk\Include\Ptrvec.h"\
	"D:\Maxsdk\Include\Quat.h"\
	"D:\Maxsdk\Include\Ref.h"\
	"D:\Maxsdk\Include\Render.h"\
	"D:\Maxsdk\Include\Rtclick.h"\
	"D:\Maxsdk\Include\Sceneapi.h"\
	"D:\Maxsdk\Include\Shape.h"\
	"D:\Maxsdk\Include\Shphier.h"\
	"D:\Maxsdk\Include\Shpsels.h"\
	"D:\Maxsdk\Include\Snap.h"\
	"D:\Maxsdk\Include\Soundobj.h"\
	"D:\Maxsdk\Include\Spline3d.h"\
	"d:\maxsdk\include\splshape.h"\
	"D:\Maxsdk\Include\Stack.h"\
	"D:\Maxsdk\Include\Stack3.h"\
	"d:\maxsdk\include\stdmat.h"\
	"d:\maxsdk\include\strbasic.h"\
	"D:\Maxsdk\Include\Strclass.h"\
	"D:\Maxsdk\Include\Tab.h"\
	"d:\maxsdk\include\texutil.h"\
	"d:\maxsdk\include\trig.h"\
	"D:\Maxsdk\Include\Triobj.h"\
	"D:\Maxsdk\Include\Units.h"\
	"D:\Maxsdk\Include\Utilexp.h"\
	"D:\Maxsdk\Include\Utillib.h"\
	"D:\Maxsdk\Include\Vedge.h"\
	"D:\Maxsdk\Include\Winutil.h"\
	

"$(INTDIR)\max2pie.obj" : $(SOURCE) $(DEP_CPP_MAX2P) "$(INTDIR)"

"$(INTDIR)\max2pie.sbr" : $(SOURCE) $(DEP_CPP_MAX2P) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Bmtex.h

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"

!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Bmtex.cpp
DEP_CPP_BMTEX=\
	".\Mtlhdr.h"\
	"D:\Maxsdk\Include\Acolor.h"\
	"D:\Maxsdk\Include\Animtbl.h"\
	"D:\Maxsdk\Include\Appio.h"\
	"D:\Maxsdk\Include\Assert1.h"\
	"D:\Maxsdk\Include\Bitarray.h"\
	"D:\Maxsdk\Include\Bitmap.h"\
	"d:\maxsdk\include\bmmlib.h"\
	"D:\Maxsdk\Include\Box2.h"\
	"D:\Maxsdk\Include\Box3.h"\
	"D:\Maxsdk\Include\Buildver.h"\
	"D:\Maxsdk\Include\Channels.h"\
	"D:\Maxsdk\Include\Cmdmode.h"\
	"D:\Maxsdk\Include\Color.h"\
	"D:\Maxsdk\Include\Control.h"\
	"D:\Maxsdk\Include\Coreexp.h"\
	"D:\Maxsdk\Include\Custcont.h"\
	"D:\Maxsdk\Include\Dbgprint.h"\
	"D:\Maxsdk\Include\Dpoint3.h"\
	"D:\Maxsdk\Include\Euler.h"\
	"D:\Maxsdk\Include\Evuser.h"\
	"d:\maxsdk\include\export.h"\
	"D:\Maxsdk\Include\Gencam.h"\
	"D:\Maxsdk\Include\Genhier.h"\
	"D:\Maxsdk\Include\Genlight.h"\
	"D:\Maxsdk\Include\Genshape.h"\
	"D:\Maxsdk\Include\Geom.h"\
	"D:\Maxsdk\Include\Geomlib.h"\
	"D:\Maxsdk\Include\Gfloat.h"\
	"D:\Maxsdk\Include\Gfx.h"\
	"D:\Maxsdk\Include\Gfxlib.h"\
	"D:\Maxsdk\Include\Gutil.h"\
	"d:\maxsdk\include\hitdata.h"\
	"D:\Maxsdk\Include\Hold.h"\
	"D:\Maxsdk\Include\Impapi.h"\
	"D:\Maxsdk\Include\Impexp.h"\
	"d:\maxsdk\include\imtl.h"\
	"D:\Maxsdk\Include\Inode.h"\
	"D:\Maxsdk\Include\Interval.h"\
	"d:\maxsdk\include\ioapi.h"\
	"D:\Maxsdk\Include\Iparamb.h"\
	"D:\Maxsdk\Include\Ipoint2.h"\
	"D:\Maxsdk\Include\Ipoint3.h"\
	"D:\Maxsdk\Include\Linklist.h"\
	"D:\Maxsdk\Include\Lockid.h"\
	"D:\Maxsdk\Include\Log.h"\
	"D:\Maxsdk\Include\Matrix2.h"\
	"D:\Maxsdk\Include\Matrix3.h"\
	"d:\maxsdk\include\max.h"\
	"D:\Maxsdk\Include\Maxapi.h"\
	"D:\Maxsdk\Include\Maxcom.h"\
	"D:\Maxsdk\Include\Maxtess.h"\
	"D:\Maxsdk\Include\Maxtypes.h"\
	"D:\Maxsdk\Include\Mesh.h"\
	"D:\Maxsdk\Include\Meshlib.h"\
	"D:\Maxsdk\Include\Mouseman.h"\
	"D:\Maxsdk\Include\Mtl.h"\
	"D:\Maxsdk\Include\Nametab.h"\
	"D:\Maxsdk\Include\Object.h"\
	"D:\Maxsdk\Include\Objmode.h"\
	"D:\Maxsdk\Include\Palutil.h"\
	"D:\Maxsdk\Include\Patch.h"\
	"D:\Maxsdk\Include\Patchlib.h"\
	"D:\Maxsdk\Include\Patchobj.h"\
	"d:\maxsdk\include\plugapi.h"\
	"D:\Maxsdk\Include\Plugin.h"\
	"D:\Maxsdk\Include\Point2.h"\
	"D:\Maxsdk\Include\Point3.h"\
	"D:\Maxsdk\Include\Point4.h"\
	"D:\Maxsdk\Include\Ptrvec.h"\
	"D:\Maxsdk\Include\Quat.h"\
	"D:\Maxsdk\Include\Ref.h"\
	"D:\Maxsdk\Include\Render.h"\
	"D:\Maxsdk\Include\Rtclick.h"\
	"D:\Maxsdk\Include\Sceneapi.h"\
	"D:\Maxsdk\Include\Snap.h"\
	"D:\Maxsdk\Include\Soundobj.h"\
	"D:\Maxsdk\Include\Stack.h"\
	"D:\Maxsdk\Include\Stack3.h"\
	"d:\maxsdk\include\stdmat.h"\
	"d:\maxsdk\include\strbasic.h"\
	"D:\Maxsdk\Include\Strclass.h"\
	"D:\Maxsdk\Include\Tab.h"\
	"d:\maxsdk\include\texutil.h"\
	"d:\maxsdk\include\trig.h"\
	"D:\Maxsdk\Include\Triobj.h"\
	"D:\Maxsdk\Include\Units.h"\
	"D:\Maxsdk\Include\Utilexp.h"\
	"D:\Maxsdk\Include\Utillib.h"\
	"D:\Maxsdk\Include\Vedge.h"\
	"D:\Maxsdk\Include\Winutil.h"\
	

!IF  "$(CFG)" == "MAXplugIn - Win32 Release"


"$(INTDIR)\Bmtex.obj" : $(SOURCE) $(DEP_CPP_BMTEX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MAXplugIn - Win32 Debug"


"$(INTDIR)\Bmtex.obj" : $(SOURCE) $(DEP_CPP_BMTEX) "$(INTDIR)"

"$(INTDIR)\Bmtex.sbr" : $(SOURCE) $(DEP_CPP_BMTEX) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
