;  This file is part of Warzone 2100.
;  Copyright (C) 2007-2018 Warzone 2100 Project
;  Copyright (C) 2007      Dennis Schridde
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
;  Warzone 2100 Installer script
;

;--------------------------------
;Include section

  !include "MUI.nsh"
  !include "FileFunc.nsh"
  !include "LogicLib.nsh"

  ;Include VPatch
  !include "VPatchLib.nsh"

;--------------------------------
;General
  CRCCheck on  ;make sure this isn't corrupted

  ;Name and file
  Name "${PACKAGE_NAME}"
  OutFile "${OUTFILE}"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${PACKAGE_NAME}-${PACKAGE_VERSION}"
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}" ""
  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Versioninfo

VIProductVersion "${VERSIONNUM}"
VIAddVersionKey "CompanyName"      "Warzone 2100 Project"
VIAddVersionKey "FileDescription"  "${PACKAGE_NAME} Installer"
VIAddVersionKey "FileVersion"      "${PACKAGE_VERSION}"
VIAddVersionKey "InternalName"     "${PACKAGE_NAME}"
VIAddVersionKey "LegalCopyright"   "Copyright (c) 2006-2018 Warzone 2100 Project"
VIAddVersionKey "OriginalFilename" "${PACKAGE}-${PACKAGE_VERSION}.exe"
VIAddVersionKey "ProductName"      "${PACKAGE_NAME}"
VIAddVersionKey "ProductVersion"   "${PACKAGE_VERSION}"

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
; !define MUI_LICENSEPAGE_RADIOBUTTONS

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
  !insertmacro MUI_LANGUAGE "German"

;--------------------------------
;License Language String

  LicenseLangString MUILicense ${LANG_ENGLISH} "..\..\COPYING"
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
;  File "..\..\src\warzone2100.exe"
  !insertmacro VPatchFile "warzone2100.exe.vpatch" "$INSTDIR\warzone2100.exe" "$INSTDIR\warzone2100.exe.tmp"

  ; Windows dbghelp library
  File "${EXTDIR}\bin\dbghelp.dll.license.txt"
  File "${EXTDIR}\bin\dbghelp.dll"
;  !insertmacro VPatchFile "dbghelp.dll.license.txt.vpatch" "$INSTDIR\dbghelp.dll.license.txt" "$INSTDIR\dbghelp.dll.license.txt.tmp"
;  !insertmacro VPatchFile "dbghelp.dll.vpatch" "$INSTDIR\dbghelp.dll" "$INSTDIR\dbghelp.dll.tmp"

  ; Data files
  ; TODO: FIXME: The list of files is outdated
