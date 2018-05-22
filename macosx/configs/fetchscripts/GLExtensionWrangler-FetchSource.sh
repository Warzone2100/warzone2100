#!/bin/sh

VerLib="2.1.0"
OutDir="glew"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
SourceDLP="https://downloads.sourceforge.net/project/glew/glew/${VerLib}/${FileName}"
SHA256Sum="04de91e7e6763039bc11940095cd9c7f880baba82196a7765f727ac05a993c95"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
