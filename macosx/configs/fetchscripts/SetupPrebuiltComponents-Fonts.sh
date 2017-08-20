#!/bin/sh

DirectorY="fonts_"
OutDir="fonts"
FileName="fonts.tar.gz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/fonts.tar.gz"
SHA256Sum="97f97bd9ec589e6aa182e3366cf420dfa01d603cbdc2b36527156e709a6379c7"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}"
exit ${?}

# tar -czf fonts.tar.gz --exclude '.DS_Store' fonts_
