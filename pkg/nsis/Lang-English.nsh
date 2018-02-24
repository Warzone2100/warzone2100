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
;  Warzone 2100 Project Installer - English Translations
;

!ifndef PORTABLE
  ;English
  LangString WZWelcomeText ${LANG_ENGLISH} "Welcome to the Warzone 2100 installer!\r\n\r\nThis wizard will guide you through the installation of Warzone 2100.\r\n\r\nIt is recommended that you close all other applications before continuing this installation. This will make it possible to update relevant system files without having to reboot your computer.\r\n\r\nWarzone 2100 is 100% free, fully open sourced program\r\n\r\nClick Next to continue."
!else
  LangString WZWelcomeText ${LANG_ENGLISH} "Welcome to the Warzone 2100 portable installer!\r\n\r\nThis wizard will guide you through the installation of the portable version of Warzone 2100.\r\n\r\nThis install is fully self-contained and you can uninstall the program at any time by deleting the directory.\r\n\r\nWarzone 2100 is 100% free, fully open sourced program! \r\n\r\nClick Next to continue."
!endif
  LangString WZ_GPL_NEXT ${LANG_ENGLISH} "Next"


  LangString TEXT_SecBase ${LANG_ENGLISH} "Core files"
  LangString DESC_SecBase ${LANG_ENGLISH} "The core files required to run Warzone 2100."

  LangString TEXT_SecFMVs ${LANG_ENGLISH} "Videos"
  LangString DESC_SecFMVs ${LANG_ENGLISH} "Download and install in-game cutscenes."

  LangString TEXT_SecFMVs_EngHi ${LANG_ENGLISH} "English (HQ)"
  LangString DESC_SecFMVs_EngHi ${LANG_ENGLISH} "Download and install higher-quality English in-game cutscenes (920 MB)."

  LangString TEXT_SecFMVs_Eng ${LANG_ENGLISH} "English"
  LangString DESC_SecFMVs_Eng ${LANG_ENGLISH} "Download and install English in-game cutscenes (545 MB)."

  LangString TEXT_SecFMVs_EngLo ${LANG_ENGLISH} "English (LQ)"
  LangString DESC_SecFMVs_EngLo ${LANG_ENGLISH} "Download and install a low-quality version of English in-game cutscenes (162 MB)."

  LangString TEXT_SecFMVs_Ger ${LANG_ENGLISH} "German"
  LangString DESC_SecFMVs_Ger ${LANG_ENGLISH} "Download and install German in-game cutscenes (460 MB)."

  LangString TEXT_SecNLS ${LANG_ENGLISH} "Language files"
  LangString DESC_SecNLS ${LANG_ENGLISH} "Support for languages other than English."

  LangString TEXT_SecNLS_WinFonts ${LANG_ENGLISH} "WinFonts"
  LangString DESC_SecNLS_WinFonts ${LANG_ENGLISH} "Include Windows Fonts folder into the search path. Enable this if you want to use custom fonts in config file or having troubles with standard font. Can be slow on Vista and later!"
