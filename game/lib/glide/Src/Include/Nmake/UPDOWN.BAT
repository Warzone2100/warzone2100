@echo off
echo ====recursing into %1 (make %3) =======================================
cd %1
if ERRORLEVEL 1 goto exit
%2 /nologo %3 %4 %5 %6 %7 %8 %9
if ERRORLEVEL 1 goto exit
cd ..
:exit
