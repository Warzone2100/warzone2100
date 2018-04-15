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

  ;German
  LangString WZWelcomeText ${LANG_GERMAN} "Dieser Wizard wird Sie durch die Warzone-2100-Installation führen.\r\n\r\nEs wird empfohlen sämtliche anderen Anwendungen zu schließen, bevor Sie das Setup starten. Dies ermöglicht es relevante Systemdateien zu aktualisieren, ohne neustarten zu müssen.\r\n\r\nWarzone 2100 ist zu 100% kostenlos, falls Sie dafür gezahlt haben, lassen Sie es uns wissen!\r\n\r\nKlicken Sie auf Weiter, um fortzufahren."
  LangString WZ_GPL_NEXT ${LANG_GERMAN} "nächste"


  LangString TEXT_SecBase ${LANG_GERMAN} "Core files"
  LangString DESC_SecBase ${LANG_GERMAN} "Die Kerndateien, die für Warzone 2100 benötigt werden."

  LangString TEXT_SecFMVs ${LANG_GERMAN} "Videos"
  LangString DESC_SecFMVs ${LANG_GERMAN} "Videos herunterladen und installieren."

  LangString TEXT_SecFMVs_EngHi ${LANG_GERMAN} "English (HQ)"
  LangString DESC_SecFMVs_EngHi ${LANG_GERMAN} "Videos in besserer Qualität und auf Englisch herunterladen und installieren (920 MiB)."

  LangString TEXT_SecFMVs_Eng ${LANG_GERMAN} "English"
  LangString DESC_SecFMVs_Eng ${LANG_GERMAN} "Die englischen Videos herunterladen und installieren (545 MiB)."

  LangString TEXT_SecFMVs_EngLo ${LANG_GERMAN} "English (LQ)"
  LangString DESC_SecFMVs_EngLo ${LANG_GERMAN} "Die englischen Videos in geringer Qualität herunterladen und installieren (162 MiB)."

  LangString TEXT_SecFMVs_Ger ${LANG_GERMAN} "German"
  LangString DESC_SecFMVs_Ger ${LANG_GERMAN} "Die deutschen Videos herunterladen und installieren (460 MiB)."

  LangString TEXT_SecNLS ${LANG_GERMAN} "Language files"
  LangString DESC_SecNLS ${LANG_GERMAN} "Unterstützung für Sprachen außer Englisch (Deutsch inbegriffen)."

  LangString TEXT_SecNLS_WinFonts ${LANG_GERMAN} "WinFonts"
  LangString DESC_SecNLS_WinFonts ${LANG_GERMAN} "Den Windows-Schriftarten-Ordner in den Suchpfad aufnehmen. Nutzen Sie dies, falls Sie später eigene Schriftarten in der Konfigurationsdatei eingeben wollen oder es zu Problemen mit der Standardschriftart kommt. Kann unter Vista und später langsam sein!"

  LangString TEXT_SecMSSysLibraries ${LANG_GERMAN} "Important Microsoft Runtime DLLs"
  LangString DESC_SecMSSysLibraries ${LANG_GERMAN} "Download and install (or update) Microsoft's Visual C++ redistributable system libraries, which some components may require to run."
