#!/bin/sh

VerLib="5.9.1"
OutDir="QT"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
BuiltDLP="https://github.com/past-due/wz2100-mac-build-tools/raw/master/${FileName}"
SHA256Sum="d7e850a411dc394ea1f59de094a29304a56faa02b0699dbf1dcfee175155ef5d"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}"
exit ${?}

# tar -czf QT-5.9.1.tgz --exclude '.DS_Store' QT-5.9.1
