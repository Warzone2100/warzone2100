#!/bin/sh

DirectorY="fonts_"
OutDir="fonts"
FileName="fonts.tar.gz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/fonts.tar.gz"
MD5Sum="1b8805b36f6f1ba71026d6b5ece1ed52"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}

# tar -czf fonts.tar.gz --exclude '.DS_Store' fonts_
