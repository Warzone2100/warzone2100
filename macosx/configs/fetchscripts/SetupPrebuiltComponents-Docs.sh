#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-a9c2db1"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="591d5dc15c4e56a61eb8e4433de31a8d"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
