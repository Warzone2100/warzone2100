#!/bin/sh

DirectorY="SDL-1.2.15"
OutDir="SDL"
FileName="SDL-1.2.15.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/SDL-1.2.15.tgz"
MD5Sum="11191d1d17befc3a2ddc9065effacfac"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}

# tar -czf SDL-1.2.15.tgz --exclude '.DS_Store' SDL-1.2.15
