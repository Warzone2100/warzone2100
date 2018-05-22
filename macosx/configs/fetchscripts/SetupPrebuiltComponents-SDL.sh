#!/bin/sh

DirectorY="SDL-2.0.8"
OutDir="SDL2"
FileName="SDL2-2.0.8.dmg"
BuiltDLP="https://www.libsdl.org/release/SDL2-2.0.8.dmg"
SHA256Sum="74dd2cb6b18e35e8181523590115f10f1da774939c21ce27768a2a80ba57ad5f"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}" "-dmg"
exit ${?}

# tar -czf SDL-2.0.8.tgz --exclude '.DS_Store' SDL-2.0.8
