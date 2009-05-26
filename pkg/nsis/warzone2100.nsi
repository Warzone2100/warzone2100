;  This file is part of Warzone 2100.
;  Copyright (C) 2006-2009  Warzone Resurrection Project
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
  !include "FileFunc.nsh"

;--------------------------------
;General

  ;Name and file
  Name "${PACKAGE_NAME}"
  OutFile "${OUTFILE}"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${PACKAGE_NAME}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\${PACKAGE_NAME}" ""

;--------------------------------
;Versioninfo

VIProductVersion "${VERSIONNUM}"
VIAddVersionKey "CompanyName"		"Warzone Resurrection Project"
VIAddVersionKey "FileDescription"	"${PACKAGE_NAME} Installer"
VIAddVersionKey "FileVersion"		"${PACKAGE_VERSION}"
VIAddVersionKey "InternalName"		"${PACKAGE_NAME}"
VIAddVersionKey "LegalCopyright"	"Copyright © 2006-2009 Warzone Resurrection Project"
VIAddVersionKey "OriginalFilename"	"${PACKAGE}-${PACKAGE_VERSION}.exe"
VIAddVersionKey "ProductName"		"${PACKAGE_NAME}"
VIAddVersionKey "ProductVersion"	"${PACKAGE_VERSION}"

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ICON "${TOP_SRCDIR}\icons\warzone2100.ico"
  !define MUI_UNICON "${TOP_SRCDIR}\icons\warzone2100.uninstall.ico"

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

  LicenseLangString MUILicense ${LANG_ENGLISH} "${TOP_SRCDIR}\COPYING"
  LicenseLangString MUILicense ${LANG_DUTCH} "${TOP_SRCDIR}\COPYING"
  LicenseLangString MUILicense ${LANG_GERMAN} "${TOP_SRCDIR}\COPYING"

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
  File "${TOP_BUILDDIR}\src\${PACKAGE}.exe"

  ; Windows dbghelp library
  File "${EXTDIR}\bin\dbghelp.dll.license.txt"
  File "${EXTDIR}\bin\dbghelp.dll"

  ; Data files
  File "${TOP_BUILDDIR}\data\mp.wz"
  File "${TOP_BUILDDIR}\data\base.wz"

  ; Information/documentation files
  File "/oname=ChangeLog.txt" "${TOP_SRCDIR}\ChangeLog"
  File "/oname=Authors.txt" "${TOP_SRCDIR}\AUTHORS"
  File "/oname=License.txt" "${TOP_SRCDIR}\COPYING"
  File "/oname=Readme.en.txt" "${TOP_SRCDIR}\doc\Readme.en"
  File "/oname=Readme.de.txt" "${TOP_SRCDIR}\doc\Readme.de"
  File "/oname=Readme.en.html" "${TOP_SRCDIR}\doc\Readme.en.xhtml"
  File "/oname=Readme.de.html" "${TOP_SRCDIR}\doc\Readme.de.xhtml"

  ; Music files
  SetOutPath "$INSTDIR\music"
  File "${TOP_SRCDIR}\data\music\menu.ogg"
  File "${TOP_SRCDIR}\data\music\track1.ogg"
  File "${TOP_SRCDIR}\data\music\track2.ogg"
  File "${TOP_SRCDIR}\data\music\music.wpl"

  SetOutPath "$INSTDIR\styles"

  File "/oname=readme.print.css" "${TOP_SRCDIR}\doc\styles\readme.print.css"
  File "/oname=readme.screen.css" "${TOP_SRCDIR}\doc\styles\readme.screen.css"

  SetOutPath "$INSTDIR\fonts"

  File "${EXTDIR}\etc\fonts\fonts.conf"
  File "${EXTDIR}\etc\fonts\DejaVuSans.ttf"
  File "${EXTDIR}\etc\fonts\DejaVuSans-Bold.ttf"

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

  ExecWait '"$INSTDIR\oalinst.exe" --silent'

SectionEnd


SectionGroup /e $(TEXT_SecMods) secMods

