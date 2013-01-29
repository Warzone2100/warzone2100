#!/bin/sh

VerLib="1.5.14"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/${VerLib}/${FileName}"
MD5Sum="27e76e0223d654093ffeb2f3daa56cc3"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
