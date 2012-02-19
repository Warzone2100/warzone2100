#!/bin/sh

OutDir="libpng"
DirectorY="${OutDir}-1.5.9"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/1.5.9/${FileName}"
MD5Sum="c740ba66cd7074ba2471b6a4ff48e1fb"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
