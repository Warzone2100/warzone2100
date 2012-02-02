#!/bin/sh

DirectorY="libtheora-1.1.1"
OutDir="libtheora"
FileName="libtheora-1.1.1.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz"
MD5Sum="bb4dc37f0dc97db98333e7160bfbb52b"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
