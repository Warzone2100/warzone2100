#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-94d7d61"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="1d39a208b38537d401db310ac362b46a"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
