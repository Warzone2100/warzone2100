#!/bin/sh

DirectorY="usr"
OutDir="usr-freetype"
FileName="freetype-2.4.4-darwin9-bin4.tar.gz"
BuiltDLP="http://r.research.att.com/libs/freetype-2.4.4-darwin9-bin4.tar.gz"
MD5Sum="1c782dbe8176d987666cdc0e73194862"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
