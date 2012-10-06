#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-fdf2282"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="44f9d1204f55b6fe536e3dc86f454f4e"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
