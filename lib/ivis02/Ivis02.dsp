# Microsoft Developer Studio Project File - Name="Ivis02" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Ivis02 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Ivis02.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ivis02.mak" CFG="Ivis02 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ivis02 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Ivis02 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Ivis02 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Ivis02 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\Framework" /I "c:\glide\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "INC_GLIDE" /D "__MSC__" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Ivis02 - Win32 Release"
# Name "Ivis02 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\3dfxdyn.c
# End Source File
# Begin Source File

SOURCE=.\3dfxFunc.c
# End Source File
# Begin Source File

SOURCE=.\3dfxmode.c
# End Source File
# Begin Source File

SOURCE=.\3dfxtext.c
# End Source File
# Begin Source File

SOURCE=.\BitImage.c
# End Source File
# Begin Source File

SOURCE=.\Bspimd.c
# End Source File
# Begin Source File

SOURCE=.\Bug.c
# End Source File
# Begin Source File

SOURCE=.\D3dmode.c
# End Source File
# Begin Source File

SOURCE=.\d3drender.c
# End Source File
# Begin Source File

SOURCE=.\dx6TexMan.c
# End Source File
# Begin Source File

SOURCE=.\Fbf.c
# End Source File
# Begin Source File

SOURCE=.\Getdxver.cpp
# End Source File
# Begin Source File

SOURCE=.\Imd.c
# End Source File
# Begin Source File

SOURCE=.\Imdload.c
# End Source File
# Begin Source File

SOURCE=.\Ivi.c
# End Source File
# Begin Source File

SOURCE=.\Pcx.c
# End Source File
# Begin Source File

SOURCE=.\pieBlitFunc.c
# End Source File
# Begin Source File

SOURCE=.\pieClip.c
# End Source File
# Begin Source File

SOURCE=.\Piedraw.c
# End Source File
# Begin Source File

SOURCE=.\Piefunc.c
# End Source File
# Begin Source File

SOURCE=.\pieMatrix.c
# End Source File
# Begin Source File

SOURCE=.\pieMode.c
# End Source File
# Begin Source File

SOURCE=.\piePalette.c
# End Source File
# Begin Source File

SOURCE=.\pieState.c
# End Source File
# Begin Source File

SOURCE=.\pieTexture.c
# End Source File
# Begin Source File

SOURCE=.\Rendfunc.c
# End Source File
# Begin Source File

SOURCE=.\Rendmode.c
# End Source File
# Begin Source File

SOURCE=.\Tex.c
# End Source File
# Begin Source File

SOURCE=.\Texd3d.c
# End Source File
# Begin Source File

SOURCE=.\TextDraw.c
# End Source File
# Begin Source File

SOURCE=.\V4101.c
# End Source File
# Begin Source File

SOURCE=.\Vsr.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\3dfxFunc.h
# End Source File
# Begin Source File

SOURCE=.\3dfxmode.h
# End Source File
# Begin Source File

SOURCE=.\3dfxText.h
# End Source File
# Begin Source File

SOURCE=.\Amd3d.h
# End Source File
# Begin Source File

SOURCE=.\BitImage.h
# End Source File
# Begin Source File

SOURCE=.\Bspfunc.h
# End Source File
# Begin Source File

SOURCE=.\Bspimd.h
# End Source File
# Begin Source File

SOURCE=.\Bug.h
# End Source File
# Begin Source File

SOURCE=.\D3dmode.h
# End Source File
# Begin Source File

SOURCE=.\d3drender.h
# End Source File
# Begin Source File

SOURCE=.\Dglide.h
# End Source File
# Begin Source File

SOURCE=.\dx6TexMan.h
# End Source File
# Begin Source File

SOURCE=.\Fbf.h
# End Source File
# Begin Source File

SOURCE=.\Geo.h
# End Source File
# Begin Source File

SOURCE=.\Imd.h
# End Source File
# Begin Source File

SOURCE=.\Ivi.h
# End Source File
# Begin Source File

SOURCE=.\Ivisdef.h
# End Source File
# Begin Source File

SOURCE=.\ivispatch.h
# End Source File
# Begin Source File

SOURCE=.\Pcx.h
# End Source File
# Begin Source File

SOURCE=.\pieBlitFunc.h
# End Source File
# Begin Source File

SOURCE=.\Pieclip.h
# End Source File
# Begin Source File

SOURCE=.\Piedef.h
# End Source File
# Begin Source File

SOURCE=.\Piefunc.h
# End Source File
# Begin Source File

SOURCE=.\pieMatrix.h
# End Source File
# Begin Source File

SOURCE=.\pieMode.h
# End Source File
# Begin Source File

SOURCE=.\piePalette.h
# End Source File
# Begin Source File

SOURCE=.\pieState.h
# End Source File
# Begin Source File

SOURCE=.\pieTexture.h
# End Source File
# Begin Source File

SOURCE=.\pieTypes.h
# End Source File
# Begin Source File

SOURCE=.\Rendfunc.h
# End Source File
# Begin Source File

SOURCE=.\Rendmode.h
# End Source File
# Begin Source File

SOURCE=.\Tex.h
# End Source File
# Begin Source File

SOURCE=.\Texd3d.h
# End Source File
# Begin Source File

SOURCE=.\TextDraw.h
# End Source File
# Begin Source File

SOURCE=.\V4101.h
# End Source File
# Begin Source File

SOURCE=.\Vid.h
# End Source File
# Begin Source File

SOURCE=.\Vsr.h
# End Source File
# End Group
# End Target
# End Project
