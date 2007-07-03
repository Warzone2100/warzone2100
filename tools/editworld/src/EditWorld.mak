# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=EditWorld - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to EditWorld - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "EditWorld - Win32 Release" && "$(CFG)" !=\
 "EditWorld - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "EditWorld.mak" CFG="EditWorld - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EditWorld - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EditWorld - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "EditWorld - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "EditWorld - Win32 Release"

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

ALL : "$(OUTDIR)\EditWorld.exe" "$(OUTDIR)\EditWorld.bsc"

CLEAN : 
	-@erase "$(INTDIR)\AutoFlagDialog.obj"
	-@erase "$(INTDIR)\AutoFlagDialog.sbr"
	-@erase "$(INTDIR)\bmpHandler.obj"
	-@erase "$(INTDIR)\bmpHandler.sbr"
	-@erase "$(INTDIR)\Brush.obj"
	-@erase "$(INTDIR)\Brush.sbr"
	-@erase "$(INTDIR)\BrushProp.obj"
	-@erase "$(INTDIR)\BrushProp.sbr"
	-@erase "$(INTDIR)\BTEdit.obj"
	-@erase "$(INTDIR)\BTEdit.sbr"
	-@erase "$(INTDIR)\BTEditDoc.obj"
	-@erase "$(INTDIR)\BTEditDoc.sbr"
	-@erase "$(INTDIR)\BTEditView.obj"
	-@erase "$(INTDIR)\BTEditView.sbr"
	-@erase "$(INTDIR)\ChnkIO.obj"
	-@erase "$(INTDIR)\ChnkIO.sbr"
	-@erase "$(INTDIR)\DDImage.obj"
	-@erase "$(INTDIR)\DDImage.sbr"
	-@erase "$(INTDIR)\DebugPrint.obj"
	-@erase "$(INTDIR)\DebugPrint.sbr"
	-@erase "$(INTDIR)\DIBDraw.obj"
	-@erase "$(INTDIR)\DIBDraw.sbr"
	-@erase "$(INTDIR)\DirectX.obj"
	-@erase "$(INTDIR)\DirectX.sbr"
	-@erase "$(INTDIR)\EditWorld.res"
	-@erase "$(INTDIR)\ExpandLimitsDlg.obj"
	-@erase "$(INTDIR)\ExpandLimitsDlg.sbr"
	-@erase "$(INTDIR)\ExportInfo.obj"
	-@erase "$(INTDIR)\ExportInfo.sbr"
	-@erase "$(INTDIR)\FileParse.obj"
	-@erase "$(INTDIR)\FileParse.sbr"
	-@erase "$(INTDIR)\GateInterface.obj"
	-@erase "$(INTDIR)\GateInterface.sbr"
	-@erase "$(INTDIR)\Gateway.obj"
	-@erase "$(INTDIR)\Gateway.sbr"
	-@erase "$(INTDIR)\GatewaySup.obj"
	-@erase "$(INTDIR)\GatewaySup.sbr"
	-@erase "$(INTDIR)\Geometry.obj"
	-@erase "$(INTDIR)\Geometry.sbr"
	-@erase "$(INTDIR)\GrdLand.obj"
	-@erase "$(INTDIR)\GrdLand.sbr"
	-@erase "$(INTDIR)\HeightMap.obj"
	-@erase "$(INTDIR)\HeightMap.sbr"
	-@erase "$(INTDIR)\InitialLimitsDlg.obj"
	-@erase "$(INTDIR)\InitialLimitsDlg.sbr"
	-@erase "$(INTDIR)\KeyHandler.obj"
	-@erase "$(INTDIR)\KeyHandler.sbr"
	-@erase "$(INTDIR)\LimitsDialog.obj"
	-@erase "$(INTDIR)\LimitsDialog.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\MapPrefs.obj"
	-@erase "$(INTDIR)\MapPrefs.sbr"
	-@erase "$(INTDIR)\ObjectProperties.obj"
	-@erase "$(INTDIR)\ObjectProperties.sbr"
	-@erase "$(INTDIR)\PastePrefs.obj"
	-@erase "$(INTDIR)\PastePrefs.sbr"
	-@erase "$(INTDIR)\PCXHandler.obj"
	-@erase "$(INTDIR)\PCXHandler.sbr"
	-@erase "$(INTDIR)\PlayerMap.obj"
	-@erase "$(INTDIR)\PlayerMap.sbr"
	-@erase "$(INTDIR)\SaveSegmentDialog.obj"
	-@erase "$(INTDIR)\SaveSegmentDialog.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TDView.obj"
	-@erase "$(INTDIR)\TDView.sbr"
	-@erase "$(INTDIR)\TextSel.obj"
	-@erase "$(INTDIR)\TextSel.sbr"
	-@erase "$(INTDIR)\TexturePrefs.obj"
	-@erase "$(INTDIR)\TexturePrefs.sbr"
	-@erase "$(INTDIR)\TextureView.obj"
	-@erase "$(INTDIR)\TextureView.sbr"
	-@erase "$(INTDIR)\TileTypes.obj"
	-@erase "$(INTDIR)\TileTypes.sbr"
	-@erase "$(INTDIR)\WFView.obj"
	-@erase "$(INTDIR)\WFView.sbr"
	-@erase "$(OUTDIR)\EditWorld.bsc"
	-@erase "$(OUTDIR)\EditWorld.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDITORWORLD" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "EDITORWORLD" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/EditWorld.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/EditWorld.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/EditWorld.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AutoFlagDialog.sbr" \
	"$(INTDIR)\bmpHandler.sbr" \
	"$(INTDIR)\Brush.sbr" \
	"$(INTDIR)\BrushProp.sbr" \
	"$(INTDIR)\BTEdit.sbr" \
	"$(INTDIR)\BTEditDoc.sbr" \
	"$(INTDIR)\BTEditView.sbr" \
	"$(INTDIR)\ChnkIO.sbr" \
	"$(INTDIR)\DDImage.sbr" \
	"$(INTDIR)\DebugPrint.sbr" \
	"$(INTDIR)\DIBDraw.sbr" \
	"$(INTDIR)\DirectX.sbr" \
	"$(INTDIR)\ExpandLimitsDlg.sbr" \
	"$(INTDIR)\ExportInfo.sbr" \
	"$(INTDIR)\FileParse.sbr" \
	"$(INTDIR)\GateInterface.sbr" \
	"$(INTDIR)\Gateway.sbr" \
	"$(INTDIR)\GatewaySup.sbr" \
	"$(INTDIR)\Geometry.sbr" \
	"$(INTDIR)\GrdLand.sbr" \
	"$(INTDIR)\HeightMap.sbr" \
	"$(INTDIR)\InitialLimitsDlg.sbr" \
	"$(INTDIR)\KeyHandler.sbr" \
	"$(INTDIR)\LimitsDialog.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\MapPrefs.sbr" \
	"$(INTDIR)\ObjectProperties.sbr" \
	"$(INTDIR)\PastePrefs.sbr" \
	"$(INTDIR)\PCXHandler.sbr" \
	"$(INTDIR)\PlayerMap.sbr" \
	"$(INTDIR)\SaveSegmentDialog.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TDView.sbr" \
	"$(INTDIR)\TextSel.sbr" \
	"$(INTDIR)\TexturePrefs.sbr" \
	"$(INTDIR)\TextureView.sbr" \
	"$(INTDIR)\TileTypes.sbr" \
	"$(INTDIR)\WFView.sbr"

