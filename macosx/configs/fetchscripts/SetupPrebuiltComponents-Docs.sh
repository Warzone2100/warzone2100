#!/bin/sh

DirectorY="WarzoneHelp-44d8bf7"
OutDir="WarzoneHelp"
FileName="WarzoneHelp-44d8bf7.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/WarzoneHelp-44d8bf7.tgz"
MD5Sum="606a2d131a9d5de498eb24e5887ae4ae"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