Section $(TEXT_SecAivolutionMod) SecAivolutionMod

  SetOutPath "$INSTDIR\mods\global"

  File "${TOP_BUILDDIR}\data\mods\global\aivolution.wz"

  SetOutPath "$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN "Application"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - Aivolution.lnk" "$INSTDIR\${PACKAGE}.exe" "--mod aivolution.wz"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section $(TEXT_SecMusicMod) SecMusicMod

  SetOutPath "$INSTDIR\mods\global\autoload"

  IfFileExists "music_1.0.wz" +6
    NSISdl::download "http://download.gna.org/warzone/releases/mods/community-music_1.0.AUTHORS"          "music_1.0.AUTHORS.txt"
    NSISdl::download "http://download.gna.org/warzone/releases/mods/community-music_1.0.wz"               "music_1.0.wz"
    Pop $R0 ; Get the return value
    StrCmp $R0 "success" +2
      MessageBox MB_OK|MB_ICONSTOP "Download of Music mod failed: $R0"

  SetOutPath "$INSTDIR"

SectionEnd

Section $(TEXT_SecNTWMod) SecNTWMod

  SetOutPath "$INSTDIR\mods\multiplay"

  File "${TOP_BUILDDIR}\data\mods\multiplay\ntw.wz"

  SetOutPath "$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN "Application"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - NTW.lnk" "$INSTDIR\${PACKAGE}.exe" "--mod_mp ntw.wz"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

SectionGroupEnd

Section $(TEXT_SecFMVs) SecFMVs

  IfFileExists "sequences.wz" +5
    NSISdl::download "http://download.gna.org/warzone/videos/sequences.wz"               "sequences.wz"
    Pop $R0 ; Get the return value
    StrCmp $R0 "success" +2
      MessageBox MB_OK|MB_ICONSTOP "Download of videos failed: $R0"

SectionEnd

