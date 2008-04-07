# Microsoft Developer Studio Project File - Name="EditWorld" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EditWorld - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EditWorld.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EditWorld.mak" CFG="EditWorld - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EditWorld - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EditWorld - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EditWorld - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "EditWorld - Win32 Release"
# Name "EditWorld - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\aboutdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\autoflagdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\bmphandler.cpp
# End Source File
# Begin Source File

SOURCE=.\brush.cpp
# End Source File
# Begin Source File

SOURCE=.\brushprop.cpp
# End Source File
# Begin Source File

SOURCE=.\btedit.cpp
# End Source File
# Begin Source File

SOURCE=.\bteditdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\bteditview.cpp
# End Source File
# Begin Source File

SOURCE=.\chnkio.cpp
# End Source File
# Begin Source File

SOURCE=.\ddimage.cpp
# End Source File
# Begin Source File

SOURCE=.\debugprint.cpp
# End Source File
# Begin Source File

SOURCE=.\dibdraw.cpp
# End Source File
# Begin Source File

SOURCE=.\directx.cpp
# End Source File
# Begin Source File

SOURCE=.\editworld.rc
# End Source File
# Begin Source File

SOURCE=.\expandlimitsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\fileparse.cpp
# End Source File
# Begin Source File

SOURCE=.\gateway.cpp
# End Source File
# Begin Source File

SOURCE=.\gatewaysup.cpp
# End Source File
# Begin Source File

SOURCE=.\geometry.cpp
# End Source File
# Begin Source File

SOURCE=.\grdland.cpp
# End Source File
# Begin Source File

SOURCE=.\heightmap.cpp
# End Source File
# Begin Source File

SOURCE=.\infodialog.cpp
# End Source File
# Begin Source File

SOURCE=.\initiallimitsdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\keyhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\limitsdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\mainframe.cpp
# End Source File
# Begin Source File

SOURCE=.\mapprefs.cpp
# End Source File
# Begin Source File

SOURCE=.\objectproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\pasteprefs.cpp
# End Source File
# Begin Source File

SOURCE=.\pcxhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\playermap.cpp
# End Source File
# Begin Source File

SOURCE=..\README
# End Source File
# Begin Source File

SOURCE=.\savesegmentdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\tdview.cpp
# End Source File
# Begin Source File

SOURCE=.\textsel.cpp
# End Source File
# Begin Source File

SOURCE=.\textureprefs.cpp
# End Source File
# Begin Source File

SOURCE=.\textureview.cpp
# End Source File
# Begin Source File

SOURCE=.\tiletypes.cpp
# End Source File
# Begin Source File

SOURCE=.\undoredo.cpp
# End Source File
# Begin Source File

SOURCE=.\wfview.cpp
# End Source File
# Begin Source File

SOURCE=.\winapi.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\aboutdialog.hpp
# End Source File
# Begin Source File

SOURCE=.\autoflagdialog.hpp
# End Source File
# Begin Source File

SOURCE=.\bmphandler.h
# End Source File
# Begin Source File

SOURCE=.\brush.h
# End Source File
# Begin Source File

SOURCE=.\brushprop.h
# End Source File
# Begin Source File

SOURCE=.\btedit.h
# End Source File
# Begin Source File

SOURCE=.\bteditdoc.h
# End Source File
# Begin Source File

SOURCE=.\bteditview.h
# End Source File
# Begin Source File

SOURCE=.\chnkio.h
# End Source File
# Begin Source File

SOURCE=.\ddimage.h
# End Source File
# Begin Source File

SOURCE=.\debugprint.hpp
# End Source File
# Begin Source File

SOURCE=.\devmap.h
# End Source File
# Begin Source File

SOURCE=.\dibdraw.h
# End Source File
# Begin Source File

SOURCE=.\directx.h
# End Source File
# Begin Source File

SOURCE=.\expandlimitsdlg.h
# End Source File
# Begin Source File

SOURCE=.\fileparse.h
# End Source File
# Begin Source File

SOURCE=.\gateway.h
# End Source File
# Begin Source File

SOURCE=.\gateway.hpp
# End Source File
# Begin Source File

SOURCE=.\gatewaydef.h
# End Source File
# Begin Source File

SOURCE=.\geometry.h
# End Source File
# Begin Source File

SOURCE=.\grdland.h
# End Source File
# Begin Source File

SOURCE=.\heightmap.h
# End Source File
# Begin Source File

SOURCE=.\infodialog.hpp
# End Source File
# Begin Source File

SOURCE=.\initiallimitsdialog.hpp
# End Source File
# Begin Source File

SOURCE=.\keyhandler.hpp
# End Source File
# Begin Source File

SOURCE=.\limitsdialog.hpp
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\mainframe.h
# End Source File
# Begin Source File

SOURCE=.\mapprefs.h
# End Source File
# Begin Source File

SOURCE=.\objectproperties.h
# End Source File
# Begin Source File

SOURCE=.\pasteprefs.h
# End Source File
# Begin Source File

SOURCE=.\pcxhandler.h
# End Source File
# Begin Source File

SOURCE=.\playermap.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\savesegmentdialog.hpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tdview.h
# End Source File
# Begin Source File

SOURCE=.\textsel.h
# End Source File
# Begin Source File

SOURCE=.\textureprefs.h
# End Source File
# Begin Source File

SOURCE=.\textureview.h
# End Source File
# Begin Source File

SOURCE=.\tiletypes.h
# End Source File
# Begin Source File

SOURCE=.\typedefs.h
# End Source File
# Begin Source File

SOURCE=.\undoredo.h
# End Source File
# Begin Source File

SOURCE=.\wfview.h
# End Source File
# Begin Source File

SOURCE=.\winapi.hpp
# End Source File
# Begin Source File

SOURCE=.\winstuff.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\arrow.cur
# End Source File
# Begin Source File

SOURCE=.\res\arrowcop.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btedit.ico
# End Source File
# Begin Source File

SOURCE=.\res\btedit.rc2
# End Source File
# Begin Source File

SOURCE=.\res\bteditdoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\height_d.cur
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\nodrop.cur
# End Source File
# Begin Source File

SOURCE=.\res\pointer_.cur
# End Source File
# Begin Source File

SOURCE=.\res\selectre.cur
# End Source File
# Begin Source File

SOURCE=.\res\smallbru.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# End Group
# End Target
# End Project
