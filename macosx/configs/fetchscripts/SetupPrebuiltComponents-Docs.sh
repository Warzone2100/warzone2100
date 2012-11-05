#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-5997d95"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="72c0fd20b480892b6bcd9d9c31810491"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
