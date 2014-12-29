#!/bin/sh

VerLib="1.5.21"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/${VerLib}/${FileName}"
MD5Sum="5a399a6dd143cb82cdb6c8d98c75fa42"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
