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
  Name "${PACKAGE_NAME}"
  OutFile "${PACKAGE}-${PACKAGE_VERSION}.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${PACKAGE_NAME}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\${PACKAGE_NAME}" ""

  SetCompressor /FINAL /SOLID lzma

;--------------------------------
;Versioninfo

VIProductVersion "${VERSIONNUM}"
VIAddVersionKey "CompanyName"		"Warzone Resurrection Project"
VIAddVersionKey "FileDescription"	"${PACKAGE_NAME} Installer"
VIAddVersionKey "FileVersion"		"${PACKAGE_VERSION}"
VIAddVersionKey "InternalName"		"${PACKAGE_NAME}"
VIAddVersionKey "LegalCopyright"	"Copyright © 2006 Warzone Resurrection Project"
VIAddVersionKey "OriginalFilename"	"${PACKAGE}-${PACKAGE_VERSION}.exe"
VIAddVersionKey "ProductName"		"${PACKAGE_NAME}"
VIAddVersionKey "ProductVersion"	"${PACKAGE_VERSION}"

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ICON "..\..\icons\warzone2100.ico"
  !define MUI_UNICON "..\..\icons\warzone2100.uninstall.ico"

  !define MUI_ABORTWARNING

  ; Settings for MUI_PAGE_LICENSE
  ; Purposefully commented out, as we do _not_ want to trouble users with an
  ; additional mouse click (while otherwise pressing "return" continuously
  ; would satisfy)
;  !define MUI_LICENSEPAGE_RADIOBUTTONS

  ;Start Menu Folder Page Configuration (for MUI_PAGE_STARTMENU)
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PACKAGE_NAME}"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  ; These indented statements modify settings for MUI_PAGE_FINISH
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_RUN_TEXT $(TEXT_RunWarzone)
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME $(TEXT_Readme)

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE $(MUILicense)
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_STARTMENU "Application" $STARTMENU_FOLDER
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

  LicenseLangString MUILicense ${LANG_ENGLISH} "..\..\COPYING"
  LicenseLangString MUILicense ${LANG_DUTCH} "..\..\COPYING"
  LicenseLangString MUILicense ${LANG_GERMAN} "..\..\COPYING"

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
  File "..\..\src\${PACKAGE}.exe"

  ; Windows dbghelp library
  File "${EXTDIR}\bin\dbghelp.dll.license.txt"
  File "${EXTDIR}\bin\dbghelp.dll"

  ; Data files
  File "..\..\data\mp.wz"
  File "..\..\data\warzone.wz"

  ; Information/documentation files
  File "/oname=ChangeLog.txt" "..\..\ChangeLog"
  File "/oname=Authors.txt" "..\..\AUTHORS"
  File "/oname=License.txt" "..\..\COPYING"
  File "/oname=Readme.en.txt" "..\..\doc\Readme.en"
  File "/oname=Readme.de.txt" "..\..\doc\Readme.de"
;  File "/oname=Readme.nl.txt" "..\..\doc\Readme.nl"
  File "/oname=Readme.en.html" "..\..\doc\Readme.en.xhtml"
  File "/oname=Readme.de.html" "..\..\doc\Readme.de.xhtml"
