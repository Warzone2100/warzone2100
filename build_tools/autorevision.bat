echo "Building autorevision.h... (msysgit must be in system path)"
cd %1..\
echo %1
echo %cd%
IF EXIST autorevision.h del autorevision.h
sh.exe %1../build_tools/autorevision -t h > src/autorevision.h.lf
IF NOT EXIST src\autorevision.h.lf goto failed:
echo "converting file to CRLF from LF"
perl -p -e 's/\n/\r\n/' < src\autorevision.h.lf > src\autorevision.h
del src\autorevision.h.lf
IF EXIST src\autorevision.h goto good:
:failed
echo "Failed! Is msysgit in your system path?"
exit
:good
echo "File has been generated."
exit
