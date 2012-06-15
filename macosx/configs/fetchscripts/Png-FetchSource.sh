#!/bin/sh

VerLib="1.5.11"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/${VerLib}/${FileName}"
MD5Sum="7373f65ca1407ce9910d634e527e8959"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
