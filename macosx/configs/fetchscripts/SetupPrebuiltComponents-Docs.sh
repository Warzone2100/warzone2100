#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-41eac5e"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="ebe80baed4440bdbce68c495c4f51c4d"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
