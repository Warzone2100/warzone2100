#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-1974546"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="ec7fafe1ffc4f2c94f2e916e85489982"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
