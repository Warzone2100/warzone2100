;  This file is part of Warzone 2100.
;  Copyright (C) 2006-2018  Warzone 2100 Project
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
;  Warzone 2100 Project Installer script
;

;--------------------------------
;Include section

  !include "MUI.nsh"
  !include "FileFunc.nsh"
  !include "LogicLib.nsh"

;--------------------------------
;General
  CRCCheck on  ;make sure this isn't corrupted

  ;Name and file
  Name "${PACKAGE_NAME}"
  OutFile "${OUTFILE}"

!ifndef PORTABLE
  ;Default installation folder
  InstallDir "$PROGRAMFILES\${PACKAGE_NAME}-${PACKAGE_VERSION}"
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}" ""
  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin
!else
  RequestExecutionLevel user
!endif

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

!ifndef PORTABLE
  Var MUI_TEMP
  Var STARTMENU_FOLDER
!endif

;--------------------------------
;Interface Settings

!ifndef PORTABLE
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100header.bmp"
  !define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEPAGE_TITLE "Welcome to Warzone 2100 v. ${PACKAGE_VERSION}"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100welcome.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100welcome.bmp"
  !define MUI_ICON "${TOP_SRCDIR}\icons\warzone2100.ico"
  !define MUI_UNICON "${TOP_SRCDIR}\icons\warzone2100.uninstall.ico"
 !else
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100headerP.bmp"
  !define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEPAGE_TITLE "Warzone 2100 v. ${PACKAGE_VERSION} Portable"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100welcomeP.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${TOP_SRCDIR}\icons\wz2100welcomeP.bmp"
  !define MUI_ICON "${TOP_SRCDIR}\icons\wz2100portable.ico"
  !define MUI_UNICON "${TOP_SRCDIR}\icons\wz2100portableU.ico"
!endif

  !define MUI_WELCOMEPAGE_TEXT   "$(WZWelcomeText)"
  !define MUI_ABORTWARNING

!ifndef PORTABLE
  ;Start Menu Folder Page Configuration (for MUI_PAGE_STARTMENU)
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PACKAGE_NAME}-${PACKAGE_VERSION}"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!endif
  ; These indented statements modify settings for MUI_PAGE_FINISH
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_UNFINISHPAGE_NOAUTOCLOSE

  !define MUI_LICENSEPAGE_RADIOBUTTONS
  !define MUI_LICENSEPAGE_RADIOBUTTONS_TEXT_ACCEPT "$(WZ_GPL_NEXT)"

;--------------------------------
;Pages

  !define MUI_PAGE_CUSTOMFUNCTION_PRE WelcomePageSetupLinkPre
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW WelcomePageSetupLinkShow
  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "${TOP_SRCDIR}\COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
!ifndef PORTABLE
  !insertmacro MUI_PAGE_STARTMENU "Application" $STARTMENU_FOLDER
!endif
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
  !insertmacro MUI_LANGUAGE "Russian"

;--------------------------------
;Reserve Files

  ;These files should be inserted before other files in the data block
  ;Keep these lines before any File command
  ;Only for solid compression (by default, solid compression is enabled for BZIP2 and LZMA)

  !insertmacro MUI_RESERVEFILE_LANGDLL
Function WelcomePageSetupLinkPre
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Bottom" "142" ; limit size of the upper label
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Settings" "Numfields" "4" ; increase counter
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Type" "Link"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Text" "Visit our Official Homepage http://wz2100.net for the latest version."
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "State" "http://wz2100.net/"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Left" "120"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Right" "300"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Top" "154"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Bottom" "172"
FunctionEnd

Function WelcomePageSetupLinkShow
  Push $0
  GetDlgItem $0 $MUI_HWND 1203
  SetCtlColors $0 "0000FF" "FFFFFF"
  ; underline font
  CreateFont $1 "$(^Font)" "$(^FontSize)" "400" /UNDERLINE
  SendMessage $0 ${WM_SETFONT} $1 1
  Pop $0
FunctionEnd

;--------------------------------
;Installer Sections

Section $(TEXT_SecBase) SecBase
  SectionIn RO
  SetOutPath "$INSTDIR"
  SetShellVarContext all

  ;ADD YOUR OWN FILES HERE...

  ; Main executable
  File "${TOP_BUILDDIR}\src\${PACKAGE}.exe"

  ; Windows dbghelp library
  File "${TOP_SRCDIR}\win32\dbghelp\6.12\x86\dbghelp.dll.license.txt"
  File "${TOP_SRCDIR}\win32\dbghelp\6.12\x86\dbghelp.dll"

