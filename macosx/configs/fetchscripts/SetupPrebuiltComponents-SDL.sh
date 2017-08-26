#!/bin/sh

DirectorY="SDL-2.0.5"
OutDir="SDL2"
FileName="SDL2-2.0.5.dmg"
BuiltDLP="https://www.libsdl.org/release/SDL2-2.0.5.dmg"
SHA256Sum="09309e5af6739ce91e8e5db443604a0d0d85e0b728652423ba1a00e26363c30c"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}" "-dmg"
exit ${?}

# tar -czf SDL-2.0.5.tgz --exclude '.DS_Store' SDL-2.0.5
