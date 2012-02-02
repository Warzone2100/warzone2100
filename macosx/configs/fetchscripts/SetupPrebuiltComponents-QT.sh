#!/bin/sh

DirectorY="QT-4.7.4"
OutDir="QT"
FileName="QT-4.7.4.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/QT-4.7.4.tgz"
MD5Sum="74e410b376e1c19b0990f637f646f26e"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}

# tar -czf QT-4.7.4.tgz --exclude '.DS_Store' QT-4.7.4
