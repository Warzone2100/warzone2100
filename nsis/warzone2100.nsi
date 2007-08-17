;  This file is part of Warzone 2100.
;  Copyright (C) 2006-2007  Warzone Resurrection Project
;  Copyright (C) 2006       Dennis Schridde
;
;  Warzone 2100 is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  Warzone 2100 is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with Warzone 2100; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
;
;  NSIS Modern User Interface
;  Warzone 2100 Resurrection Installer script
;

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Warzone 2100"
  OutFile "warzone2100-${VERSION}.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Warzone 2100"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\Warzone 2100" ""

  SetCompressor /FINAL /SOLID lzma

;--------------------------------
;Versioninfo

VIProductVersion "${VERSIONNUM}"
VIAddVersionKey "CompanyName"		"Warzone Resurrection Project"
VIAddVersionKey "FileDescription"	"Warzone 2100 Installer"
VIAddVersionKey "FileVersion"		"${VERSION}"
VIAddVersionKey "InternalName"		"Warzone 2100"
VIAddVersionKey "LegalCopyright"	"Copyright © 2006 Warzone Resurrection Project"
VIAddVersionKey "OriginalFilename"	"warzone2100-${VERSION}.exe"
VIAddVersionKey "ProductName"		"Warzone 2100"
VIAddVersionKey "ProductVersion"	"${VERSION}"

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

  ; Settings for MUI_PAGE_LICENSE
  !define MUI_LICENSEPAGE_RADIOBUTTONS

  ;Start Menu Folder Page Configuration (for MUI_PAGE_STARTMENU)
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Warzone 2100"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  ; These indented statements modify settings for MUI_PAGE_FINISH
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_RUN_TEXT $(TEXT_RunWarzone)
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Readme.txt"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE $(MUILicense)
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" # first language is the default language
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "German"

;--------------------------------
;License Language String

  LicenseLangString MUILicense ${LANG_ENGLISH} "..\COPYING"
  LicenseLangString MUILicense ${LANG_DUTCH} "..\COPYING"
  LicenseLangString MUILicense ${LANG_GERMAN} "..\COPYING"

;--------------------------------
;Reserve Files

  ;These files should be inserted before other files in the data block
  ;Keep these lines before any File command
  ;Only for solid compression (by default, solid compression is enabled for BZIP2 and LZMA)

  !insertmacro MUI_RESERVEFILE_LANGDLL



;--------------------------------
;Installer Sections

Section $(TEXT_SecBase) SecBase

  SectionIn RO

  SetOutPath "$INSTDIR"

  ;ADD YOUR OWN FILES HERE...

  ; Main executable
  File "..\src\warzone2100.exe"

  ; Windows dbghelp library
  File "${EXTDIR}\dbghelp.dll.license.txt"
  File "${EXTDIR}\dbghelp.dll"

  ; Data files
  File "..\data\mp.wz"
  File "..\data\warzone.wz"

  ; Information/documentation files
  File "/oname=ChangeLog.txt" "..\ChangeLog"
  File "/oname=Authors.txt" "..\AUTHORS"
  File "/oname=License.txt" "..\COPYING"
  File "/oname=Readme.txt" "..\README"


  ;Store installation folder
  WriteRegStr HKLM "Software\Warzone 2100" "" $INSTDIR

  ; Write the Windows-uninstall keys
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "DisplayName" "Warzone 2100"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "DisplayIcon" "$INSTDIR\warzone2100.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "Publisher" "Warzone Resurrection Project"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "URLInfoAbout" "http://wz2100.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100" "NoRepair" 1

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Warzone 2100.lnk" "$INSTDIR\warzone2100.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd


; Installs OpenAL runtime libraries, using Creative's installer
Section $(TEXT_SecOpenAL) SecOpenAL

  File "${EXTDIR}\oalinst.exe"

  ExecWait "$INSTDIR\oalinst.exe"

SectionEnd


SectionGroup /e $(TEXT_SecMods) secMods