!ifdef INSTALLVIDS
; sequences.wz
  File "${EXTDIR}\opt\sequences.wz"
!endif
  ; Data files
  File "${TOP_BUILDDIR}\data\mp.wz"
  File "${TOP_BUILDDIR}\data\base.wz"

  ; Information/documentation files (convert eols for text files)
  File "${TOP_SRCDIR}\ChangeLog"
  Push "ChangeLog"
  Push "ChangeLog.txt"
  Call unix2dos

  File "${TOP_SRCDIR}\AUTHORS"
  Push "AUTHORS"
  Push "Authors.txt"
  Call unix2dos

  File "${TOP_SRCDIR}\COPYING.NONGPL"
  Push "COPYING.NONGPL"
  Push "COPYING.NONGPL.txt"
  Call unix2dos

  File "${TOP_SRCDIR}\COPYING"
  Push "COPYING"
  Push "COPYING.txt"
  Call unix2dos

  File "${TOP_SRCDIR}\COPYING.README"
  Push "COPYING.README"
  Push "COPYING.README.txt"
  Call unix2dos

  File "${TOP_SRCDIR}\README.md"
  Push "README.md"
  Push "README.md.txt"
  Call unix2dos

  SetOutPath "$INSTDIR\doc"
  File "${TOP_SRCDIR}\doc\quickstartguide.html"
  File "${TOP_SRCDIR}\doc\quickstartguide.pdf"
  File "${TOP_SRCDIR}\doc\docbook-xsl.css"
  SetOutPath "$INSTDIR\doc\images"
  File "${TOP_SRCDIR}\doc\images\*.*"

  ; Music files
  SetOutPath "$INSTDIR\music"
  File "${TOP_SRCDIR}\data\music\menu.ogg"
  File "${TOP_SRCDIR}\data\music\track1.ogg"
  File "${TOP_SRCDIR}\data\music\track2.ogg"
  File "${TOP_SRCDIR}\data\music\track3.ogg"
  File "${TOP_SRCDIR}\data\music\music.wpl"

  SetOutPath "$INSTDIR\fonts"
  File "/oname=fonts.conf" "${TOP_SRCDIR}\win32\fontconfig\fonts.conf.wd_disable"
  File "${TOP_SRCDIR}\3rdparty\fonts\DejaVu\DejaVuSans.ttf"
  File "${TOP_SRCDIR}\3rdparty\fonts\DejaVu\DejaVuSans-Bold.ttf"
  File "/oname=DejaVu.LICENSE.txt" "${TOP_SRCDIR}\3rdparty\fonts\DejaVu\LICENSE"

!ifndef PORTABLE

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

  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\${PACKAGE_NAME}-${PACKAGE_VERSION}.lnk" "$INSTDIR\${PACKAGE}.exe"
!endif
SectionEnd

; NOTE: you need upgraded security privs to install shortcut on desktop for portable.

!ifndef INSTALLVIDS
SectionGroup $(TEXT_SecFMVs) SecFMVs

Section /o $(TEXT_SecFMVs_EngHi) SecFMVs_EngHi
  SetOutPath "$INSTDIR"
  IfFileExists "sequences.wz" +5
    NSISdl::download "http://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/high-quality-en/sequences.wz"               "sequences.wz"
    Pop $R0 ; Get the return value
    StrCmp $R0 "success" +2
      MessageBox MB_OK|MB_ICONSTOP "Download of videos failed: $R0"
SectionEnd

Section /o $(TEXT_SecFMVs_Eng) SecFMVs_Eng
  SetOutPath "$INSTDIR"
  IfFileExists "sequences.wz" +5
    NSISdl::download "http://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/standard-quality-en/sequences.wz"               "sequences.wz"
    Pop $R0 ; Get the return value
    StrCmp $R0 "success" +2
      MessageBox MB_OK|MB_ICONSTOP "Download of videos failed: $R0"
SectionEnd

Section /o $(TEXT_SecFMVs_EngLo) SecFMVs_EngLo
  SetOutPath "$INSTDIR"
  IfFileExists "sequences.wz" +5
    NSISdl::download "http://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/low-quality-en/sequences.wz"               "sequences.wz"
    Pop $R0 ; Get the return value
    StrCmp $R0 "success" +2
      MessageBox MB_OK|MB_ICONSTOP "Download of videos failed: $R0"
