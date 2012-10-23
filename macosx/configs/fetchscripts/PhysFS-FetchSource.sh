#!/bin/sh

OutDir="physfs"
DirectorY="${OutDir}-2.0.3"
FileName="${DirectorY}.tar.bz2"
SourceDLP="http://icculus.org/physfs/downloads/${FileName}"
MD5Sum="c2c727a8a8deb623b521b52d0080f613"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
