#!/bin/sh

OutDir="zlib"
DirectorY="${OutDir}-1.2.7"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://zlib.net/${FileName}"
MD5Sum="60df6a37c56e7c1366cca812414f7b85"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
