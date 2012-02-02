#!/bin/sh

OutDir="libogg"
DirectorY="${OutDir}-1.3.0"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/ogg/${FileName}"
MD5Sum="0a7eb40b86ac050db3a789ab65fe21c2"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