Section $(TEXT_SecGrimMod) SecGrimMod

  SetOutPath "$INSTDIR\mods\global"

  File "..\data\mods\global\grim.wz"

  SetOutPath "$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Warzone 2100 - Grim's GFX.lnk" "$INSTDIR\warzone2100.exe" "--mod grim.wz"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

SectionGroupEnd

Section $(TEXT_SecNLS) SecNLS

  SetOutPath "$INSTDIR\${LOCALEDIR}\da\LC_MESSAGES"
  File "..\data\locale\da\LC_MESSAGES\${PACKAGE}.mo"

  SetOutPath "$INSTDIR\${LOCALEDIR}\de\LC_MESSAGES"
  File "..\data\locale\de\LC_MESSAGES\${PACKAGE}.mo"

  SetOutPath "$INSTDIR\${LOCALEDIR}\la\LC_MESSAGES"
  File "..\data\locale\la\LC_MESSAGES\${PACKAGE}.mo"

  SetOutPath "$INSTDIR\${LOCALEDIR}\nb\LC_MESSAGES"
  File "..\data\locale\nb\LC_MESSAGES\${PACKAGE}.mo"

  SetOutPath "$INSTDIR\${LOCALEDIR}\nl\LC_MESSAGES"
  File "..\data\locale\nl\LC_MESSAGES\${PACKAGE}.mo"

SectionEnd



;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

Function LaunchLink
  ExecShell "" "$SMPROGRAMS\$STARTMENU_FOLDER\Warzone 2100.lnk"
FunctionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString TEXT_SecBase ${LANG_ENGLISH} "Standard installation"
  LangString DESC_SecBase ${LANG_ENGLISH} "Standard installation."

  LangString TEXT_SecOpenAL ${LANG_ENGLISH} "OpenAL libraries"
  LangString DESC_SecOpenAL ${LANG_ENGLISH} "Runtime libraries for OpenAL, a free Audio interface. Implementation by Creative Labs."

  LangString TEXT_SecMods ${LANG_ENGLISH} "Mods"
  LangString DESC_SecMods ${LANG_ENGLISH} "Various mods."

  LangString TEXT_SecGrimMod ${LANG_ENGLISH} "Grim's graphics-update"
  LangString DESC_SecGrimMod ${LANG_ENGLISH} "Grim's graphics-update. Contains more detailed textures for campaign 1 as well as additional texture- and model-updates. Copyright (C) 2005-2007 Grim Moroe, Use is only permited for Warzone 2100."

  LangString TEXT_SecNLS ${LANG_ENGLISH} "NLS"
  LangString DESC_SecNLS ${LANG_ENGLISH} "Support for languages other than English."



  LangString TEXT_SecBase ${LANG_DUTCH} "Standaard installatie"
  LangString DESC_SecBase ${LANG_DUTCH} "Standaard installatie."

  LangString TEXT_SecOpenAL ${LANG_DUTCH} "OpenAL bibliotheken"
  LangString DESC_SecOpenAL ${LANG_DUTCH} "Vereiste bibliotheken voor OpenAL, een opensource/vrije Audio Bibliotheek."

  LangString TEXT_SecMods ${LANG_DUTCH} "Mods"
  LangString DESC_SecMods ${LANG_DUTCH} "Verschillende mods."

  LangString TEXT_SecGrimMod ${LANG_DUTCH} "Grim's grafische-update"
  LangString DESC_SecGrimMod ${LANG_DUTCH} "Grim's grafische-update. Bevat meer gedetaïleerde textures voor campaign 1 en extra texture- en model-updates. Copyright (C) 2005-2007 Grim Moroe, gebruik is alleen toegestaan voor Warzone 2100."

  LangString TEXT_SecNLS ${LANG_DUTCH} "NLS"
  LangString DESC_SecNLS ${LANG_DUTCH} "Ondersteuning voor andere talen dan Engels (Nederlands inbegrepen)."



  LangString TEXT_SecBase ${LANG_GERMAN} "Standardinstallation"
  LangString DESC_SecBase ${LANG_GERMAN} "Standardinstallation."

  LangString TEXT_SecOpenAL ${LANG_GERMAN} "OpenAL Bibliotheken"
  LangString DESC_SecOpenAL ${LANG_GERMAN} "Bibliotheken für OpenAL, ein freies Audio Interface. Implementation von Creative Labs."

  LangString TEXT_SecMods ${LANG_GERMAN} "Mods"
  LangString DESC_SecMods ${LANG_GERMAN} "Verschiedene Mods."

  LangString TEXT_SecGrimMod ${LANG_GERMAN} "Grims Grafik-Update"
  LangString DESC_SecGrimMod ${LANG_GERMAN} "Grims Grafik-Update. Enthält detailliertere Texturen für Kampagne 1 sowie einige andere Textur- und Model-Updates. Copyright (C) 2005-2007 Grim Moroe, Verwendung nur für Warzone 2100 gestattet."

  LangString TEXT_SecNLS ${LANG_GERMAN} "NLS"
  LangString DESC_SecNLS ${LANG_GERMAN} "Unterstützung für Sprachen anders als Englisch (Deutsch inbegriffen)."



  LangString TEXT_RunWarzone ${LANG_ENGLISH} "Run Warzone 2100"
  LangString TEXT_RunWarzone ${LANG_DUTCH} "Start Warzone 2100"
  LangString TEXT_RunWarzone ${LANG_GERMAN} "Starte Warzone 2100"



  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBase} $(DESC_SecBase)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecOpenAL} $(DESC_SecOpenAL)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecMods} $(DESC_SecMods)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGrimMod} $(DESC_SecGrimMod)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecNLS} $(DESC_SecNLS)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END



