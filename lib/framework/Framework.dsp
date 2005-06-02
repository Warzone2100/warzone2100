# Microsoft Developer Studio Project File - Name="Framework" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Framework - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Framework.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Framework.mak" CFG="Framework - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Framework - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Framework - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Framework - Win32 Release"

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

!ELSEIF  "$(CFG)" == "Framework - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\Framework" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "YY_STATIC" /FR /YX /FD /GZ /c
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

# Name "Framework - Win32 Release"
# Name "Framework - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Block.c
# End Source File
# Begin Source File

SOURCE=.\Cursor.c
# End Source File
# Begin Source File

SOURCE=.\Dderror.c
# End Source File
# Begin Source File

SOURCE=.\Debug.c
# End Source File
# Begin Source File

SOURCE=.\DXInput.c
# End Source File
# Begin Source File

SOURCE=.\Font.c
# End Source File
# Begin Source File

SOURCE=.\Frame.c
# End Source File
# Begin Source File

SOURCE=.\FrameResource.c
# End Source File
# Begin Source File

SOURCE=.\Heap.c
# End Source File
# Begin Source File

SOURCE=.\Image.c
# End Source File
# Begin Source File

SOURCE=.\Input.c
# End Source File
# Begin Source File

SOURCE=.\Mem.c
# End Source File
# Begin Source File

SOURCE=.\Mono.c
# End Source File
# Begin Source File

SOURCE=.\multiWDG.c
# End Source File
# Begin Source File

SOURCE=.\resource_l.c
# End Source File
# Begin Source File

SOURCE=.\resource_y.c
# End Source File
# Begin Source File

SOURCE=.\Screen.c
# End Source File
# Begin Source File

SOURCE=.\StrRes.c
# End Source File
# Begin Source File

SOURCE=.\StrRes_l.c
# End Source File
# Begin Source File

SOURCE=.\StrRes_y.c
# End Source File
# Begin Source File

SOURCE=.\Surface.c
# End Source File
# Begin Source File

SOURCE=.\Treap.c
# End Source File
# Begin Source File

SOURCE=.\Trig.c
# End Source File
# Begin Source File

SOURCE=.\W95trace.c
# End Source File
# Begin Source File

SOURCE=.\Wdg.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Block.h
# End Source File
# Begin Source File

SOURCE=.\Cursor.h
# End Source File
# Begin Source File

SOURCE=.\Dderror.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\DXInput.h
# End Source File
# Begin Source File

SOURCE=.\Font.h
# End Source File
# Begin Source File

SOURCE=.\fractions.h
# End Source File
# Begin Source File

SOURCE=.\Frame.h
# End Source File
# Begin Source File

SOURCE=.\FrameInt.h
# End Source File
# Begin Source File

SOURCE=.\FrameResource.h
# End Source File
# Begin Source File

SOURCE=.\Heap.h
# End Source File
# Begin Source File

SOURCE=.\Image.h
# End Source File
# Begin Source File

SOURCE=.\Input.h
# End Source File
# Begin Source File

SOURCE=.\ListMacs.h
# End Source File
# Begin Source File

SOURCE=.\Mem.h
# End Source File
# Begin Source File

SOURCE=.\MemInt.h
# End Source File
# Begin Source File

SOURCE=.\Mono.h
# End Source File
# Begin Source File

SOURCE=.\multiWDG.h
# End Source File
# Begin Source File

SOURCE=.\ResLY.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\resource_y.h
# End Source File
# Begin Source File

SOURCE=.\Screen.h
# End Source File
# Begin Source File

SOURCE=.\StrRes.h
# End Source File
# Begin Source File

SOURCE=.\StrRes_y.h
# End Source File
# Begin Source File

SOURCE=.\StrResLY.h
# End Source File
# Begin Source File

SOURCE=.\Surface.h
# End Source File
# Begin Source File

SOURCE=.\Treap.h
# End Source File
# Begin Source File

SOURCE=.\TreapInt.h
# End Source File
# Begin Source File

SOURCE=.\Trig.h
# End Source File
# Begin Source File

SOURCE=.\Types.h
# End Source File
# Begin Source File

SOURCE=.\W95trace.h
# End Source File
# Begin Source File

SOURCE=.\Wdg.h
# End Source File
# End Group
# End Target
# End Project
