#!/bin/sh

VerLib="1.10.0"
OutDir="glew"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
SourceDLP="http://downloads.sourceforge.net/project/glew/glew/${VerLib}/${FileName}"
MD5Sum="2f09e5e6cb1b9f3611bcac79bc9c2d5d"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
