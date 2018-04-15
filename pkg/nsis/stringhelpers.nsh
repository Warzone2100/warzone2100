!define StrLoc "!insertmacro StrLoc"

!macro StrLoc ResultVar String SubString StartPoint
Push "${String}"
Push "${SubString}"
Push "${StartPoint}"
Call StrLoc
Pop "${ResultVar}"
!macroend

Function StrLoc
/*After this point:
------------------------------------------
$R0 = StartPoint (input)
$R1 = SubString (input)
$R2 = String (input)
$R3 = SubStringLen (temp)
$R4 = StrLen (temp)
$R5 = StartCharPos (temp)
$R6 = TempStr (temp)*/

;Get input from user
Exch $R0
Exch
Exch $R1
Exch 2
Exch $R2
Push $R3
Push $R4
Push $R5
Push $R6

;Get "String" and "SubString" length
StrLen $R3 $R1
StrLen $R4 $R2
;Start "StartCharPos" counter
StrCpy $R5 0

;Loop until "SubString" is found or "String" reaches its end
${Do}
;Remove everything before and after the searched part ("TempStr")
StrCpy $R6 $R2 $R3 $R5

;Compare "TempStr" with "SubString"
${If} $R6 == $R1
${If} $R0 == `<`
IntOp $R6 $R3 + $R5
IntOp $R0 $R4 - $R6
${Else}
StrCpy $R0 $R5
${EndIf}
${ExitDo}
${EndIf}
;If not "SubString", this could be "String"'s end
${If} $R5 >= $R4
StrCpy $R0 ``
${ExitDo}
${EndIf}
;If not, continue the loop
IntOp $R5 $R5 + 1
${Loop}

;Return output to user
Pop $R6
Pop $R5
Pop $R4
Pop $R3
Pop $R2
Exch
Pop $R1
Exch $R0
FunctionEnd
