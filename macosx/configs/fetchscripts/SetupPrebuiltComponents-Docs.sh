#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-5e1740a"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="1512cb2f8eb7dd5ff907fa5c88f0de9c"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
