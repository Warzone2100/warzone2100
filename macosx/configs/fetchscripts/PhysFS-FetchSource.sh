#!/bin/sh

OutDir="physfs"
DirectorY="${OutDir}-3.0.1"
FileName="${DirectorY}.tar.bz2"
SourceDLP="https://icculus.org/physfs/downloads/${FileName}"
SHA256Sum="b77b9f853168d9636a44f75fca372b363106f52d789d18a2f776397bf117f2f1"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
