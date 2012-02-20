#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-e0bcef3"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="015f892f2d635612ad8d6d4a439ff36e"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
