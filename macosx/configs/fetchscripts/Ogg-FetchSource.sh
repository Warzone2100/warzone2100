#!/bin/sh

OutDir="libogg"
DirectorY="${OutDir}-1.3.1"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/ogg/${FileName}"
MD5Sum="ba526cd8f4403a5d351a9efaa8608fbc"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