"$(OUTDIR)\EditWorld.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/EditWorld.pdb" /machine:I386\
 /out:"$(OUTDIR)/EditWorld.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AutoFlagDialog.obj" \
	"$(INTDIR)\bmpHandler.obj" \
	"$(INTDIR)\Brush.obj" \
	"$(INTDIR)\BrushProp.obj" \
	"$(INTDIR)\BTEdit.obj" \
	"$(INTDIR)\BTEditDoc.obj" \
	"$(INTDIR)\BTEditView.obj" \
	"$(INTDIR)\ChnkIO.obj" \
	"$(INTDIR)\DDImage.obj" \
	"$(INTDIR)\DebugPrint.obj" \
	"$(INTDIR)\DIBDraw.obj" \
	"$(INTDIR)\DirectX.obj" \
	"$(INTDIR)\EditWorld.res" \
	"$(INTDIR)\ExpandLimitsDlg.obj" \
	"$(INTDIR)\ExportInfo.obj" \
	"$(INTDIR)\FileParse.obj" \
	"$(INTDIR)\GateInterface.obj" \
	"$(INTDIR)\Gateway.obj" \
	"$(INTDIR)\GatewaySup.obj" \
	"$(INTDIR)\Geometry.obj" \
	"$(INTDIR)\GrdLand.obj" \
	"$(INTDIR)\HeightMap.obj" \
	"$(INTDIR)\InitialLimitsDlg.obj" \
	"$(INTDIR)\KeyHandler.obj" \
	"$(INTDIR)\LimitsDialog.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MapPrefs.obj" \
	"$(INTDIR)\ObjectProperties.obj" \
	"$(INTDIR)\PastePrefs.obj" \
	"$(INTDIR)\PCXHandler.obj" \
	"$(INTDIR)\PlayerMap.obj" \
	"$(INTDIR)\SaveSegmentDialog.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TDView.obj" \
	"$(INTDIR)\TextSel.obj" \
	"$(INTDIR)\TexturePrefs.obj" \
	"$(INTDIR)\TextureView.obj" \
	"$(INTDIR)\TileTypes.obj" \
	"$(INTDIR)\WFView.obj"

