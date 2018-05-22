#!/bin/sh

OutDir="libogg"
DirectorY="${OutDir}-1.3.3"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/ogg/${FileName}"
SHA256Sum="c2e8a485110b97550f453226ec644ebac6cb29d1caef2902c007edab4308d985"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