SectionEnd

;Section /o $(TEXT_SecFMVs_Ger) SecFMVs_Ger
;  SetOutPath "$INSTDIR"
;  IfFileExists "sequences.wz" +5
;    NSISdl::download "http://download.gna.org/warzone/videos/2.2/warzone2100-sequences-ger-hi-2.2.wz"               "sequences.wz"
;    Pop $R0 ; Get the return value
;    StrCmp $R0 "success" +2
;      MessageBox MB_OK|MB_ICONSTOP "Download of videos failed: $R0"
;
;SectionEnd

SectionGroupEnd
!endif

SectionGroup $(TEXT_SecNLS) SecNLS

Section "-NLS files" SecNLS_files
  SetOutPath "$INSTDIR\locale\ca\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\ca_ES.gmo"

  SetOutPath "$INSTDIR\locale\cs\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\cs.gmo"

  SetOutPath "$INSTDIR\locale\da\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\da.gmo"

  SetOutPath "$INSTDIR\locale\de\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\de.gmo"

  SetOutPath "$INSTDIR\locale\el\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\el.gmo"

  SetOutPath "$INSTDIR\locale\en_GB\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\en_GB.gmo"

  SetOutPath "$INSTDIR\locale\es\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\es.gmo"

  SetOutPath "$INSTDIR\locale\et\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\et_EE.gmo"

  SetOutPath "$INSTDIR\locale\fi\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\fi.gmo"

  SetOutPath "$INSTDIR\locale\fr\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\fr.gmo"

  SetOutPath "$INSTDIR\locale\fy\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\fy.gmo"

  SetOutPath "$INSTDIR\locale\ga\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\ga.gmo"

  SetOutPath "$INSTDIR\locale\hr\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\hr.gmo"

  SetOutPath "$INSTDIR\locale\hu\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\hu.gmo"

  SetOutPath "$INSTDIR\locale\it\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\it.gmo"

  SetOutPath "$INSTDIR\locale\ko\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\ko.gmo"

  SetOutPath "$INSTDIR\locale\la\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\la.gmo"

  SetOutPath "$INSTDIR\locale\lt\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\lt.gmo"

  SetOutPath "$INSTDIR\locale\nb\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\nb.gmo"

  SetOutPath "$INSTDIR\locale\nl\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\nl.gmo"

  SetOutPath "$INSTDIR\locale\pl\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\pl.gmo"

  SetOutPath "$INSTDIR\locale\pt_BR\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\pt_BR.gmo"

  SetOutPath "$INSTDIR\locale\pt\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\pt.gmo"

  SetOutPath "$INSTDIR\locale\ro\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\ro.gmo"

  SetOutPath "$INSTDIR\locale\ru\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\ru.gmo"

  SetOutPath "$INSTDIR\locale\sk\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\sk.gmo"

  SetOutPath "$INSTDIR\locale\sl\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\sl.gmo"

  SetOutPath "$INSTDIR\locale\tr\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\tr.gmo"

  SetOutPath "$INSTDIR\locale\uk\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\uk_UA.gmo"

  SetOutPath "$INSTDIR\locale\zh_TW\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\zh_TW.gmo"

  SetOutPath "$INSTDIR\locale\zh_CN\LC_MESSAGES"
  File "/oname=warzone2100.mo" "${TOP_SRCDIR}\po\zh_CN.gmo"

SectionEnd

;Replace fonts.conf with Windows 'fonts' enabled one
Section /o $(TEXT_SecNLS_WinFonts) SecNLS_WinFonts
  SetOutPath "$INSTDIR\fonts"
  Delete "$INSTDIR\fonts\fonts.conf"
  File "/oname=fonts.conf" "${TOP_SRCDIR}\win32\fontconfig\fonts.conf.wd_enable"
SectionEnd

SectionGroupEnd

;--------------------------------
;Installer Functions

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
!ifndef INSTALLVIDS
  # increase required size of section 'SecFMVs_EngHi' by file size
  SectionGetSize ${SecFMVs_EngHi} $0
  IntOp $0 $0 + 671938;135
  SectionSetSize ${SecFMVs_EngHi} $0

  # increase required size of section 'SecFMVs_Eng' by file size
  SectionGetSize ${SecFMVs_Eng} $0
  IntOp $0 $0 + 571937;134
  SectionSetSize ${SecFMVs_Eng} $0

  # increase required size of section 'SecFMVs_EngLo' by file size
  SectionGetSize ${SecFMVs_EngLo} $0
  IntOp $0 $0 + 165681;436
  SectionSetSize ${SecFMVs_EngLo} $0

  # increase required size of section 'SecFMVs_Ger' by file size
