#!/bin/sh

VerLib="2.8"
OutDir="freetype"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="https://downloads.sourceforge.net/project/freetype/freetype2/${VerLib}/${FileName}"
SHA256Sum="33a28fabac471891d0523033e99c0005b95e5618dc8ffa7fa47f9dadcacb1c9b"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
