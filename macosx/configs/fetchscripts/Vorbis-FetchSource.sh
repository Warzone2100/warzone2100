#!/bin/sh

OutDir="libvorbis"
DirectorY="${OutDir}-1.3.6"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/vorbis/${FileName}"
SHA256Sum="6ed40e0241089a42c48604dc00e362beee00036af2d8b3f46338031c9e0351cb"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