;  SectionGetSize ${SecFMVs_Ger} $0
;  IntOp $0 $0 + 499187;492
;  SectionSetSize ${SecFMVs_Ger} $0

  ;HACK: Set section 'Video' as read-only
  SectionGetFlags ${SecFMVs} $0
  IntOp $0 $0 ^ ${SF_SELECTED}
  IntOp $0 $0 | ${SF_RO}
  SectionSetFlags ${SecFMVs} $0

  ;FIXME: Select default video sub-component
  ; Default is still set to standard instead of high, since there is a ~450MB difference
  StrCpy $5 ${SecFMVs_Eng}
!endif

!ifdef PORTABLE
 System::Call "kernel32::GetCurrentDirectory(i ${NSIS_MAX_STRLEN}, t .r0)"
 GetFullPathName $0 "$0\"
 StrCpy $INSTDIR "$0${PACKAGE_NAME}-${PACKAGE_VERSION}\"
!endif
FunctionEnd

!ifndef INSTALLVIDS
Function .onSelChange
${If} ${SectionIsSelected} ${SecFMVs_Eng}
${OrIf} ${SectionIsSelected} ${SecFMVs_EngHi}
${OrIf} ${SectionIsSelected} ${SecFMVs_EngLo}
;${OrIf} ${SectionIsSelected} ${SecFMVs_Ger}
  !insertmacro StartRadioButtons $5
    !insertmacro RadioButton ${SecFMVs_EngHi}
    !insertmacro RadioButton ${SecFMVs_Eng}
    !insertmacro RadioButton ${SecFMVs_EngLo}
;   !insertmacro RadioButton ${SecFMVs_Ger}
  !insertmacro EndRadioButtons
${EndIf}
FunctionEnd
!endif

Function unix2dos
    ; strips all CRs and then converts all LFs into CRLFs
    ; (this is roughly equivalent to "cat file | dos2unix | unix2dos")
    ; beware that this function destroys $0 $1 $2
    ;
    ; usage:
    ;    Push "infile"
    ;    Push "outfile"
    ;    Call unix2dos
    ClearErrors
    Pop $2
    FileOpen $1 $2 w      ;$1 = file output (opened for writing)
    Pop $2
    FileOpen $0 $2 r      ;$0 = file input (opened for reading)
    Push $2               ;save name for deleting
    IfErrors unix2dos_done

unix2dos_loop:
    FileReadByte $0 $2  ; read a byte (stored in $2)
    IfErrors unix2dos_done  ; EOL
    StrCmp $2 13 unix2dos_loop  ; skip CR
    StrCmp $2 10 unix2dos_cr unix2dos_write  ; if LF write an extra CR

unix2dos_cr:
    FileWriteByte $1 13

unix2dos_write:
    FileWriteByte $1 $2  ; write byte
    Goto unix2dos_loop   ; read next byte

unix2dos_done:
    FileClose $0         ; close files
    FileClose $1
    Pop $0
    Delete $0            ; delete original

FunctionEnd

;--------------------------------
;Descriptions

  !include "Lang-English.nsh"
  !include "Lang-Dutch.nsh"
  !include "Lang-German.nsh"
  !include "Lang-Russian.nsh"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBase} $(DESC_SecBase)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs} $(DESC_SecFMVs)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs_Eng} $(DESC_SecFMVs_Eng)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs_EngHi} $(DESC_SecFMVs_EngHi)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs_EngLo} $(DESC_SecFMVs_EngLo)
;  !insertmacro MUI_DESCRIPTION_TEXT ${SecFMVs_Ger} $(DESC_SecFMVs_Ger)

    !insertmacro MUI_DESCRIPTION_TEXT ${SecNLS} $(DESC_SecNLS)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecNLS_WinFonts} $(DESC_SecNLS_WinFonts)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section
!ifndef PORTABLE

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\${PACKAGE}.exe"

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
!endif

;--------------------------------
;Uninstaller Functions

!ifndef PORTABLE
Function un.onInit
  !verbose push
  !verbose push
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd
!endif
