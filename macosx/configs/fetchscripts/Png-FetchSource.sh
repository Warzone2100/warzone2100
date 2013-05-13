#!/bin/sh

VerLib="1.5.15"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/${VerLib}/${FileName}"
MD5Sum="ea24254980fd820964a710e4d2a947c7"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
