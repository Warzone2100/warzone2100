#!/bin/sh

VerLib="4.8.4"
OutDir="QT"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="72739f2e24a6716b0bde2bdc95a44ceb"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}

# tar -czf QT-4.8.4.tgz --exclude '.DS_Store' QT-4.8.4
