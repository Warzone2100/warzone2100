#!/bin/sh

VerLib="1.6.32"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng16/${VerLib}/${FileName}"
SHA256Sum="1a8ae5c8eafad895cc3fce78fbcb6fdef663b8eb8375f04616e6496360093abb"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