;  File "..\..\data\mp.wz"
;  File "..\..\data\base.wz"
  !insertmacro VPatchFile "mp.wz.vpatch" "$INSTDIR\mp.wz" "$INSTDIR\mp.wz.tmp"
  !insertmacro VPatchFile "base.wz.vpatch" "$INSTDIR\base.wz" "$INSTDIR\base.wz.tmp"

  ; Information/documentation files
  File "/oname=ChangeLog.txt" "..\..\ChangeLog"
  File "/oname=Authors.txt" "..\..\AUTHORS"
  File "/oname=License.txt" "..\..\COPYING"
  File "/oname=README.md.txt" "..\..\README.md"

  SetOutPath "$INSTDIR\styles"

  File "/oname=readme.print.css" "..\..\doc\styles\readme.print.css"
  File "/oname=readme.screen.css" "..\..\doc\styles\readme.screen.css"

  ;Store installation folder
  WriteRegStr HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}" "" $INSTDIR

  ; Write the Windows-uninstall keys
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "DisplayName" "${PACKAGE_NAME}-${PACKAGE_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "DisplayVersion" "${PACKAGE_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "DisplayIcon" "$INSTDIR\${PACKAGE}.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "Publisher" "Warzone 2100 Project"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "URLInfoAbout" "${PACKAGE_BUGREPORT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}" "NoRepair" 1

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    SetOutPath "$INSTDIR"
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER-${PACKAGE_VERSION}"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER-${PACKAGE_VERSION}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER-${PACKAGE_VERSION}\${PACKAGE_NAME}.lnk" "$INSTDIR\${PACKAGE}.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER-${PACKAGE_VERSION}\Quick Start Guide (html).lnk" "$INSTDIR\doc\quickstartguide.html"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER-${PACKAGE_VERSION}\Quick Start Guide (pdf).lnk" "$INSTDIR\doc\quickstartguide.pdf"

  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

; Installs OpenAL runtime libraries, using Creative's installer
!ifndef PORTABLE
Section $(TEXT_SecOpenAL) SecOpenAL
  SetOutPath "$INSTDIR"
  File "${EXTDIR}\bin\oalinst.exe"
  ExecWait '"$INSTDIR\oalinst.exe" --silent'
SectionEnd
!endif

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

!ifndef PORTABLE
  LangString TEXT_SecOpenAL ${LANG_ENGLISH} "OpenAL libraries"
  LangString DESC_SecOpenAL ${LANG_ENGLISH} "Runtime libraries for OpenAL, a free Audio interface. Implementation by Creative Labs."
!endif

  LangString TEXT_SecBase ${LANG_DUTCH} "Standard installatie"
  LangString DESC_SecBase ${LANG_DUTCH} "Standard installatie."

  LangString TEXT_SecBase ${LANG_GERMAN} "Standardinstallation"
  LangString DESC_SecBase ${LANG_GERMAN} "Standardinstallation."

!ifndef PORTABLE
  LangString TEXT_SecOpenAL ${LANG_GERMAN} "OpenAL Bibliotheken"
  LangString DESC_SecOpenAL ${LANG_GERMAN} "Bibliotheken für OpenAL, ein freies Audio-Interface. Implementation von Creative Labs."
!endif

  LangString TEXT_RunWarzone ${LANG_ENGLISH} "Run Warzone 2100"
  LangString TEXT_RunWarzone ${LANG_GERMAN} "Starte Warzone 2100"



  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBase} $(DESC_SecBase)

!ifndef PORTABLE
    !insertmacro MUI_DESCRIPTION_TEXT ${SecOpenAL} $(DESC_SecOpenAL)
!endif

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

  Delete "$INSTDIR\README.md.txt"
  Delete "$INSTDIR\COPYING.NONGPL.txt"
  Delete "$INSTDIR\COPYING.README.txt"
  Delete "$INSTDIR\COPYING.txt"

  Delete "$INSTDIR\doc\images\*.*"
  Delete "$INSTDIR\doc\*.*"
  RMDir "$INSTDIR\doc\images"
  RMDIR "$INSTDIR\doc"

  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\Authors.txt"
  Delete "$INSTDIR\ChangeLog.txt"

  Delete "$INSTDIR\music\menu.ogg"
  Delete "$INSTDIR\music\track1.ogg"
  Delete "$INSTDIR\music\track2.ogg"
  Delete "$INSTDIR\music\track3.ogg"
  Delete "$INSTDIR\music\music.wpl"
  RMDir "$INSTDIR\music"

  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\fonts\fonts.conf"
  Delete "$INSTDIR\fonts\DejaVuSansMono.ttf"
  Delete "$INSTDIR\fonts\DejaVuSansMono-Bold.ttf"
  Delete "$INSTDIR\fonts\DejaVuSans.ttf"
  Delete "$INSTDIR\fonts\DejaVuSans-Bold.ttf"
  RMDir "$INSTDIR\fonts"

