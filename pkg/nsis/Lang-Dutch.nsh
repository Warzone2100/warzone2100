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

  ;Dutch
  LangString WZWelcomeText ${LANG_DUTCH} "Deze installatiewizard leidt u door het installatieproces van Warzone 2100.\r\n\r\nHet is aangeraden om alle andere applicaties te sluiten alvorens verder te gaan met deze installatie. Dit maakt het mogelijk om de betreffende systeembestanden te vervangen zonder uw computer opnieuw op te starten"
  LangString WZ_GPL_NEXT ${LANG_DUTCH} "volgende"


  LangString TEXT_SecBase ${LANG_DUTCH} "Core files"
  LangString DESC_SecBase ${LANG_DUTCH} "The core files required to run Warzone 2100."

  LangString TEXT_SecFMVs ${LANG_DUTCH} "Videos"
  LangString DESC_SecFMVs ${LANG_DUTCH} "Download and install in-game cutscenes."

  LangString TEXT_SecFMVs_EngHi ${LANG_DUTCH} "English (HQ)"
  LangString DESC_SecFMVs_EngHi ${LANG_DUTCH} "Download and install higher-quality English in-game cutscenes (920 MB)."

  LangString TEXT_SecFMVs_Eng ${LANG_DUTCH} "English"
  LangString DESC_SecFMVs_Eng ${LANG_DUTCH} "Download and install English in-game cutscenes (545 MB)."

  LangString TEXT_SecFMVs_EngLo ${LANG_DUTCH} "English (LQ)"
  LangString DESC_SecFMVs_EngLo ${LANG_DUTCH} "Download and install a low-quality version of English in-game cutscenes (162 MB)."

  LangString TEXT_SecFMVs_Ger ${LANG_DUTCH} "German"
  LangString DESC_SecFMVs_Ger ${LANG_DUTCH} "Download and install German in-game cutscenes (460 MB)."

  LangString TEXT_SecNLS ${LANG_DUTCH} "Language files"
  LangString DESC_SecNLS ${LANG_DUTCH} "Ondersteuning voor andere talen dan Engels (Nederlands inbegrepen)."

  LangString TEXT_SecNLS_WinFonts ${LANG_DUTCH} "WinFonts"
  LangString DESC_SecNLS_WinFonts ${LANG_DUTCH} "Include Windows Fonts folder into the search path. Enable this if you want to use custom fonts in config file or having troubles with standard font. Can be slow on Vista and later!"

  LangString TEXT_SecMSSysLibraries ${LANG_DUTCH} "Important Microsoft Runtime DLLs"
  LangString DESC_SecMSSysLibraries ${LANG_DUTCH} "Download and install (or update) Microsoft's Visual C++ redistributable system libraries, which some components may require to run."
