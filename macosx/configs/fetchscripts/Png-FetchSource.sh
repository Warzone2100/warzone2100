#!/bin/sh

DirectorY="libpng-1.5.8"
OutDir="libpng"
FileName="libpng-1.5.8.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/1.5.8/libpng-1.5.8.tar.gz"
MD5Sum="dc2b84a1c077531ceb5bf9d79ad889a4"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
