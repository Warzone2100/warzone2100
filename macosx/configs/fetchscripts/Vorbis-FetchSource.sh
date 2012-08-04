#!/bin/sh

OutDir="libvorbis"
DirectorY="${OutDir}-1.3.3"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/vorbis/${FileName}"
MD5Sum="6b1a36f0d72332fae5130688e65efe1f"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
