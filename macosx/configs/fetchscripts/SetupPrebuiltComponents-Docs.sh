#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-7f6ad1d"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="32240b607980ba99cd66fe2740330e5a"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