"$(OUTDIR)\EditWorld.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

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

ALL : "$(OUTDIR)\EditWorld.exe" "$(OUTDIR)\EditWorld.bsc"

CLEAN : 
	-@erase "$(INTDIR)\AutoFlagDialog.obj"
	-@erase "$(INTDIR)\AutoFlagDialog.sbr"
	-@erase "$(INTDIR)\bmpHandler.obj"
	-@erase "$(INTDIR)\bmpHandler.sbr"
	-@erase "$(INTDIR)\Brush.obj"
	-@erase "$(INTDIR)\Brush.sbr"
	-@erase "$(INTDIR)\BrushProp.obj"
	-@erase "$(INTDIR)\BrushProp.sbr"
	-@erase "$(INTDIR)\BTEdit.obj"
	-@erase "$(INTDIR)\BTEdit.sbr"
	-@erase "$(INTDIR)\BTEditDoc.obj"
	-@erase "$(INTDIR)\BTEditDoc.sbr"
	-@erase "$(INTDIR)\BTEditView.obj"
	-@erase "$(INTDIR)\BTEditView.sbr"
	-@erase "$(INTDIR)\ChnkIO.obj"
	-@erase "$(INTDIR)\ChnkIO.sbr"
	-@erase "$(INTDIR)\DDImage.obj"
	-@erase "$(INTDIR)\DDImage.sbr"
	-@erase "$(INTDIR)\DebugPrint.obj"
	-@erase "$(INTDIR)\DebugPrint.sbr"
	-@erase "$(INTDIR)\DIBDraw.obj"
	-@erase "$(INTDIR)\DIBDraw.sbr"
	-@erase "$(INTDIR)\DirectX.obj"
	-@erase "$(INTDIR)\DirectX.sbr"
	-@erase "$(INTDIR)\EditWorld.res"
	-@erase "$(INTDIR)\ExpandLimitsDlg.obj"
	-@erase "$(INTDIR)\ExpandLimitsDlg.sbr"
	-@erase "$(INTDIR)\ExportInfo.obj"
	-@erase "$(INTDIR)\ExportInfo.sbr"
	-@erase "$(INTDIR)\FileParse.obj"
	-@erase "$(INTDIR)\FileParse.sbr"
	-@erase "$(INTDIR)\GateInterface.obj"
	-@erase "$(INTDIR)\GateInterface.sbr"
	-@erase "$(INTDIR)\Gateway.obj"
	-@erase "$(INTDIR)\Gateway.sbr"
	-@erase "$(INTDIR)\GatewaySup.obj"
	-@erase "$(INTDIR)\GatewaySup.sbr"
	-@erase "$(INTDIR)\Geometry.obj"
	-@erase "$(INTDIR)\Geometry.sbr"
	-@erase "$(INTDIR)\GrdLand.obj"
	-@erase "$(INTDIR)\GrdLand.sbr"
	-@erase "$(INTDIR)\HeightMap.obj"
	-@erase "$(INTDIR)\HeightMap.sbr"
	-@erase "$(INTDIR)\InitialLimitsDlg.obj"
	-@erase "$(INTDIR)\InitialLimitsDlg.sbr"
	-@erase "$(INTDIR)\KeyHandler.obj"
	-@erase "$(INTDIR)\KeyHandler.sbr"
	-@erase "$(INTDIR)\LimitsDialog.obj"
	-@erase "$(INTDIR)\LimitsDialog.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\MapPrefs.obj"
	-@erase "$(INTDIR)\MapPrefs.sbr"
	-@erase "$(INTDIR)\ObjectProperties.obj"
	-@erase "$(INTDIR)\ObjectProperties.sbr"
	-@erase "$(INTDIR)\PastePrefs.obj"
	-@erase "$(INTDIR)\PastePrefs.sbr"
	-@erase "$(INTDIR)\PCXHandler.obj"
	-@erase "$(INTDIR)\PCXHandler.sbr"
	-@erase "$(INTDIR)\PlayerMap.obj"
	-@erase "$(INTDIR)\PlayerMap.sbr"
	-@erase "$(INTDIR)\SaveSegmentDialog.obj"
	-@erase "$(INTDIR)\SaveSegmentDialog.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TDView.obj"
	-@erase "$(INTDIR)\TDView.sbr"
	-@erase "$(INTDIR)\TextSel.obj"
	-@erase "$(INTDIR)\TextSel.sbr"
	-@erase "$(INTDIR)\TexturePrefs.obj"
	-@erase "$(INTDIR)\TexturePrefs.sbr"
	-@erase "$(INTDIR)\TextureView.obj"
	-@erase "$(INTDIR)\TextureView.sbr"
	-@erase "$(INTDIR)\TileTypes.obj"
	-@erase "$(INTDIR)\TileTypes.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\WFView.obj"
	-@erase "$(INTDIR)\WFView.sbr"
	-@erase "$(OUTDIR)\EditWorld.bsc"
	-@erase "$(OUTDIR)\EditWorld.exe"
	-@erase "$(OUTDIR)\EditWorld.ilk"
	-@erase "$(OUTDIR)\EditWorld.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDITORWORLD" /FR /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /D "EDITORWORLD" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/EditWorld.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/EditWorld.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/EditWorld.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AutoFlagDialog.sbr" \
	"$(INTDIR)\bmpHandler.sbr" \
	"$(INTDIR)\Brush.sbr" \
	"$(INTDIR)\BrushProp.sbr" \
	"$(INTDIR)\BTEdit.sbr" \
	"$(INTDIR)\BTEditDoc.sbr" \
	"$(INTDIR)\BTEditView.sbr" \
	"$(INTDIR)\ChnkIO.sbr" \
	"$(INTDIR)\DDImage.sbr" \
	"$(INTDIR)\DebugPrint.sbr" \
	"$(INTDIR)\DIBDraw.sbr" \
	"$(INTDIR)\DirectX.sbr" \
	"$(INTDIR)\ExpandLimitsDlg.sbr" \
	"$(INTDIR)\ExportInfo.sbr" \
	"$(INTDIR)\FileParse.sbr" \
	"$(INTDIR)\GateInterface.sbr" \
	"$(INTDIR)\Gateway.sbr" \
	"$(INTDIR)\GatewaySup.sbr" \
	"$(INTDIR)\Geometry.sbr" \
	"$(INTDIR)\GrdLand.sbr" \
	"$(INTDIR)\HeightMap.sbr" \
	"$(INTDIR)\InitialLimitsDlg.sbr" \
	"$(INTDIR)\KeyHandler.sbr" \
	"$(INTDIR)\LimitsDialog.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\MapPrefs.sbr" \
	"$(INTDIR)\ObjectProperties.sbr" \
	"$(INTDIR)\PastePrefs.sbr" \
	"$(INTDIR)\PCXHandler.sbr" \
	"$(INTDIR)\PlayerMap.sbr" \
	"$(INTDIR)\SaveSegmentDialog.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TDView.sbr" \
	"$(INTDIR)\TextSel.sbr" \
	"$(INTDIR)\TexturePrefs.sbr" \
	"$(INTDIR)\TextureView.sbr" \
	"$(INTDIR)\TileTypes.sbr" \
	"$(INTDIR)\WFView.sbr"