;  File "/oname=Readme.nl.html" "..\..\doc\Readme.nl.xhtml"

  SetOutPath "$INSTDIR\styles"

  File "/oname=readme.print.css" "..\..\doc\styles\readme.print.css"
  File "/oname=readme.screen.css" "..\..\doc\styles\readme.screen.css"

  SetOutPath "$INSTDIR\fonts"

  File "${EXTDIR}\etc\fonts\fonts.conf"
  File "${EXTDIR}\etc\fonts\DejaVuSansMono.ttf"

  ;Store installation folder
  WriteRegStr HKLM "Software\${PACKAGE_NAME}" "" $INSTDIR

  ; Write the Windows-uninstall keys
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayName" "${PACKAGE_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayVersion" "${PACKAGE_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayIcon" "$INSTDIR\${PACKAGE}.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "Publisher" "Warzone Resurrection Project"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "URLInfoAbout" "${PACKAGE_BUGREPORT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoRepair" 1

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME}.lnk" "$INSTDIR\${PACKAGE}.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd


; Installs OpenAL runtime libraries, using Creative's installer
Section $(TEXT_SecOpenAL) SecOpenAL

  SetOutPath "$INSTDIR"

  File "${EXTDIR}\bin\oalinst.exe"

  ExecWait "$INSTDIR\oalinst.exe"

SectionEnd


SectionGroup /e $(TEXT_SecMods) secMods

Section $(TEXT_SecGrimMod) SecGrimMod

  SetOutPath "$INSTDIR\mods\global"

  File "..\..\data\mods\global\grim.wz"

  SetOutPath "$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - Grim's GFX.lnk" "$INSTDIR\${PACKAGE}.exe" "--mod grim.wz"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section $(TEXT_SecAIvolutionMod) SecAIvolutionMod

  SetOutPath "$INSTDIR\mods\global"

  File "..\..\data\mods\global\aivolution.wz"

  SetOutPath "$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN "Application"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - AIvolution.lnk" "$INSTDIR\${PACKAGE}.exe" "--mod aivolution.wz"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

SectionGroupEnd

Section $(TEXT_SecNLS) SecNLS

  SetOutPath "$INSTDIR\locale\da\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "..\..\po\da.gmo"

  SetOutPath "$INSTDIR\locale\de\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "..\..\po\de.gmo"

  SetOutPath "$INSTDIR\locale\la\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "..\..\po\la.gmo"

  SetOutPath "$INSTDIR\locale\nb\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "..\..\po\nb.gmo"

  SetOutPath "$INSTDIR\locale\nl\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "..\..\po\nl.gmo"

SectionEnd



;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

Function LaunchLink
  ExecShell "" "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME}.lnk"
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
  LangString DESC_SecGrimMod ${LANG_ENGLISH} "Grim's graphics-update. Contains more detailed textures for campaign 1 as well as additional texture- and model-updates. Copyright (C) 2005-2007 Grim Moroe, Use is only permited for ${PACKAGE_NAME}."

  LangString TEXT_SecAIvolutionMod ${LANG_ENGLISH} "AIvolution"
  LangString DESC_SecAIvolutionMod ${LANG_ENGLISH} "Improved artificial intelligence that learns."

  LangString TEXT_SecNLS ${LANG_ENGLISH} "NLS"
  LangString DESC_SecNLS ${LANG_ENGLISH} "Support for languages other than English."



  LangString TEXT_SecBase ${LANG_DUTCH} "Standaard installatie"
  LangString DESC_SecBase ${LANG_DUTCH} "Standaard installatie."

  LangString TEXT_SecOpenAL ${LANG_DUTCH} "OpenAL bibliotheken"
  LangString DESC_SecOpenAL ${LANG_DUTCH} "Vereiste bibliotheken voor OpenAL, een opensource/vrije Audio Bibliotheek."

  LangString TEXT_SecMods ${LANG_DUTCH} "Mods"
  LangString DESC_SecMods ${LANG_DUTCH} "Verschillende mods."

  LangString TEXT_SecGrimMod ${LANG_DUTCH} "Grim's grafische-update"
  LangString DESC_SecGrimMod ${LANG_DUTCH} "Grim's grafische-update. Bevat meer gedetaïleerde textures voor campaign 1 en extra texture- en model-updates. Copyright (C) 2005-2007 Grim Moroe, gebruik is alleen toegestaan voor ${PACKAGE_NAME}."

  LangString TEXT_SecAIvolutionMod ${LANG_DUTCH} "AIvolution"
  LangString DESC_SecAIvolutionMod ${LANG_DUTCH} "Verbeterde kunstmatige intelligentie die leert."

  LangString TEXT_SecNLS ${LANG_DUTCH} "NLS"
  LangString DESC_SecNLS ${LANG_DUTCH} "Ondersteuning voor andere talen dan Engels (Nederlands inbegrepen)."



  LangString TEXT_SecBase ${LANG_GERMAN} "Standardinstallation"
  LangString DESC_SecBase ${LANG_GERMAN} "Standardinstallation."

  LangString TEXT_SecOpenAL ${LANG_GERMAN} "OpenAL Bibliotheken"
  LangString DESC_SecOpenAL ${LANG_GERMAN} "Bibliotheken für OpenAL, ein freies Audio Interface. Implementation von Creative Labs."

  LangString TEXT_SecMods ${LANG_GERMAN} "Mods"
  LangString DESC_SecMods ${LANG_GERMAN} "Verschiedene Mods."

  LangString TEXT_SecGrimMod ${LANG_GERMAN} "Grims Grafik-Update"
  LangString DESC_SecGrimMod ${LANG_GERMAN} "Grims Grafik-Update. Enthält detailliertere Texturen für Kampagne 1 sowie einige andere Textur- und Model-Updates. Copyright (C) 2005-2007 Grim Moroe, Verwendung nur für ${PACKAGE_NAME} gestattet."

  LangString TEXT_SecAIvolutionMod ${LANG_GERMAN} "AIvolutino"
  LangString DESC_SecAIvolutionMod ${LANG_GERMAN} "Verbesserte künstliche Intelligenz die erlernt."

  LangString TEXT_SecNLS ${LANG_GERMAN} "NLS"
  LangString DESC_SecNLS ${LANG_GERMAN} "Unterstützung für Sprachen anders als Englisch (Deutsch inbegriffen)."



  LangString TEXT_RunWarzone ${LANG_ENGLISH} "Run ${PACKAGE_NAME}"
  LangString TEXT_RunWarzone ${LANG_DUTCH} "Start ${PACKAGE_NAME}"
  LangString TEXT_RunWarzone ${LANG_GERMAN} "Starte ${PACKAGE_NAME}"


  LangString TEXT_Readme ${LANG_ENGLISH} "$INSTDIR\Readme.en.html"
  LangString TEXT_Readme ${LANG_DUTCH}   "$INSTDIR\Readme.en.html"
  LangString TEXT_Readme ${LANG_GERMAN}  "$INSTDIR\Readme.de.html"


  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBase} $(DESC_SecBase)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecOpenAL} $(DESC_SecOpenAL)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecMods} $(DESC_SecMods)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGrimMod} $(DESC_SecGrimMod)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAIvolutionMod} $(DESC_SecAIvolutionMod)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecNLS} $(DESC_SecNLS)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END



