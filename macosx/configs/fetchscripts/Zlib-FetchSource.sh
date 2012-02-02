#!/bin/sh

DirectorY="zlib-1.2.6"
OutDir="zlib"
FileName="zlib-1.2.6.tar.gz"
SourceDLP="http://zlib.net/zlib-1.2.6.tar.gz"
MD5Sum="618e944d7c7cd6521551e30b32322f4a"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
