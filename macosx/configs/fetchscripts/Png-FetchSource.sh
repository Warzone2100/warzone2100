#!/bin/sh

OutDir="libpng"
DirectorY="${OutDir}-1.5.10"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/1.5.10/${FileName}"
MD5Sum="9e5d864bce8f06751bbd99962ecf4aad"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
