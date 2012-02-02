#!/bin/sh

DirectorY="WarzoneHelp-8d40cbe"
OutDir="WarzoneHelp"
FileName="WarzoneHelp-8d40cbe.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/WarzoneHelp-8d40cbe.tgz"
MD5Sum="b5f5622d51fe8c2c364469192b483816"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