"$(OUTDIR)\EditWorld.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=winmm.lib dxguid.lib ddraw.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/EditWorld.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/EditWorld.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AutoFlagDialog.obj" \
	"$(INTDIR)\bmpHandler.obj" \
	"$(INTDIR)\Brush.obj" \
	"$(INTDIR)\BrushProp.obj" \
	"$(INTDIR)\BTEdit.obj" \
	"$(INTDIR)\BTEditDoc.obj" \
	"$(INTDIR)\BTEditView.obj" \
	"$(INTDIR)\ChnkIO.obj" \
	"$(INTDIR)\DDImage.obj" \
	"$(INTDIR)\DebugPrint.obj" \
	"$(INTDIR)\DIBDraw.obj" \
	"$(INTDIR)\DirectX.obj" \
	"$(INTDIR)\EditWorld.res" \
	"$(INTDIR)\ExpandLimitsDlg.obj" \
	"$(INTDIR)\ExportInfo.obj" \
	"$(INTDIR)\FileParse.obj" \
	"$(INTDIR)\GateInterface.obj" \
	"$(INTDIR)\Gateway.obj" \
	"$(INTDIR)\GatewaySup.obj" \
	"$(INTDIR)\Geometry.obj" \
	"$(INTDIR)\GrdLand.obj" \
	"$(INTDIR)\HeightMap.obj" \
	"$(INTDIR)\InitialLimitsDlg.obj" \
	"$(INTDIR)\KeyHandler.obj" \
	"$(INTDIR)\LimitsDialog.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MapPrefs.obj" \
	"$(INTDIR)\ObjectProperties.obj" \
	"$(INTDIR)\PastePrefs.obj" \
	"$(INTDIR)\PCXHandler.obj" \
	"$(INTDIR)\PlayerMap.obj" \
	"$(INTDIR)\SaveSegmentDialog.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TDView.obj" \
	"$(INTDIR)\TextSel.obj" \
	"$(INTDIR)\TexturePrefs.obj" \
	"$(INTDIR)\TextureView.obj" \
	"$(INTDIR)\TileTypes.obj" \
	"$(INTDIR)\WFView.obj"

"$(OUTDIR)\EditWorld.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "EditWorld - Win32 Release"
# Name "EditWorld - Win32 Debug"

!IF  "$(CFG)" == "EditWorld - Win32 Release"

!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\WFView.cpp
DEP_CPP_WFVIE=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\ObjectProperties.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\WFView.obj" : $(SOURCE) $(DEP_CPP_WFVIE) "$(INTDIR)"

"$(INTDIR)\WFView.sbr" : $(SOURCE) $(DEP_CPP_WFVIE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Brush.cpp

!IF  "$(CFG)" == "EditWorld - Win32 Release"

DEP_CPP_BRUSH=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\Brush.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\StdAfx.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\Brush.obj" : $(SOURCE) $(DEP_CPP_BRUSH) "$(INTDIR)"

"$(INTDIR)\Brush.sbr" : $(SOURCE) $(DEP_CPP_BRUSH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

DEP_CPP_BRUSH=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\Brush.obj" : $(SOURCE) $(DEP_CPP_BRUSH) "$(INTDIR)"

"$(INTDIR)\Brush.sbr" : $(SOURCE) $(DEP_CPP_BRUSH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BTEdit.cpp
DEP_CPP_BTEDI=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BTEdit.obj" : $(SOURCE) $(DEP_CPP_BTEDI) "$(INTDIR)"

"$(INTDIR)\BTEdit.sbr" : $(SOURCE) $(DEP_CPP_BTEDI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\BTEditDoc.cpp

!IF  "$(CFG)" == "EditWorld - Win32 Release"

DEP_CPP_BTEDIT=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\ExpandLimitsDlg.h"\
	".\ExportInfo.h"\
	".\GateInterface.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\InitialLimitsDlg.h"\
	".\KeyHandler.h"\
	".\LimitsDialog.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\MapPrefs.h"\
	".\PastePrefs.h"\
	".\PCXHandler.h"\
	".\PlayerMap.h"\
	".\SaveSegmentDialog.h"\
	".\SnapPrefs.h"\
	".\StdAfx.h"\
	".\TexturePrefs.h"\
	".\TextureView.h"\
	".\typedefs.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BTEditDoc.obj" : $(SOURCE) $(DEP_CPP_BTEDIT) "$(INTDIR)"

"$(INTDIR)\BTEditDoc.sbr" : $(SOURCE) $(DEP_CPP_BTEDIT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

DEP_CPP_BTEDIT=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\ExpandLimitsDlg.h"\
	".\ExportInfo.h"\
	".\FileParse.h"\
	".\GateInterface.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\InitialLimitsDlg.h"\
	".\KeyHandler.h"\
	".\LimitsDialog.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\MapPrefs.h"\
	".\PastePrefs.h"\
	".\PCXHandler.h"\
	".\PlayerMap.h"\
	".\SaveSegmentDialog.h"\
	".\SnapPrefs.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TexturePrefs.h"\
	".\TextureView.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BTEditDoc.obj" : $(SOURCE) $(DEP_CPP_BTEDIT) "$(INTDIR)"

"$(INTDIR)\BTEditDoc.sbr" : $(SOURCE) $(DEP_CPP_BTEDIT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BTEditView.cpp
DEP_CPP_BTEDITV=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\ObjectProperties.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BTEditView.obj" : $(SOURCE) $(DEP_CPP_BTEDITV) "$(INTDIR)"

"$(INTDIR)\BTEditView.sbr" : $(SOURCE) $(DEP_CPP_BTEDITV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ChnkIO.cpp
DEP_CPP_CHNKI=\
	".\ChnkIO.h"\
	".\DebugPrint.h"\
	".\typedefs.h"\
	

"$(INTDIR)\ChnkIO.obj" : $(SOURCE) $(DEP_CPP_CHNKI) "$(INTDIR)"

"$(INTDIR)\ChnkIO.sbr" : $(SOURCE) $(DEP_CPP_CHNKI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DDImage.cpp
DEP_CPP_DDIMA=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\macros.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\DDImage.obj" : $(SOURCE) $(DEP_CPP_DDIMA) "$(INTDIR)"

"$(INTDIR)\DDImage.sbr" : $(SOURCE) $(DEP_CPP_DDIMA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DIBDraw.cpp
DEP_CPP_DIBDR=\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	

"$(INTDIR)\DIBDraw.obj" : $(SOURCE) $(DEP_CPP_DIBDR) "$(INTDIR)"

"$(INTDIR)\DIBDraw.sbr" : $(SOURCE) $(DEP_CPP_DIBDR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DirectX.cpp
DEP_CPP_DIREC=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\D3DWrap.h"\
	".\DebugPrint.h"\
	".\DirectX.h"\
	".\Geometry.h"\
	".\macros.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\DirectX.obj" : $(SOURCE) $(DEP_CPP_DIREC) "$(INTDIR)"

"$(INTDIR)\DirectX.sbr" : $(SOURCE) $(DEP_CPP_DIREC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\FileParse.cpp
DEP_CPP_FILEP=\
	".\DebugPrint.h"\
	".\FileParse.h"\
	

"$(INTDIR)\FileParse.obj" : $(SOURCE) $(DEP_CPP_FILEP) "$(INTDIR)"

"$(INTDIR)\FileParse.sbr" : $(SOURCE) $(DEP_CPP_FILEP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Geometry.cpp
DEP_CPP_GEOME=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\D3DWrap.h"\
	".\DebugPrint.h"\
	".\DirectX.h"\
	".\Geometry.h"\
	".\macros.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\Geometry.obj" : $(SOURCE) $(DEP_CPP_GEOME) "$(INTDIR)"

"$(INTDIR)\Geometry.sbr" : $(SOURCE) $(DEP_CPP_GEOME) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\GrdLand.cpp
DEP_CPP_GRDLA=\
	".\ChnkIO.h"\
	".\DebugPrint.h"\
	".\GrdLand.h"\
	".\typedefs.h"\
	

"$(INTDIR)\GrdLand.obj" : $(SOURCE) $(DEP_CPP_GRDLA) "$(INTDIR)"

"$(INTDIR)\GrdLand.sbr" : $(SOURCE) $(DEP_CPP_GRDLA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HeightMap.cpp

!IF  "$(CFG)" == "EditWorld - Win32 Release"

DEP_CPP_HEIGH=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\GateInterface.h"\
	".\Geometry.h"\
	".\HeightMap.h"\
	".\macros.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\HeightMap.obj" : $(SOURCE) $(DEP_CPP_HEIGH) "$(INTDIR)"

"$(INTDIR)\HeightMap.sbr" : $(SOURCE) $(DEP_CPP_HEIGH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

DEP_CPP_HEIGH=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\GateInterface.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\HeightMap.obj" : $(SOURCE) $(DEP_CPP_HEIGH) "$(INTDIR)"

"$(INTDIR)\HeightMap.sbr" : $(SOURCE) $(DEP_CPP_HEIGH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\KeyHandler.cpp
DEP_CPP_KEYHA=\
	".\DebugPrint.h"\
	".\KeyHandler.h"\
	".\typedefs.h"\
	

"$(INTDIR)\KeyHandler.obj" : $(SOURCE) $(DEP_CPP_KEYHA) "$(INTDIR)"

"$(INTDIR)\KeyHandler.sbr" : $(SOURCE) $(DEP_CPP_KEYHA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "EditWorld - Win32 Release"

DEP_CPP_MAINF=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\DDImage.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\Geometry.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TDView.h"\
	".\TextureView.h"\
	".\typedefs.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

DEP_CPP_MAINF=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TDView.h"\
	".\TextSel.h"\
	".\TextureView.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MapPrefs.cpp
DEP_CPP_MAPPR=\
	".\BTEdit.h"\
	".\MapPrefs.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\MapPrefs.obj" : $(SOURCE) $(DEP_CPP_MAPPR) "$(INTDIR)"

"$(INTDIR)\MapPrefs.sbr" : $(SOURCE) $(DEP_CPP_MAPPR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PCXHandler.cpp
DEP_CPP_PCXHA=\
	".\DebugPrint.h"\
	".\PCXHandler.h"\
	".\typedefs.h"\
	

"$(INTDIR)\PCXHandler.obj" : $(SOURCE) $(DEP_CPP_PCXHA) "$(INTDIR)"

"$(INTDIR)\PCXHandler.sbr" : $(SOURCE) $(DEP_CPP_PCXHA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SaveSegmentDialog.cpp
DEP_CPP_SAVES=\
	".\BTEdit.h"\
	".\SaveSegmentDialog.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\SaveSegmentDialog.obj" : $(SOURCE) $(DEP_CPP_SAVES) "$(INTDIR)"

"$(INTDIR)\SaveSegmentDialog.sbr" : $(SOURCE) $(DEP_CPP_SAVES) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TDView.cpp
DEP_CPP_TDVIE=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TDView.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\TDView.obj" : $(SOURCE) $(DEP_CPP_TDVIE) "$(INTDIR)"

"$(INTDIR)\TDView.sbr" : $(SOURCE) $(DEP_CPP_TDVIE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TextSel.cpp
DEP_CPP_TEXTS=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\TextSel.obj" : $(SOURCE) $(DEP_CPP_TEXTS) "$(INTDIR)"

"$(INTDIR)\TextSel.sbr" : $(SOURCE) $(DEP_CPP_TEXTS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TexturePrefs.cpp
DEP_CPP_TEXTU=\
	".\BTEdit.h"\
	".\StdAfx.h"\
	".\TexturePrefs.h"\
	".\typedefs.h"\
	

"$(INTDIR)\TexturePrefs.obj" : $(SOURCE) $(DEP_CPP_TEXTU) "$(INTDIR)"

"$(INTDIR)\TexturePrefs.sbr" : $(SOURCE) $(DEP_CPP_TEXTU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TextureView.cpp
DEP_CPP_TEXTUR=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\AutoFlagDialog.h"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TextureView.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\TextureView.obj" : $(SOURCE) $(DEP_CPP_TEXTUR) "$(INTDIR)"

"$(INTDIR)\TextureView.sbr" : $(SOURCE) $(DEP_CPP_TEXTUR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TileTypes.cpp
DEP_CPP_TILET=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\TileTypes.obj" : $(SOURCE) $(DEP_CPP_TILET) "$(INTDIR)"

"$(INTDIR)\TileTypes.sbr" : $(SOURCE) $(DEP_CPP_TILET) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\bmpHandler.cpp
DEP_CPP_BMPHA=\
	".\bmpHandler.h"\
	".\DebugPrint.h"\
	".\WinStuff.h"\
	

"$(INTDIR)\bmpHandler.obj" : $(SOURCE) $(DEP_CPP_BMPHA) "$(INTDIR)"

"$(INTDIR)\bmpHandler.sbr" : $(SOURCE) $(DEP_CPP_BMPHA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\EditWorld.rc
DEP_RSC_EDITW=\
	".\res\arrow.cur"\
	".\res\arrowcop.cur"\
	".\res\bitmap1.bmp"\
	".\res\bitmap2.bmp"\
	".\res\bmp00001.bmp"\
	".\res\bmp00002.bmp"\
	".\res\BTEdit.ico"\
	".\res\BTEdit.rc2"\
	".\res\BTEditDoc.ico"\
	".\res\cur00001.cur"\
	".\res\cursor1.cur"\
	".\res\height_d.cur"\
	".\res\ico00001.ico"\
	".\res\ico00002.ico"\
	".\res\ico00003.ico"\
	".\res\ico00004.ico"\
	".\res\icon1.ico"\
	".\res\nodrop.cur"\
	".\res\pointer_.cur"\
	".\res\selectre.cur"\
	".\res\smallbru.ico"\
	".\res\Toolbar.bmp"\
	".\res\toolbar1.bmp"\
	

"$(INTDIR)\EditWorld.res" : $(SOURCE) $(DEP_RSC_EDITW) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "EditWorld - Win32 Release"

!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ObjectProperties.cpp
DEP_CPP_OBJEC=\
	".\BTEdit.h"\
	".\ObjectProperties.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\ObjectProperties.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"

"$(INTDIR)\ObjectProperties.sbr" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\LimitsDialog.cpp
DEP_CPP_LIMIT=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\LimitsDialog.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\LimitsDialog.obj" : $(SOURCE) $(DEP_CPP_LIMIT) "$(INTDIR)"

"$(INTDIR)\LimitsDialog.sbr" : $(SOURCE) $(DEP_CPP_LIMIT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\InitialLimitsDlg.cpp
DEP_CPP_INITI=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\InitialLimitsDlg.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\InitialLimitsDlg.obj" : $(SOURCE) $(DEP_CPP_INITI) "$(INTDIR)"

"$(INTDIR)\InitialLimitsDlg.sbr" : $(SOURCE) $(DEP_CPP_INITI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ExpandLimitsDlg.cpp
DEP_CPP_EXPAN=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\ExpandLimitsDlg.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\ExpandLimitsDlg.obj" : $(SOURCE) $(DEP_CPP_EXPAN) "$(INTDIR)"

"$(INTDIR)\ExpandLimitsDlg.sbr" : $(SOURCE) $(DEP_CPP_EXPAN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PlayerMap.cpp
DEP_CPP_PLAYE=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\BTEdit.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\PlayerMap.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\PlayerMap.obj" : $(SOURCE) $(DEP_CPP_PLAYE) "$(INTDIR)"

"$(INTDIR)\PlayerMap.sbr" : $(SOURCE) $(DEP_CPP_PLAYE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\AutoFlagDialog.cpp
DEP_CPP_AUTOF=\
	".\AutoFlagDialog.h"\
	".\BTEdit.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\AutoFlagDialog.obj" : $(SOURCE) $(DEP_CPP_AUTOF) "$(INTDIR)"

"$(INTDIR)\AutoFlagDialog.sbr" : $(SOURCE) $(DEP_CPP_AUTOF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\GatewaySup.c
DEP_CPP_GATEW=\
	".\DebugPrint.h"\
	".\GateInterface.h"\
	".\Gateway.h"\
	".\GatewayDef.h"\
	".\typedefs.h"\
	
NODEP_CPP_GATEW=\
	".\Frame.h"\
	".\Map.h"\
	

"$(INTDIR)\GatewaySup.obj" : $(SOURCE) $(DEP_CPP_GATEW) "$(INTDIR)"

"$(INTDIR)\GatewaySup.sbr" : $(SOURCE) $(DEP_CPP_GATEW) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Gateway.c
DEP_CPP_GATEWA=\
	".\DebugPrint.h"\
	".\GateInterface.h"\
	".\Gateway.h"\
	".\GatewayDef.h"\
	".\typedefs.h"\
	
NODEP_CPP_GATEWA=\
	".\Frame.h"\
	".\Map.h"\
	

"$(INTDIR)\Gateway.obj" : $(SOURCE) $(DEP_CPP_GATEWA) "$(INTDIR)"

"$(INTDIR)\Gateway.sbr" : $(SOURCE) $(DEP_CPP_GATEWA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\GateInterface.cpp
DEP_CPP_GATEI=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\BTEditView.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\ExpandLimitsDlg.h"\
	".\FileParse.h"\
	".\GateInterface.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\InitialLimitsDlg.h"\
	".\KeyHandler.h"\
	".\LimitsDialog.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\MainFrm.h"\
	".\MapPrefs.h"\
	".\PCXHandler.h"\
	".\PlayerMap.h"\
	".\SaveSegmentDialog.h"\
	".\SnapPrefs.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TexturePrefs.h"\
	".\TextureView.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\GateInterface.obj" : $(SOURCE) $(DEP_CPP_GATEI) "$(INTDIR)"

"$(INTDIR)\GateInterface.sbr" : $(SOURCE) $(DEP_CPP_GATEI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DebugPrint.c

"$(INTDIR)\DebugPrint.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\DebugPrint.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ExportInfo.cpp
DEP_CPP_EXPOR=\
	".\BTEdit.h"\
	".\ExportInfo.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\ExportInfo.obj" : $(SOURCE) $(DEP_CPP_EXPOR) "$(INTDIR)"

"$(INTDIR)\ExportInfo.sbr" : $(SOURCE) $(DEP_CPP_EXPOR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PastePrefs.cpp
DEP_CPP_PASTE=\
	".\BTEdit.h"\
	".\PastePrefs.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	

"$(INTDIR)\PastePrefs.obj" : $(SOURCE) $(DEP_CPP_PASTE) "$(INTDIR)"

"$(INTDIR)\PastePrefs.sbr" : $(SOURCE) $(DEP_CPP_PASTE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\BrushProp.cpp

!IF  "$(CFG)" == "EditWorld - Win32 Release"

DEP_CPP_BRUSHP=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\Geometry.h"\
	".\HeightMap.h"\
	".\macros.h"\
	".\StdAfx.h"\
	".\typedefs.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BrushProp.obj" : $(SOURCE) $(DEP_CPP_BRUSHP) "$(INTDIR)"

"$(INTDIR)\BrushProp.sbr" : $(SOURCE) $(DEP_CPP_BRUSHP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EditWorld - Win32 Debug"

DEP_CPP_BRUSHP=\
	"..\..\mssdk\include\d3dcaps.h"\
	"..\..\mssdk\include\d3dtypes.h"\
	"..\..\mssdk\include\d3dvec.inl"\
	".\bmpHandler.h"\
	".\Brush.h"\
	".\BrushProp.h"\
	".\BTEdit.h"\
	".\BTEditDoc.h"\
	".\ChnkIO.h"\
	".\DDImage.h"\
	".\DebugPrint.h"\
	".\DevMap.h"\
	".\DIBDraw.h"\
	".\DirectX.h"\
	".\FileParse.h"\
	".\Geometry.h"\
	".\GrdLand.h"\
	".\HeightMap.h"\
	".\KeyHandler.h"\
	".\listtemp.h"\
	".\macros.h"\
	".\PCXHandler.h"\
	".\StdAfx.h"\
	".\TextSel.h"\
	".\TileTypes.h"\
	".\typedefs.h"\
	".\UndoRedo.h"\
	".\WFView.h"\
	{$(INCLUDE)}"\d3d.h"\
	

"$(INTDIR)\BrushProp.obj" : $(SOURCE) $(DEP_CPP_BRUSHP) "$(INTDIR)"

"$(INTDIR)\BrushProp.sbr" : $(SOURCE) $(DEP_CPP_BRUSHP) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
