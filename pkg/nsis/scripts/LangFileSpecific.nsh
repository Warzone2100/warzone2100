/*

DERIVED FROM NSIS LangFile.nsh
License: (see NSIS_COPYING.txt)

***************
Modifications:
- Offers LANGFILE_SPECIFIC_INCLUDE and LANGFILE_SPECIFIC_INCLUDE_WITHDEFAULT
  which take an EXPLICIT_LANG_IDNAME parameter.
  With these, custom installer string .nsh files no longer need the:
  !insertmacro LANGFILE_EXT <Language>
  line at the top.
***************

Original LangFile.nsh comments (modified):


Header file to create language files that can be
included with a single command.

Copyright 2008-2020 Joost Verburg, Anders Kjersem

* Either LANGFILE_SPECIFIC_INCLUDE or LANGFILE_SPECIFIC_INCLUDE_WITHDEFAULT
  can be called from the script to include a language file.

  - LANGFILE_SPECIFIC_INCLUDE takes the language name, and the language file name as parameters.
  - LANGFILE_SPECIFIC_INCLUDE_WITHDEFAULT takes as additional third
    parameter, the default language file to load missing strings from.

* Language strings in the language file have the format:
  ${LangFileString} LANGSTRING_NAME "Text"

* Example:

  ; Setup.nsi
  !include "MUI.nsh"
  !include "LangFileSpecific.nsh"
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro LANGFILE_SPECIFIC_INCLUDE "English" "EnglishExtra.nsh"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro LANGFILE_SPECIFIC_INCLUDE "Swedish" "SwedishExtra.nsh"
  Section
  MessageBox MB_OK "$(myCustomString)"
  SectionEnd

  ; SwedishExtra.nsh
  ${LangFileString} myCustomString "Bork bork"

*/

!ifndef LANGFILE_SPECIFIC_LANG_INCLUDED
!define LANGFILE_SPECIFIC_LANG_INCLUDED

!macro LANGFILE_SPECIFIC_INCLUDE EXPLICIT_LANG_IDNAME FILENAME

  ;Called from script: include a language file

  !ifdef LangFileString
    !undef LangFileString
  !endif

  !define LangFileString "!insertmacro LANGFILE_SPECIFIC_SETSTRING"
  
  !ifdef LANGFILE_SPECIFIC_IDNAME
    !undef LANGFILE_SPECIFIC_IDNAME
  !endif
  !define LANGFILE_SPECIFIC_IDNAME "${EXPLICIT_LANG_IDNAME}"

  !define LANGFILE_SPECIFIC_SETNAMES
  !include "${FILENAME}"
  !undef LANGFILE_SPECIFIC_SETNAMES

  ;Create language strings
  !define /redef LangFileString "!insertmacro LANGFILE_SPECIFIC_LANGSTRING"
  !include "${FILENAME}"

!macroend

!macro LANGFILE_SPECIFIC_INCLUDE_WITHDEFAULT EXPLICIT_LANG_IDNAME FILENAME FILENAME_DEFAULT

  ;Called from script: include a language file
  ;Obtains missing strings from a default file

  !ifdef LangFileString
    !undef LangFileString
  !endif

  !define LangFileString "!insertmacro LANGFILE_SPECIFIC_SETSTRING"
  
  !ifdef LANGFILE_SPECIFIC_IDNAME
    !undef LANGFILE_SPECIFIC_IDNAME
  !endif
  !define LANGFILE_SPECIFIC_IDNAME "${EXPLICIT_LANG_IDNAME}"

  !define LANGFILE_SPECIFIC_SETNAMES
  !include "${FILENAME}"
  !undef LANGFILE_SPECIFIC_SETNAMES

  ;Include default language for missing strings
  !define LANGFILE_SPECIFIC_PRIV_INCLUDEISFALLBACK "${FILENAME_DEFAULT}"
  !include "${FILENAME_DEFAULT}"
  !undef LANGFILE_SPECIFIC_PRIV_INCLUDEISFALLBACK

  ;Create language strings
  !define /redef LangFileString "!insertmacro LANGFILE_SPECIFIC_LANGSTRING"
  !include "${FILENAME_DEFAULT}"

!macroend

!macro LANGFILE_SPECIFIC_SETSTRING NAME VALUE

  ;Set define with translated string

  !ifndef ${NAME}
    !define "${NAME}" "${VALUE}"
    !ifdef LANGFILE_SPECIFIC_PRIV_INCLUDEISFALLBACK
      !warning 'LangString "${NAME}" for language ${LANGFILE_SPECIFIC_IDNAME} is missing, using fallback from "${LANGFILE_SPECIFIC_PRIV_INCLUDEISFALLBACK}"'
    !endif
  !endif

!macroend

!macro LANGFILE_SPECIFIC_LANGSTRING NAME DUMMY

  ;Create a language string from a define and undefine

  LangString "${NAME}" "${LANG_${LANGFILE_SPECIFIC_IDNAME}}" "${${NAME}}"
  !undef "${NAME}"

!macroend

!endif
