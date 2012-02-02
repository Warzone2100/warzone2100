#!/bin/sh

DirectorY="usr"
OutDir="usr-fontconfig"
FileName="fontconfig-2.8.0-darwin9-bin3.tar.gz"
BuiltDLP="http://r.research.att.com/libs/fontconfig-2.8.0-darwin9-bin3.tar.gz"
MD5Sum="5d53bd02deb019ac972481afef05562c"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