Section $(TEXT_SecNLS) SecNLS

  SetOutPath "$INSTDIR\locale\cs\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\cs.gmo"

  SetOutPath "$INSTDIR\locale\da\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\da.gmo"

  SetOutPath "$INSTDIR\locale\de\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\de.gmo"

  SetOutPath "$INSTDIR\locale\en_GB\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\en_GB.gmo"

  SetOutPath "$INSTDIR\locale\es\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\es.gmo"

  SetOutPath "$INSTDIR\locale\fi\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\fi.gmo"

  SetOutPath "$INSTDIR\locale\fr\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\fr.gmo"

  SetOutPath "$INSTDIR\locale\fy\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\fy.gmo"

  SetOutPath "$INSTDIR\locale\ga\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\ga.gmo"

  SetOutPath "$INSTDIR\locale\it\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\it.gmo"

  SetOutPath "$INSTDIR\locale\la\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\la.gmo"

  SetOutPath "$INSTDIR\locale\lt\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\lt.gmo"

  SetOutPath "$INSTDIR\locale\nb\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\nb.gmo"

  SetOutPath "$INSTDIR\locale\nl\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\nl.gmo"

  SetOutPath "$INSTDIR\locale\pl\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\pl.gmo"

  SetOutPath "$INSTDIR\locale\pt_BR\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\pt_BR.gmo"

  SetOutPath "$INSTDIR\locale\pt\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\pt.gmo"

  SetOutPath "$INSTDIR\locale\ro\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\ro.gmo"

  SetOutPath "$INSTDIR\locale\ru\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\ru.gmo"

  SetOutPath "$INSTDIR\locale\zh_CN\LC_MESSAGES"
  File "/oname=${PACKAGE}.mo" "${TOP_BUILDDIR}\po\zh_CN.gmo"

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

  LangString TEXT_SecAivolutionMod ${LANG_ENGLISH} "Aivolution"
  LangString DESC_SecAivolutionMod ${LANG_ENGLISH} "Improved artificial intelligence that learns."

  LangString TEXT_SecMusicMod ${LANG_ENGLISH} "Music"
  LangString DESC_SecMusicMod ${LANG_ENGLISH} "Download and install music."

  LangString TEXT_SecFMVs ${LANG_ENGLISH} "Videos"
  LangString DESC_SecFMVs ${LANG_ENGLISH} "Download and install videos (in-game cutscenes)."

  LangString TEXT_SecNLS ${LANG_ENGLISH} "NLS"
  LangString DESC_SecNLS ${LANG_ENGLISH} "Support for languages other than English."

  LangString TEXT_SecNTWMod ${LANG_ENGLISH} "NTW: New Team War mod"
  LangString DESC_SecNTWMod ${LANG_ENGLISH} "NTW: New Team War mod. Modifies most of the weapons and research."



  LangString TEXT_SecBase ${LANG_DUTCH} "Standaard installatie"
  LangString DESC_SecBase ${LANG_DUTCH} "Standaard installatie."

  LangString TEXT_SecOpenAL ${LANG_DUTCH} "OpenAL bibliotheken"
  LangString DESC_SecOpenAL ${LANG_DUTCH} "Vereiste bibliotheken voor OpenAL, een opensource/vrije Audio Bibliotheek."

  LangString TEXT_SecMods ${LANG_DUTCH} "Mods"
  LangString DESC_SecMods ${LANG_DUTCH} "Verschillende mods."

  LangString TEXT_SecAivolutionMod ${LANG_DUTCH} "Aivolution"
  LangString DESC_SecAivolutionMod ${LANG_DUTCH} "Verbeterde kunstmatige intelligentie die leert."

  LangString TEXT_SecMusicMod ${LANG_DUTCH} "Muziek"
  LangString DESC_SecMusicMod ${LANG_DUTCH} "Muziek downloaden en installeren."

  LangString TEXT_SecFMVs ${LANG_DUTCH} "FMV"
  LangString DESC_SecFMVs ${LANG_DUTCH} "FMV downloaden en installeren."

  LangString TEXT_SecNLS ${LANG_DUTCH} "NLS"
  LangString DESC_SecNLS ${LANG_DUTCH} "Ondersteuning voor andere talen dan Engels (Nederlands inbegrepen)."

  LangString TEXT_SecNTWMod ${LANG_DUTCH} "NTW: New Team War mod"
  LangString DESC_SecNTWMod ${LANG_DUTCH} "NTW: New Team War mod. Wijzigd de meeste wapens en onderzoeken."



  LangString TEXT_SecBase ${LANG_GERMAN} "Standardinstallation"
  LangString DESC_SecBase ${LANG_GERMAN} "Standardinstallation."

  LangString TEXT_SecOpenAL ${LANG_GERMAN} "OpenAL Bibliotheken"
  LangString DESC_SecOpenAL ${LANG_GERMAN} "Bibliotheken für OpenAL, ein freies Audio Interface. Implementation von Creative Labs."

  LangString TEXT_SecMods ${LANG_GERMAN} "Mods"
  LangString DESC_SecMods ${LANG_GERMAN} "Verschiedene Mods."

  LangString TEXT_SecAivolutionMod ${LANG_GERMAN} "Aivolution"
  LangString DESC_SecAivolutionMod ${LANG_GERMAN} "Verbesserte künstliche Intelligenz, die dazulernt."

  LangString TEXT_SecMusicMod ${LANG_GERMAN} "Musik"
  LangString DESC_SecMusicMod ${LANG_GERMAN} "Musik herunterladen und installieren."

  LangString TEXT_SecFMVs ${LANG_GERMAN} "FMV"
  LangString DESC_SecFMVs ${LANG_GERMAN} "FMV herunterladen und installieren."

  LangString TEXT_SecNLS ${LANG_GERMAN} "NLS"
  LangString DESC_SecNLS ${LANG_GERMAN} "Unterstützung für Sprachen außer Englisch (Deutsch inbegriffen)."

  LangString TEXT_SecNTWMod ${LANG_GERMAN} "NTW: New Team War mod"
  LangString DESC_SecNTWMod ${LANG_GERMAN} "NTW: New Team War mod. Verändert die meisten Forschungen und Waffen."



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
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAivolutionMod} $(DESC_SecAivolutionMod)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMusicMod} $(DESC_SecMusicMod)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs} $(DESC_SecFMVs)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecNTWMod} $(DESC_SecNTWMod)

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

  Delete "$INSTDIR\base.wz"
  Delete "$INSTDIR\mp.wz"
  Delete "$INSTDIR\sequences.wz"

  Delete "$INSTDIR\stderr.txt"
  Delete "$INSTDIR\stdout.txt"

  Delete "$INSTDIR\Readme.en.txt"
  Delete "$INSTDIR\Readme.de.txt"
  Delete "$INSTDIR\Readme.en.html"
  Delete "$INSTDIR\Readme.de.html"

  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\Authors.txt"
  Delete "$INSTDIR\ChangeLog.txt"

  Delete "$INSTDIR\music\menu.ogg"
  Delete "$INSTDIR\music\track1.ogg"
  Delete "$INSTDIR\music\track2.ogg"
  Delete "$INSTDIR\music\music.wpl"
  RMDir "$INSTDIR\music"

  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\styles\readme.print.css"
  Delete "$INSTDIR\styles\readme.screen.css"
  RMDir "$INSTDIR\styles"

  Delete "$INSTDIR\fonts\fonts.conf"
  Delete "$INSTDIR\fonts\DejaVuSansMono.ttf"
  Delete "$INSTDIR\fonts\DejaVuSansMono-Bold.ttf"
  Delete "$INSTDIR\fonts\DejaVuSans.ttf"
  Delete "$INSTDIR\fonts\DejaVuSans-Bold.ttf"
  RMDir "$INSTDIR\fonts"

  Delete "$INSTDIR\mods\global\autoload\music_1.0.AUTHORS.txt"
  Delete "$INSTDIR\mods\global\autoload\music_1.0.wz"
  RMDir "$INSTDIR\mods\global\autoload"

  Delete "$INSTDIR\mods\global\aivolution.wz"
  RMDir "$INSTDIR\mods\global"

  Delete "$INSTDIR\mods\multiplay\ntw.wz"
  RMDir "$INSTDIR\mods\multiplay"
  RMDir "$INSTDIR\mods"

  Delete "$INSTDIR\locale\cs\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\cs\LC_MESSAGES"
  RMDir "$INSTDIR\locale\cs"

  Delete "$INSTDIR\locale\da\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\da\LC_MESSAGES"
  RMDir "$INSTDIR\locale\da"

  Delete "$INSTDIR\locale\de\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\de\LC_MESSAGES"
  RMDir "$INSTDIR\locale\de"

  Delete "$INSTDIR\locale\en_GB\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\en_GB\LC_MESSAGES"
  RMDir "$INSTDIR\locale\en_GB"

  Delete "$INSTDIR\locale\es\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\es\LC_MESSAGES"
  RMDir "$INSTDIR\locale\es"

  Delete "$INSTDIR\locale\fi\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\fi\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fi"

  Delete "$INSTDIR\locale\fr\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\fr\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fr"

  Delete "$INSTDIR\locale\fy\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\fy\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fy"

  Delete "$INSTDIR\locale\ga\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\ga\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ga"

  Delete "$INSTDIR\locale\it\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\it\LC_MESSAGES"
  RMDir "$INSTDIR\locale\it"

  Delete "$INSTDIR\locale\la\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\la\LC_MESSAGES"
  RMDir "$INSTDIR\locale\la"

  Delete "$INSTDIR\locale\lt\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\lt\LC_MESSAGES"
  RMDir "$INSTDIR\locale\lt"

  Delete "$INSTDIR\locale\nb\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\nb\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nb"

  Delete "$INSTDIR\locale\nl\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\nl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nl"

  Delete "$INSTDIR\locale\pl\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\pl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pl"

  Delete "$INSTDIR\locale\pt_BR\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\pt_BR\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pt_BR"

  Delete "$INSTDIR\locale\pt\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\pt\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pt"

  Delete "$INSTDIR\locale\ro\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\ro\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ro"

  Delete "$INSTDIR\locale\ru\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\ru\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ru"

  Delete "$INSTDIR\locale\zh_CN\LC_MESSAGES\${PACKAGE}.mo"
  RMDir "$INSTDIR\locale\zh_CN\LC_MESSAGES"
  RMDir "$INSTDIR\locale\zh_CN"

  RMDir "$INSTDIR\locale"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME}.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - Aivolution.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PACKAGE_NAME} - NTW.lnk"
  RMDir "$SMPROGRAMS\$STARTMENU_FOLDER"

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
