#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-00f437c"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="ec7759f4702cf93f54ed9eb69406421d"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
