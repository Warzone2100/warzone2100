#!/bin/sh

OutDir="libvorbis"
DirectorY="${OutDir}-1.3.5"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/vorbis/${FileName}"
SHA256Sum="6efbcecdd3e5dfbf090341b485da9d176eb250d893e3eb378c428a2db38301ce"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