;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\warzone2100.exe"

  Delete "$INSTDIR\oalinst.exe"

  Delete "$INSTDIR\dbghelp.dll.license.txt"
  Delete "$INSTDIR\dbghelp.dll"

  Delete "$INSTDIR\warzone.wz"
  Delete "$INSTDIR\mp.wz"

  Delete "$INSTDIR\Readme.txt"
  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\Authors.txt"
  Delete "$INSTDIR\ChangeLog.txt"

  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\mods\global\grim.wz"

  RMDir "$INSTDIR\mods\global"
  RMDir "$INSTDIR\mods"
  RMDir "$INSTDIR"

  Delete "$INSTDIR\${LOCALEDIR}\da\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\${LOCALEDIR}\da\LC_MESSAGES"
  RMDir "$INSTDIR\${LOCALEDIR}\da"

  Delete "$INSTDIR\${LOCALEDIR}\de\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\${LOCALEDIR}\de\LC_MESSAGES"
  RMDir "$INSTDIR\${LOCALEDIR}\de"

  Delete "$INSTDIR\${LOCALEDIR}\la\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\${LOCALEDIR}\la\LC_MESSAGES"
  RMDir "$INSTDIR\${LOCALEDIR}\la"

  Delete "$INSTDIR\${LOCALEDIR}\nb\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\${LOCALEDIR}\nb\LC_MESSAGES"
  RMDir "$INSTDIR\${LOCALEDIR}\nb"

  Delete "$INSTDIR\${LOCALEDIR}\nl\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\${LOCALEDIR}\nl\LC_MESSAGES"
  RMDir "$INSTDIR\${LOCALEDIR}\nl"

  RMDir "$INSTDIR\${LOCALEDIR}"

  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Warzone 2100.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Warzone 2100 - Grim's GFX.lnk"

  ;Delete empty start menu parent diretories
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

  startMenuDeleteLoop:
	ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

    IfErrors startMenuDeleteLoopDone

    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegValue HKLM "Software\Warzone 2100" "Start Menu Folder"
  DeleteRegValue HKLM "Software\Warzone 2100" ""
  DeleteRegKey /ifempty HKLM "Software\Warzone 2100"

  ; Unregister with Windows' uninstall system
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Warzone 2100"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd
