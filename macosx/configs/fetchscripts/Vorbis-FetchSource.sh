#!/bin/sh

DirectorY="libvorbis-1.3.2"
OutDir="libvorbis"
FileName="libvorbis-1.3.2.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.2.tar.gz"
MD5Sum="c870b9bd5858a0ecb5275c14486d9554"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
