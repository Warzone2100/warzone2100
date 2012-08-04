#!/bin/sh

VerLib="1.8.0"
OutDir="glew"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
SourceDLP="http://downloads.sourceforge.net/project/glew/glew/${VerLib}/${FileName}"
MD5Sum="07c47ad0253e5d9c291448f1216c8732"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