; remove all the locales

  Delete "$INSTDIR\locale\ca\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\ca\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ca"

  Delete "$INSTDIR\locale\cs\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\cs\LC_MESSAGES"
  RMDir "$INSTDIR\locale\cs"

  Delete "$INSTDIR\locale\da\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\da\LC_MESSAGES"
  RMDir "$INSTDIR\locale\da"

  Delete "$INSTDIR\locale\de\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\de\LC_MESSAGES"
  RMDir "$INSTDIR\locale\de"

  Delete "$INSTDIR\locale\el\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\el\LC_MESSAGES"
  RMDir "$INSTDIR\locale\el"

  Delete "$INSTDIR\locale\en_GB\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\en_GB\LC_MESSAGES"
  RMDir "$INSTDIR\locale\en_GB"

  Delete "$INSTDIR\locale\es\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\es\LC_MESSAGES"
  RMDir "$INSTDIR\locale\es"

  Delete "$INSTDIR\locale\et\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\et\LC_MESSAGES"
  RMDir "$INSTDIR\locale\et"

  Delete "$INSTDIR\locale\fi\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\fi\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fi"

  Delete "$INSTDIR\locale\fr\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\fr\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fr"

  Delete "$INSTDIR\locale\fy\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\fy\LC_MESSAGES"
  RMDir "$INSTDIR\locale\fy"

  Delete "$INSTDIR\locale\ga\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\ga\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ga"

  Delete "$INSTDIR\locale\hr\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\hr\LC_MESSAGES"
  RMDir "$INSTDIR\locale\hr"

  Delete "$INSTDIR\locale\hu\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\hu\LC_MESSAGES"
  RMDir "$INSTDIR\locale\hu"

  Delete "$INSTDIR\locale\it\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\it\LC_MESSAGES"
  RMDir "$INSTDIR\locale\it"

  Delete "$INSTDIR\locale\ko\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\ko\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ko"

  Delete "$INSTDIR\locale\la\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\la\LC_MESSAGES"
  RMDir "$INSTDIR\locale\la"

  Delete "$INSTDIR\locale\lt\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\lt\LC_MESSAGES"
  RMDir "$INSTDIR\locale\lt"

  Delete "$INSTDIR\locale\nb\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\nb\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nb"

  Delete "$INSTDIR\locale\nl\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\nl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\nl"

  Delete "$INSTDIR\locale\pl\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\pl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pl"

  Delete "$INSTDIR\locale\pt_BR\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\pt_BR\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pt_BR"

  Delete "$INSTDIR\locale\pt\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\pt\LC_MESSAGES"
  RMDir "$INSTDIR\locale\pt"

  Delete "$INSTDIR\locale\ro\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\ro\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ro"

  Delete "$INSTDIR\locale\ru\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\ru\LC_MESSAGES"
  RMDir "$INSTDIR\locale\ru"

  Delete "$INSTDIR\locale\sk\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\sk\LC_MESSAGES"
  RMDir "$INSTDIR\locale\sk"

  Delete "$INSTDIR\locale\sl\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\sl\LC_MESSAGES"
  RMDir "$INSTDIR\locale\sl"

  Delete "$INSTDIR\locale\tr\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\tr\LC_MESSAGES"
  RMDir "$INSTDIR\locale\tr"

  Delete "$INSTDIR\locale\uk\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\uk\LC_MESSAGES"
  RMDir "$INSTDIR\locale\uk"

  Delete "$INSTDIR\locale\zh_TW\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\zh_TW\LC_MESSAGES"
  RMDir "$INSTDIR\locale\zh_TW"

  Delete "$INSTDIR\locale\zh_CN\LC_MESSAGES\warzone2100.mo"
  RMDir "$INSTDIR\locale\zh_CN\LC_MESSAGES"
  RMDir "$INSTDIR\locale\zh_CN"

  RMDir "$INSTDIR\locale"
  RMDir "$INSTDIR"

  SetShellVarContext all

; remove the desktop shortcut icon

  Delete "$DESKTOP\${PACKAGE_NAME}-${PACKAGE_VERSION}.lnk"

; and now, lets really remove the startmenu entries...

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

  Delete "$SMPROGRAMS\$MUI_TEMP-${PACKAGE_VERSION}\Uninstall.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP-${PACKAGE_VERSION}\${PACKAGE_NAME}.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP-${PACKAGE_VERSION}\Quick Start Guide (html).lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP-${PACKAGE_VERSION}\Quick Start Guide (pdf).lnk"

  ;Delete empty start menu parent directories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP-${PACKAGE_VERSION}"

  startMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

    IfErrors startMenuDeleteLoopDone

    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegValue HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}" "Start Menu Folder"
  DeleteRegValue HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}" ""
  DeleteRegKey /ifempty HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}"

  ; Unregister with Windows' uninstall system
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}-${PACKAGE_VERSION}"

SectionEnd

;--------------------------------
;Uninstaller Functions

!ifndef PORTABLE
Function un.onInit
  !verbose push
  !verbose push
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd
!endif
