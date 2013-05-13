#!/bin/sh

VerLib="1.2.8"
OutDir="zlib"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://zlib.net/${FileName}"
MD5Sum="44d667c142d7cda120332623eab69f40"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
