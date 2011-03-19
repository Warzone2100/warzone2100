echo "Building autorevision.h... (msysgit must be in system path)"
IF EXIST %1..\src\autorevision.h del %1..\src\autorevision.h
sh.exe %1..\build_tools\autorevision.sh %1..\src\autorevision.h.lf
IF NOT EXIST %1..\src\autorevision.h.lf goto failed: 
echo "converting file to CRLF from LF"
perl -p -e 's/\n/\r\n/' < %1..\src\autorevision.h.lf > %1..\src\autorevision.h
del %1..\src\autorevision.h.lf
IF EXIST %1..\src\autorevision.h goto good:
:failed
echo "Failed! Is msysgit in your system path?"
exit
:good
echo "File has been generated."
exit
