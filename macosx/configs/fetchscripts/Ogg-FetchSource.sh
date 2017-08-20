#!/bin/sh

OutDir="libogg"
DirectorY="${OutDir}-1.3.2"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/ogg/${FileName}"
SHA256Sum="e19ee34711d7af328cb26287f4137e70630e7261b17cbe3cd41011d73a654692"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
