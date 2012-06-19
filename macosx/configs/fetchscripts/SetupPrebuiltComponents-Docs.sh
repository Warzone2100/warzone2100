#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-b66426c"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="e0f3b5d15efa4a063461c1ffb21769b8"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