;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\${PACKAGE}.exe"

  Delete "$INSTDIR\oalinst.exe"

  Delete "$INSTDIR\dbghelp.dll.license.txt"
  Delete "$INSTDIR\dbghelp.dll"

  Delete "$INSTDIR\warzone.wz"
  Delete "$INSTDIR\mp.wz"

  Delete "$INSTDIR\Readme.en.txt"
  Delete "$INSTDIR\Readme.de.txt"
  Delete "$INSTDIR\Readme.nl.txt"
  Delete "$INSTDIR\Readme.en.html"
  Delete "$INSTDIR\Readme.de.html"
  Delete "$INSTDIR\Readme.nl.html"

  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\Authors.txt"
  Delete "$INSTDIR\ChangeLog.txt"

  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\styles\readme.print.css"
  Delete "$INSTDIR\styles\readme.screen.css"

  Delete "$INSTDIR\fonts\fonts.conf"
  Delete "$INSTDIR\fonts\DejaVuSansMono.ttf"

  Delete "$INSTDIR\mods\global\grim.wz"

  Delete "$INSTDIR\locale\da\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\da\LC_MESSAGES"
  RMDir "$INSTDIR\locale\da"

  Delete "$INSTDIR\locale\de\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\de\LC_MESSAGES"
  RMDir "$INSTDIR\locale\de"

  Delete "$INSTDIR\locale\la\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\la\LC_MESSAGES"
  RMDir "$INSTDIR\locale\la"

  Delete "$INSTDIR\locale\nb\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\nb\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nb"

  Delete "$INSTDIR\locale\nl\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\nl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nl"

  RMDir "$INSTDIR\locale"

  RMDir "$INSTDIR\mods\global"
  RMDir "$INSTDIR\mods"
  RMDir "$INSTDIR\styles"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME}.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - Grim's GFX.lnk"

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

  DeleteRegValue HKLM "Software\${PACKAGE_NAME}" "Start Menu Folder"
  DeleteRegValue HKLM "Software\${PACKAGE_NAME}" ""
  DeleteRegKey /ifempty HKLM "Software\${PACKAGE_NAME}"

  ; Unregister with Windows' uninstall system
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd
