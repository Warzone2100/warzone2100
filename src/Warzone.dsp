# Microsoft Developer Studio Project File - Name="Warzone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Warzone - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Warzone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Warzone.mak" CFG="Warzone - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Warzone - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Warzone - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Warzone - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Warzone - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\Lib\Framework" /I "..\Lib\GameLib" /I "..\Lib\Ivis02" /I "..\Lib\NetPlay" /I "..\Lib\Script" /I "..\Lib\Sequence" /I "..\Lib\Sound" /I "..\Lib\Widget" /I "c:\glide\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "YY_STATIC" /D "__MSC__" /D "NOREGCHECK" /FR /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ddraw.lib dsound.lib dplayx.lib dInput.lib dxguid.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "Warzone - Win32 Release"
# Name "Warzone - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Action.c
# End Source File
# Begin Source File

SOURCE=.\AdvVis.c
# End Source File
# Begin Source File

SOURCE=.\Ai.c
# End Source File
# Begin Source File

SOURCE=.\Ani.c
# End Source File
# Begin Source File

SOURCE=.\Arrow.c
# End Source File
# Begin Source File

SOURCE=.\AStar.c
# End Source File
# Begin Source File

SOURCE=.\Atmos.c
# End Source File
# Begin Source File

SOURCE=.\Aud.c
# End Source File
# Begin Source File

SOURCE=.\Audio_id.c
# End Source File
# Begin Source File

SOURCE=.\Bridge.c
# End Source File
# Begin Source File

SOURCE=.\Bucket3d.c
# End Source File
# Begin Source File

SOURCE=.\BuildPos.c
# End Source File
# Begin Source File

SOURCE=.\Cdaudio.c
# End Source File
# Begin Source File

SOURCE=.\CDSpan.c
# End Source File
# Begin Source File

SOURCE=.\Cheat.c
# End Source File
# Begin Source File

SOURCE=.\clParse.c
# End Source File
# Begin Source File

SOURCE=.\Cluster.c
# End Source File
# Begin Source File

SOURCE=.\CmdDroid.c
# End Source File
# Begin Source File

SOURCE=.\Combat.c
# End Source File
# Begin Source File

SOURCE=.\Component.c
# End Source File
# Begin Source File

SOURCE=.\Config.c
# End Source File
# Begin Source File

SOURCE=.\Console.c
# End Source File
# Begin Source File

SOURCE=.\Csnap.c
# End Source File
# Begin Source File

SOURCE=.\Data.c
# End Source File
# Begin Source File

SOURCE=.\Deliv.rc
# End Source File
# Begin Source File

SOURCE=.\Design.c
# End Source File
# Begin Source File

SOURCE=.\Difficulty.c
# End Source File
# Begin Source File

SOURCE=.\Disp2D.c
# End Source File
# Begin Source File

SOURCE=.\Display.c
# End Source File
# Begin Source File

SOURCE=.\display3d.c
# End Source File
# Begin Source File

SOURCE=.\Drive.c
# End Source File
# Begin Source File

SOURCE=.\Droid.c
# End Source File
# Begin Source File

SOURCE=.\E3Demo.c
# End Source File
# Begin Source File

SOURCE=.\Edit2D.c
# End Source File
# Begin Source File

SOURCE=.\Edit3D.c
# End Source File
# Begin Source File

SOURCE=.\Effects.c
# End Source File
# Begin Source File

SOURCE=.\Environ.c
# End Source File
# Begin Source File

SOURCE=.\Feature.c
# End Source File
# Begin Source File

SOURCE=.\Findpath.c
# End Source File
# Begin Source File

SOURCE=.\Formation.c
# End Source File
# Begin Source File

SOURCE=.\FPath.c
# End Source File
# Begin Source File

SOURCE=.\FrontEnd.c
# End Source File
# Begin Source File

SOURCE=.\Function.c
# End Source File
# Begin Source File

SOURCE=.\Game.c
# End Source File
# Begin Source File

SOURCE=.\Gateway.c
# End Source File
# Begin Source File

SOURCE=.\GatewayRoute.c
# End Source File
# Begin Source File

SOURCE=.\GatewaySup.c
# End Source File
# Begin Source File

SOURCE=.\Geometry.c
# End Source File
# Begin Source File

SOURCE=.\Group.c
# End Source File
# Begin Source File

SOURCE=.\Hci.c
# End Source File
# Begin Source File

SOURCE=.\inGameOp.c
# End Source File
# Begin Source File

SOURCE=.\Init.c
# End Source File
# Begin Source File

SOURCE=.\IntDisplay.c
# End Source File
# Begin Source File

SOURCE=.\IntelMap.c
# End Source File
# Begin Source File

SOURCE=.\IntImage.c
# End Source File
# Begin Source File

SOURCE=.\IntOrder.c
# End Source File
# Begin Source File

SOURCE=.\KeyBind.c
# End Source File
# Begin Source File

SOURCE=.\Keyedit.c
# End Source File
# Begin Source File

SOURCE=.\KeyMap.c
# End Source File
# Begin Source File

SOURCE=.\Level_l.c
# End Source File
# Begin Source File

SOURCE=.\Levels.c
# End Source File
# Begin Source File

SOURCE=.\Lighting.c
# End Source File
# Begin Source File

SOURCE=.\Loadsave.c
# End Source File
# Begin Source File

SOURCE=.\Loop.c
# End Source File
# Begin Source File

SOURCE=.\Map.c
# End Source File
# Begin Source File

SOURCE=.\MapDisplay.c
# End Source File
# Begin Source File

SOURCE=.\MapGrid.c
# End Source File
# Begin Source File

SOURCE=.\Mechanics.c
# End Source File
# Begin Source File

SOURCE=.\Message.c
# End Source File
# Begin Source File

SOURCE=.\MiscImd.c
# End Source File
# Begin Source File

SOURCE=.\Mission.c
# End Source File
# Begin Source File

SOURCE=.\Move.c
# End Source File
# Begin Source File

SOURCE=.\Mpdpxtra.c
# End Source File
# Begin Source File

SOURCE=.\Mplayer.c
# End Source File
# Begin Source File

SOURCE=.\Multibot.c
# End Source File
# Begin Source File

SOURCE=.\multigifts.c
# End Source File
# Begin Source File

SOURCE=.\MultiInt.c
# End Source File
# Begin Source File

SOURCE=.\multijoin.c
# End Source File
# Begin Source File

SOURCE=.\multilimit.c
# End Source File
# Begin Source File

SOURCE=.\MultiMenu.c
# End Source File
# Begin Source File

SOURCE=.\MultiOpt.c
# End Source File
# Begin Source File

SOURCE=.\multiplay.c
# End Source File
# Begin Source File

SOURCE=.\Multistat.c
# End Source File
# Begin Source File

SOURCE=.\multistruct.c
# End Source File
# Begin Source File

SOURCE=.\MultiSync.c
# End Source File
# Begin Source File

SOURCE=.\Objects.c
# End Source File
# Begin Source File

SOURCE=.\ObjMem.c
# End Source File
# Begin Source File

SOURCE=.\oPrint.c
# End Source File
# Begin Source File

SOURCE=.\OptimisePath.c
# End Source File
# Begin Source File

SOURCE=.\Order.c
# End Source File
# Begin Source File

SOURCE=.\Player.c
# End Source File
# Begin Source File

SOURCE=.\Power.c
# End Source File
# Begin Source File

SOURCE=.\PowerCrypt.c
# End Source File
# Begin Source File

SOURCE=.\projectile.c
# End Source File
# Begin Source File

SOURCE=.\Radar.c
# End Source File
# Begin Source File

SOURCE=.\RayCast.c
# End Source File
# Begin Source File

SOURCE=.\readfiles.c
# End Source File
# Begin Source File

SOURCE=.\Research.c
# End Source File
# Begin Source File

SOURCE=.\Scores.c
# End Source File
# Begin Source File

SOURCE=.\ScriptAI.c
# End Source File
# Begin Source File

SOURCE=.\ScriptCB.c
# End Source File
# Begin Source File

SOURCE=.\ScriptExtern.c
# End Source File
# Begin Source File

SOURCE=.\ScriptFuncs.c
# End Source File
# Begin Source File

SOURCE=.\ScriptObj.c
# End Source File
# Begin Source File

SOURCE=.\ScriptTabs.c
# End Source File
# Begin Source File

SOURCE=.\ScriptVals.c
# End Source File
# Begin Source File

SOURCE=.\ScriptVals_l.c
# End Source File
# Begin Source File

SOURCE=.\ScriptVals_y.c
# End Source File
# Begin Source File

SOURCE=.\Selection.c
# End Source File
# Begin Source File

SOURCE=.\seqDisp.c
# End Source File
# Begin Source File

SOURCE=.\Stats.c
# End Source File
# Begin Source File

SOURCE=.\structure.c
# End Source File
# Begin Source File

SOURCE=.\Target.c
# End Source File
# Begin Source File

SOURCE=.\Text.c
# End Source File
# Begin Source File

SOURCE=.\Texture.c
# End Source File
# Begin Source File

SOURCE=.\Transporter.c
# End Source File
# Begin Source File

SOURCE=.\Visibility.c
# End Source File
# Begin Source File

SOURCE=.\WarCAM.c
# End Source File
# Begin Source File

SOURCE=.\warzoneConfig.c
# End Source File
# Begin Source File

SOURCE=.\Water.c
# End Source File
# Begin Source File

SOURCE=.\WinMain.c
# End Source File
# Begin Source File

SOURCE=.\Wrappers.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Action.h
# End Source File
# Begin Source File

SOURCE=.\AdvVis.h
# End Source File
# Begin Source File

SOURCE=.\Ai.h
# End Source File
# Begin Source File

SOURCE=.\AIDef.h
# End Source File
# Begin Source File

SOURCE=.\Anim_id.h
# End Source File
# Begin Source File

SOURCE=.\Arrow.h
# End Source File
# Begin Source File

SOURCE=.\AStar.h
# End Source File
# Begin Source File

SOURCE=.\Atmos.h
# End Source File
# Begin Source File

SOURCE=.\Audio_id.h
# End Source File
# Begin Source File

SOURCE=.\Base.h
# End Source File
# Begin Source File

SOURCE=.\Bridge.h
# End Source File
# Begin Source File

SOURCE=.\Bucket3d.h
# End Source File
# Begin Source File

SOURCE=.\BuildPos.h
# End Source File
# Begin Source File

SOURCE=.\Bullet.h
# End Source File
# Begin Source File

SOURCE=.\BulletDef.h
# End Source File
# Begin Source File

SOURCE=.\Cdaudio.h
# End Source File
# Begin Source File

SOURCE=.\CDSpan.h
# End Source File
# Begin Source File

SOURCE=.\Cheat.h
# End Source File
# Begin Source File

SOURCE=.\clParse.h
# End Source File
# Begin Source File

SOURCE=.\Cluster.h
# End Source File
# Begin Source File

SOURCE=.\CmdDroid.h
# End Source File
# Begin Source File

SOURCE=.\CmdDroidDef.h
# End Source File
# Begin Source File

SOURCE=.\Combat.h
# End Source File
# Begin Source File

SOURCE=.\Component.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\Console.h
# End Source File
# Begin Source File

SOURCE=.\Csnap.h
# End Source File
# Begin Source File

SOURCE=.\Data.h
# End Source File
# Begin Source File

SOURCE=.\Deliverance.h
# End Source File
# Begin Source File

SOURCE=.\Design.h
# End Source File
# Begin Source File

SOURCE=.\Difficulty.h
# End Source File
# Begin Source File

SOURCE=.\Disp2D.h
# End Source File
# Begin Source File

SOURCE=.\Display.h
# End Source File
# Begin Source File

SOURCE=.\Display3D.h
# End Source File
# Begin Source File

SOURCE=.\Display3Ddef.h
# End Source File
# Begin Source File

SOURCE=.\DisplayDef.h
# End Source File
# Begin Source File

SOURCE=.\Drive.h
# End Source File
# Begin Source File

SOURCE=.\Droid.h
# End Source File
# Begin Source File

SOURCE=.\DroidDef.h
# End Source File
# Begin Source File

SOURCE=.\E3Demo.h
# End Source File
# Begin Source File

SOURCE=.\Edit2D.h
# End Source File
# Begin Source File

SOURCE=.\Edit3D.h
# End Source File
# Begin Source File

SOURCE=.\Effects.h
# End Source File
# Begin Source File

SOURCE=.\Environ.h
# End Source File
# Begin Source File

SOURCE=.\Feature.h
# End Source File
# Begin Source File

SOURCE=.\FeatureDef.h
# End Source File
# Begin Source File

SOURCE=.\Findpath.h
# End Source File
# Begin Source File

SOURCE=.\Formation.h
# End Source File
# Begin Source File

SOURCE=.\FormationDef.h
# End Source File
# Begin Source File

SOURCE=.\FPath.h
# End Source File
# Begin Source File

SOURCE=.\Frend.h
# End Source File
# Begin Source File

SOURCE=.\FrontEnd.h
# End Source File
# Begin Source File

SOURCE=.\Function.h
# End Source File
# Begin Source File

SOURCE=.\FunctionDef.h
# End Source File
# Begin Source File

SOURCE=.\Game.h
# End Source File
# Begin Source File

SOURCE=.\Gamedefs.h
# End Source File
# Begin Source File

SOURCE=.\Gateway.h
# End Source File
# Begin Source File

SOURCE=.\GatewayDef.h
# End Source File
# Begin Source File

SOURCE=.\GatewayRoute.h
# End Source File
# Begin Source File

SOURCE=.\Geometry.h
# End Source File
# Begin Source File

SOURCE=.\Group.h
# End Source File
# Begin Source File

SOURCE=.\Hci.h
# End Source File
# Begin Source File

SOURCE=.\inGameOp.h
# End Source File
# Begin Source File

SOURCE=.\Init.h
# End Source File
# Begin Source File

SOURCE=.\IntDisplay.h
# End Source File
# Begin Source File

SOURCE=.\IntelMap.h
# End Source File
# Begin Source File

SOURCE=.\Intfac.h
# End Source File
# Begin Source File

SOURCE=.\IntImage.h
# End Source File
# Begin Source File

SOURCE=.\IntOrder.h
# End Source File
# Begin Source File

SOURCE=.\Ivis02.h
# End Source File
# Begin Source File

SOURCE=.\KeyBind.h
# End Source File
# Begin Source File

SOURCE=.\Keyedit.h
# End Source File
# Begin Source File

SOURCE=.\KeyMap.h
# End Source File
# Begin Source File

SOURCE=.\LevelInt.h
# End Source File
# Begin Source File

SOURCE=.\Levels.h
# End Source File
# Begin Source File

SOURCE=.\Lighting.h
# End Source File
# Begin Source File

SOURCE=.\Loadsave.h
# End Source File
# Begin Source File

SOURCE=.\Loop.h
# End Source File
# Begin Source File

SOURCE=.\LOSRoute.h
# End Source File
# Begin Source File

SOURCE=.\Map.h
# End Source File
# Begin Source File

SOURCE=.\MapDisplay.h
# End Source File
# Begin Source File

SOURCE=.\MapGrid.h
# End Source File
# Begin Source File

SOURCE=.\Mechanics.h
# End Source File
# Begin Source File

SOURCE=.\Message.h
# End Source File
# Begin Source File

SOURCE=.\messageDef.h
# End Source File
# Begin Source File

SOURCE=.\MiscImd.h
# End Source File
# Begin Source File

SOURCE=.\Mission.h
# End Source File
# Begin Source File

SOURCE=.\MissionDef.h
# End Source File
# Begin Source File

SOURCE=.\Move.h
# End Source File
# Begin Source File

SOURCE=.\MoveDef.h
# End Source File
# Begin Source File

SOURCE=.\Mpdpxtra.h
# End Source File
# Begin Source File

SOURCE=.\multigifts.h
# End Source File
# Begin Source File

SOURCE=.\Multiint.h
# End Source File
# Begin Source File

SOURCE=.\multijoin.h
# End Source File
# Begin Source File

SOURCE=.\multilimit.h
# End Source File
# Begin Source File

SOURCE=.\MultiMenu.h
# End Source File
# Begin Source File

SOURCE=.\multiplay.h
# End Source File
# Begin Source File

SOURCE=.\multirecv.h
# End Source File
# Begin Source File

SOURCE=.\multistat.h
# End Source File
# Begin Source File

SOURCE=.\ObjectDef.h
# End Source File
# Begin Source File

SOURCE=.\Objects.h
# End Source File
# Begin Source File

SOURCE=.\ObjMem.h
# End Source File
# Begin Source File

SOURCE=.\Oprint.h
# End Source File
# Begin Source File

SOURCE=.\OptimisePath.h
# End Source File
# Begin Source File

SOURCE=.\Order.h
# End Source File
# Begin Source File

SOURCE=.\Orderdef.h
# End Source File
# Begin Source File

SOURCE=.\Player.h
# End Source File
# Begin Source File

SOURCE=.\Power.h
# End Source File
# Begin Source File

SOURCE=.\powercrypt.h
# End Source File
# Begin Source File

SOURCE=.\projectile.h
# End Source File
# Begin Source File

SOURCE=.\Radar.h
# End Source File
# Begin Source File

SOURCE=.\RayCast.h
# End Source File
# Begin Source File

SOURCE=.\Research.h
# End Source File
# Begin Source File

SOURCE=.\ResearchDef.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Scores.h
# End Source File
# Begin Source File

SOURCE=.\ScriptAI.h
# End Source File
# Begin Source File

SOURCE=.\ScriptCB.h
# End Source File
# Begin Source File

SOURCE=.\ScriptExtern.h
# End Source File
# Begin Source File

SOURCE=.\ScriptFuncs.h
# End Source File
# Begin Source File

SOURCE=.\ScriptObj.h
# End Source File
# Begin Source File

SOURCE=.\ScriptTabs.h
# End Source File
# Begin Source File

SOURCE=.\ScriptVals.h
# End Source File
# Begin Source File

SOURCE=.\ScriptVals_y.h
# End Source File
# Begin Source File

SOURCE=.\Selection.h
# End Source File
# Begin Source File

SOURCE=.\seqDisp.h
# End Source File
# Begin Source File

SOURCE=.\Stats.h
# End Source File
# Begin Source File

SOURCE=.\StatsDef.h
# End Source File
# Begin Source File

SOURCE=.\Structure.h
# End Source File
# Begin Source File

SOURCE=.\StructureDef.h
# End Source File
# Begin Source File

SOURCE=.\Target.h
# End Source File
# Begin Source File

SOURCE=.\Text.h
# End Source File
# Begin Source File

SOURCE=.\Texture.h
# End Source File
# Begin Source File

SOURCE=.\Transporter.h
# End Source File
# Begin Source File

SOURCE=.\Visibility.h
# End Source File
# Begin Source File

SOURCE=.\WarCAM.h
# End Source File
# Begin Source File

SOURCE=.\warzoneConfig.h
# End Source File
# Begin Source File

SOURCE=.\Weapons.h
# End Source File
# Begin Source File

SOURCE=.\WinMain.h
# End Source File
# Begin Source File

SOURCE=.\Wrappers.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
